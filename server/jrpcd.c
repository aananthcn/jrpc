/* JRPCD (Json RPC Daemon)
 * Author: Karthik Shanmugam
 * Email: kshanmu4@visteon.com
 * Date: 10-June-2016
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include <sys/queue.h>

#include "jrpcd.h"
#include "jrpcd_server.h"
#include "jrpcd_client.h"
#include "jrpcd_parser.h"
#include "jrpcd_queue.h"
#include "debug.h"

#define NODE_NAME_MAX_SZ		32
#define INTF_NAME_MAX_SZ		32
#define INTF_ARG_MAX_SZ			32
#define INTF_RET_MAX_SZ			 4

#define REGISTER_RESP_FMT		"{\"api\":\"ack\",\"snode\":\"jrpcd\",\"dnode\":\"%s\",\"if\":\"register\",\"ret\":{\"type\":\"int\",\"val\":%d}}"
#define CALL_ERR_RESP_FMT		"{\"api\":\"call\",\"snode\":\"jrpcd\",\"dnode\":\"%s\",\"if\":\"%s\",\"ret\":{\"type\":\"int\",\"val\":%d}}"

static uint8_t exit_pending;

/* Structure to hold the interface definitions */
struct jrpcd_intf_desc {
	char name[INTF_NAME_MAX_SZ];	/* Interface name */
	char arg[INTF_ARG_MAX_SZ];	/* Interface arguments string */
	char ret[INTF_RET_MAX_SZ];	/* Interface return type string */

	LIST_ENTRY(jrpcd_intf_desc) entries;
};

/* Structure to hold the node instance */
struct jrpcd_node_desc {
	char name[NODE_NAME_MAX_SZ];	/* Node name */
	uint32_t csock;		/* Socket to communicate to the node */
	uint32_t cid;		/* Client ID for the node */
	uint16_t num_intf;	/* Number of interfaces in the node */
	pthread_t tid;		/* Transmit thread id */
	pthread_t rid;		/* Receive thread id */
	void *tx_q;		/* Transmit data queue instance */
	LIST_HEAD(ifs_head, jrpcd_intf_desc) intf_list;	/* Inteface list */

	LIST_ENTRY(jrpcd_node_desc) entries;
};

LIST_HEAD(node_head, jrpcd_node_desc) node_list =
LIST_HEAD_INITIALIZER(node_list);
static int cid_next;

void jrpcd_destroy_node(struct jrpcd_node_desc *node)
{
	/* Free up interfaces */
	while (!LIST_EMPTY(&(node->intf_list))) {
		struct jrpcd_intf_desc *intf = LIST_FIRST(&(node->intf_list));
		LIST_REMOVE(intf, entries);
		free(intf);
	}

	/* Cancel Transmit thread */
	pthread_cancel(node->tid);

	/* Destroy transmit queue */
	jrpcd_queue_destroy(node->tx_q);

	/* Can't cancel if called from the receive thread */
	if (pthread_self() != node->rid) {
		pthread_cancel(node->rid);
	}

	/* Close the connection socket */
	close(node->csock);

	/* Remove node from node list */
	LIST_REMOVE(node, entries);
	free(node);

	/* Terminate execution if requested */
	if (pthread_self() == node->rid) {
		/* Process sent exit message, exit receive thread */
		pthread_exit(0);
		/* Should not come here */
	}
}

void jrpcd_cleanup(void)
{
	LOG_VERBOSE("%s", "jrpcd_cleanup");

	/* Destroy all the nodes */
	while (!LIST_EMPTY(&node_list)) {
		struct jrpcd_node_desc *node = LIST_FIRST(&node_list);
		jrpcd_destroy_node(node);
	}

	/* Cleanup queues */
	jrpcd_queue_cleanup();
}

void jrpcd_dump(void)
{
	struct jrpcd_node_desc *node;
	struct jrpcd_intf_desc *intf;

	LOG_VERBOSE("%s", "jrpcd_dump");

	LIST_FOREACH(node, &node_list, entries) {
		LOG_VERBOSE("Node name : %s", node->name);
		LOG_VERBOSE("CID : %d", node->cid);
		LOG_VERBOSE("Socket : %d", node->csock);
		LOG_VERBOSE("Num Interfaces : %d", node->num_intf);

		LIST_FOREACH(intf, &(node->intf_list), entries) {
			LOG_VERBOSE("\tInfterface name : %s", intf->name);
			LOG_VERBOSE("\targ : %s", intf->arg);
			LOG_VERBOSE("\tret : %s", intf->ret);
		}
	}
}

struct jrpcd_node_desc *jrpcd_get_node(uint32_t cid)
{
	struct jrpcd_node_desc *node;

	/* Find matching node by client id */
	LIST_FOREACH(node, &node_list, entries) {
		if (cid == node->cid) {
			return node;
		}
	}
	return NULL;
}

struct jrpcd_node_desc *jrpcd_get_node_by_name(char *name)
{
	struct jrpcd_node_desc *node;

	/* Find matching node by node name */
	LIST_FOREACH(node, &node_list, entries) {
		if (strcmp(name, node->name) == 0) {
			return node;
		}
	}
	return NULL;
}

void jrpcd_call_send_err_resp(void *queue, char *dnode, char *intf)
{
	char *buffer = NULL;

	buffer = (char *)malloc(JRPCD_MAX_MSG_SZ);
	if (buffer == NULL) {
		LOG_ERR("%s", "malloc failed");
		goto exit_0;
	}
	memset(buffer, 0, JRPCD_MAX_MSG_SZ);
	snprintf(buffer, JRPCD_MAX_MSG_SZ, CALL_ERR_RESP_FMT, dnode, intf, -1);

	jrpcd_queue_put(queue, buffer, strlen(buffer));

 exit_0:
	return;
}

void jrpcd_register_send_resp(void *queue, char *dnode, uint8_t val)
{
	char *buffer = NULL;

	buffer = (char *)malloc(JRPCD_MAX_MSG_SZ);
	if (buffer == NULL) {
		LOG_ERR("%s", "malloc failed");
		goto exit_0;
	}
	memset(buffer, 0, JRPCD_MAX_MSG_SZ);
	snprintf(buffer, JRPCD_MAX_MSG_SZ, REGISTER_RESP_FMT, dnode, val);

	jrpcd_queue_put(queue, buffer, strlen(buffer));

 exit_0:
	return;
}

void jrpcd_process_register(void *json_obj, uint32_t cid)
{
	char snode_name[NODE_NAME_MAX_SZ];
	struct jrpcd_node_desc *node;
	struct jrpcd_node_desc *dup_node;
	uint16_t intf_index = 0;

	memset(snode_name, 0, NODE_NAME_MAX_SZ);

	/* Register call has been received for an node */
	/* Find out node using the client id */
	node = jrpcd_get_node(cid);
	if (node == NULL) {
		LOG_ERR("No matching node found for %d", cid);
		goto exit_0;
	}
	if (jrpcd_parser_get_snode(json_obj, snode_name, NODE_NAME_MAX_SZ) < 0) {
		LOG_ERR("%s", "parser failed");
		goto exit_1;
	}

	/* Check if the node is already registered */
	dup_node = jrpcd_get_node_by_name(snode_name);
	if ((dup_node != NULL) && (node->cid != dup_node->cid)) {
		/* Duplicate registration, free previous registration */
		LOG_INFO("removing previous connection %d", dup_node->cid);
		jrpcd_destroy_node(dup_node);
	}
	strcpy(node->name, snode_name);

	/* Parse supported interfaces in the register api */
	if (jrpcd_parser_register_get_num_intf(json_obj, &(node->num_intf)) < 0) {
		LOG_INFO("%s", "No interfaces defined");
		node->num_intf = 0;
	}

	for (intf_index = 0; intf_index < node->num_intf; intf_index++) {
		void *intf =
		    jrpcd_parser_register_get_intf(json_obj, intf_index);
		if (intf != NULL) {
			struct jrpcd_intf_desc *intf_desc;
			LOG_INFO("Parsing interface %d", intf_index);

			intf_desc = (struct jrpcd_intf_desc *)
			    malloc(sizeof(struct jrpcd_intf_desc));
			if (intf_desc == NULL) {
				LOG_ERR("%s", "malloc failed");
				goto exit_1;
			}
			if (jrpcd_parser_register_intf_get_name
			    (intf, intf_desc->name, INTF_NAME_MAX_SZ) < 0) {
				LOG_ERR("%s", "parser failed");
				goto exit_1;
			}
			if (jrpcd_parser_register_intf_get_arg
			    (intf, intf_desc->arg, INTF_ARG_MAX_SZ) < 0) {
				LOG_ERR("%s", "parser failed");
				goto exit_1;
			}
			if (jrpcd_parser_register_intf_get_ret
			    (intf, intf_desc->ret, INTF_RET_MAX_SZ) < 0) {
				LOG_ERR("%s", "parser failed");
				goto exit_1;
			}
			LIST_INSERT_HEAD(&(node->intf_list), intf_desc,
					 entries);
		}
	}
	/* Send response to the client indicating registration is successfull */
	jrpcd_register_send_resp(node->tx_q, node->name, 0);
	return;
 exit_1:
	/* Send response to the client indicating registration is failed */
	/* Missing mandatory fields or malformed json or invaid values */
	jrpcd_register_send_resp(node->tx_q, node->name, -1);
 exit_0:
	return;
}

void jrpcd_process_exit(void *json_obj, uint32_t cid)
{
	struct jrpcd_node_desc *snode;

	/* Exit call has been received for an node */
	/* Find out node using the client id */
	snode = jrpcd_get_node(cid);
	if (snode == NULL) {
		LOG_ERR("No matching snode found for %d", cid);
		goto exit_0;
	}
	//TODO: Do we need to validate the snode string??

	/* Since we won't be returning to the callee free up parser */
	jrpcd_parser_cleanup(json_obj);

	/* Destroy node and free up resources */
	/* !!!! Below call will not return !!!! */
	jrpcd_destroy_node(snode);
 exit_0:
	return;
}

void jrpcd_process_call(void *json_obj, uint32_t cid, uint8_t *data,
			uint32_t size)
{
	char dnode_name[NODE_NAME_MAX_SZ];
	char snode_name[NODE_NAME_MAX_SZ];
	char intf_name[INTF_NAME_MAX_SZ];
	struct jrpcd_node_desc *snode;
	struct jrpcd_node_desc *dnode;
	uint8_t *buffer;

	memset(dnode_name, 0, NODE_NAME_MAX_SZ);
	memset(snode_name, 0, NODE_NAME_MAX_SZ);
	memset(intf_name, 0, INTF_NAME_MAX_SZ);

	/* API service call has been received for an node */
	/* Find out node using the client id */
	snode = jrpcd_get_node(cid);
	if (snode == NULL) {
		LOG_ERR("No matching snode found for %d", cid);
		goto exit_0;
	}
	/* Read the source node, destination node */
	if (jrpcd_parser_get_snode(json_obj, snode_name, NODE_NAME_MAX_SZ) < 0) {
		LOG_ERR("%s", "parser failed");
		goto exit_1;
	}
	if (jrpcd_parser_call_get_intf(json_obj, intf_name, INTF_NAME_MAX_SZ) <
	    0) {
		LOG_ERR("%s", "parser failed");
		goto exit_1;
	}
	if (strcmp(snode_name, snode->name) != 0) {
		LOG_ERR("%s", "snode name mismatch");
		goto exit_1;
	}
	if (jrpcd_parser_get_dnode(json_obj, dnode_name, NODE_NAME_MAX_SZ) < 0) {
		LOG_ERR("%s", "parser failed");
		goto exit_1;
	}
	/* Get the node instance of the destination node by name */
	dnode = jrpcd_get_node_by_name(dnode_name);
	if (dnode == NULL) {
		LOG_ERR("No matching dnode found for %s", dnode_name);
		goto exit_1;
	}
	//TODO: Add Validation

	/* Allocate buffer to copy the data, this will free'd in the */
	/* transmit thread of the destination node once the data is sent */
	buffer = (uint8_t *) malloc(size);
	if (buffer == NULL) {
		LOG_ERR("%s", "malloc failed");
		goto exit_1;
	}
	memcpy(buffer, data, size);

	/* put the data into the transmit queue of the destination node */
	jrpcd_queue_put(dnode->tx_q, buffer, size);
	return;
 exit_1:
	/* Something went wrong, indicate failure to the source node */
	jrpcd_call_send_err_resp(snode->tx_q, snode->name, intf_name);
 exit_0:
	return;
}

void jrpcd_process_return(void *json_obj, uint32_t cid, uint8_t *data,
			  uint32_t size)
{
	char dnode_name[NODE_NAME_MAX_SZ];
	char snode_name[NODE_NAME_MAX_SZ];
	char intf_name[INTF_NAME_MAX_SZ];
	struct jrpcd_node_desc *snode;
	struct jrpcd_node_desc *dnode;
	uint8_t *buffer;

	memset(dnode_name, 0, NODE_NAME_MAX_SZ);
	memset(snode_name, 0, NODE_NAME_MAX_SZ);
	memset(intf_name, 0, INTF_NAME_MAX_SZ);

	/* Return call has been received for an node */
	/* Find out node using the client id */
	snode = jrpcd_get_node(cid);
	if (snode == NULL) {
		LOG_ERR("No matching snode found for %d", cid);
		goto exit_0;
	}
	if (jrpcd_parser_get_snode(json_obj, snode_name, NODE_NAME_MAX_SZ) < 0) {
		LOG_ERR("%s", "parser failed");
		goto exit_0;
	}
	if (jrpcd_parser_call_get_intf(json_obj, intf_name, INTF_NAME_MAX_SZ) <
	    0) {
		LOG_ERR("%s", "parser failed");
		goto exit_0;
	}
	if (strcmp(snode_name, snode->name) != 0) {
		LOG_ERR("%s", "snode name mismatch");
		goto exit_0;
	}
	if (jrpcd_parser_get_dnode(json_obj, dnode_name, NODE_NAME_MAX_SZ) < 0) {
		LOG_ERR("%s", "parser failed");
		goto exit_0;
	}
	dnode = jrpcd_get_node_by_name(dnode_name);
	if (dnode == NULL) {
		LOG_ERR("No matching dnode found for %s", dnode_name);
		goto exit_0;
	}
	if (strcmp(dnode_name, dnode->name) != 0) {
		LOG_ERR("%s", "dnode name mismatch");
		goto exit_0;
	}

	/* Allocate buffer to copy the data, this will free'd in the */
	/* transmit thread of the destination node once the data is sent */
	buffer = (uint8_t *) malloc(size);
	if (buffer == NULL) {
		LOG_ERR("%s", "malloc failed");
		goto exit_0;
	}
	memcpy(buffer, data, size);

	/* put the data into the transmit queue of the destination node */
	jrpcd_queue_put(dnode->tx_q, buffer, size);
	return;
 exit_0:
	return;
}

int8_t jrpcd_process_recv(uint32_t cid, uint8_t *data, uint32_t size)
{
	void *json_obj = NULL;
	uint8_t api_type;
	int8_t ret = -1;

	/* Initialize JSON parser */
	jrpcd_parser_init(&json_obj, (char *)data);
	if (json_obj == NULL) {
		LOG_INFO("%s ==> \"%s\"", "Invalid json message", (char *)data);
		goto exit_0;
	}

	/* Find out the API */
	if (jrpcd_parser_get_api(json_obj, &api_type)) {
		LOG_ERR("%s", "parser failed");
		goto exit_1;
	}

	/* Process API request */
	if (JRPCD_API_REGISTER == api_type) {
		LOG_INFO("cid: %d, Recvd Register", cid);
		jrpcd_process_register(json_obj, cid);
	} else if (JRPCD_API_CALL == api_type) {
		LOG_INFO("cid: %d, Recvd Call", cid);
		jrpcd_process_call(json_obj, cid, data, size);
	} else if (JRPCD_API_RETURN == api_type) {
		LOG_INFO("cid: %d, Recvd Return", cid);
		jrpcd_process_return(json_obj, cid, data, size);
	} else if (JRPCD_API_EXIT == api_type) {
		LOG_INFO("cid: %d, Recvd Exit", cid);
		/* !!! Special Handling !!! */
		/* !!! Below call will not return !!! */
		jrpcd_process_exit(json_obj, cid);
	}
	ret = 0;
 exit_1:
	/* Cleanup parser to free up resources */
	jrpcd_parser_cleanup(json_obj);
 exit_0:
	return ret;
}

void jrpcd_exit(void)
{
	exit_pending = 1;
}

bool jrpcd_exit_pending(void)
{
	return exit_pending;
}

int8_t jrpcd_main(char *host, uint32_t port)
{
	LOG_VERBOSE("%s", "jrpcd_main");

	/* Initialize Variables */
	cid_next = 100;
	LIST_INIT(&node_list);

	/* Initialize Queues */
	jrpcd_queue_init();

	/* Initialize Server to accept incoming connections */
	if (0 == jrpcd_server_init(host, port)) {
		exit_pending = 0;
		jrpcd_server_loop();
	}
	jrpcd_dump();
	jrpcd_cleanup();

	return 0;
}

int8_t jrpcd_new_client(uint32_t csock)
{
	struct jrpcd_node_desc *node;

	/* Allocate memory for node instance */
	node = (struct jrpcd_node_desc *)malloc(sizeof(struct jrpcd_node_desc));
	if (node == NULL) {
		LOG_ERR("%s", "malloc failed");
		goto exit_0;
	}

	/* Create the transmit queue for the node */
	node->tx_q = jrpcd_queue_create(cid_next);
	if (node->tx_q == NULL) {
		LOG_ERR("%s", "queue creation failed");
		goto exit_1;
	}

	/* Create client handing threads */
	if (jrpcd_client_create
	    (csock, cid_next, &node->tid, &node->rid, node->tx_q) < 0) {
		LOG_ERR("%s", "client creation failed");
		goto exit_2;
	}

	/* Initialize node variables */
	memset(node->name, 0, NODE_NAME_MAX_SZ);
	node->csock = csock;
	node->cid = cid_next;
	node->num_intf = 0;
	LIST_INIT(&(node->intf_list));

	/* Insert node into the node list */
	LIST_INSERT_HEAD(&node_list, node, entries);

	cid_next++;
	return 0;
 exit_2:
	jrpcd_queue_destroy(node->tx_q);
 exit_1:
	free(node);
 exit_0:
	close(csock);
	return -1;
}
