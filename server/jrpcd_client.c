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
#include <errno.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>

#include "jrpcd_client.h"
#include "jrpcd_queue.h"
#include "jrpcd.h"
#include "debug.h"

#define RX_BUFF_MAX_SZ			JRPCD_MAX_MSG_SZ

struct th_data {
	uint32_t cid;
	uint32_t sock;
	void *tx_q;
};

void *jrpcd_client_transmit_thread(void *arg)
{
	struct th_data *data = (struct th_data *)arg;
	void *buffer = NULL;

	LOG_VERBOSE("Tx thread created for cid: %d", data->cid);

	/* Setup thread as cancellable */
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

	while (0 == jrpcd_exit_pending()) {
		/* Wait for data to be available in the queue */
		/* Below call will be blocked until there is some data in */
		/* in the queue. */
		uint32_t size = jrpcd_queue_get(data->tx_q, &buffer);
		if (buffer == NULL) {
			LOG_ERR("Empty item recevied for cid %d", data->cid);
			continue;
		}

		LOG_VERBOSE("sending %s to cid %d", (char *)buffer, data->cid);
		/* We have adata to send */
		if (send(data->sock, buffer, size, 0) < 0) {
			LOG_ERR("%s", "send failed");
			goto exit_0;
		}
		/* Free up buffer after sending the data */
		free(buffer);
	}

 exit_0:
	LOG_VERBOSE("Tx thread exited for cid: %d", data->cid);
	pthread_exit(NULL);
}

void *jrpcd_client_receive_thread(void *arg)
{
	struct th_data *data = (struct th_data *)arg;
	fd_set readfds;
	int32_t rc;
	uint32_t recv_bytes;
	uint8_t *buff;

	LOG_VERBOSE("Rx thread created for cid: %d", data->cid);

	/* Setup thread as cancellable */
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

	/* Allocate memory to receive data */
	buff = (uint8_t *) malloc(RX_BUFF_MAX_SZ);
	if (buff == NULL) {
		LOG_ERR("%s", "malloc failed");
		goto exit_0;
	}

	while (0 == jrpcd_exit_pending()) {
		/* Wait for data */
		FD_ZERO(&readfds);
		FD_SET(data->sock, &readfds);
		rc = select(data->sock + 1, &readfds, NULL, NULL, NULL);
		if (rc < 0) {
			goto exit_0;
		} else if (rc == 0) {
			continue;
		}

		if ((0 == jrpcd_exit_pending()) &&
		    FD_ISSET(data->sock, &readfds)) {

			memset(buff, 0, RX_BUFF_MAX_SZ);

			/* Data available. Read now. */
			recv_bytes = recv(data->sock, buff, RX_BUFF_MAX_SZ, 0);
			if (recv_bytes > 0) {
				LOG_INFO("Received for cid %d : %s", data->cid,
					 buff);
				/* Process received data */
				jrpcd_process_recv(data->cid, buff, recv_bytes);
			}
		}
	}
 exit_0:
	LOG_VERBOSE("Rx thread exited for cid: %d", data->cid);
	pthread_exit(NULL);
}

int8_t jrpcd_client_create(uint32_t csock, uint32_t cid, pthread_t * tid,
			   pthread_t * rid, void *tx_q)
{
	struct th_data *tx_data;
	struct th_data *rx_data;

	pthread_attr_t tx_attr;
	pthread_attr_t rx_attr;

	/* Setup pthread attributes */
	pthread_attr_init(&tx_attr);
	pthread_attr_setdetachstate(&tx_attr, PTHREAD_CREATE_DETACHED);

	pthread_attr_init(&rx_attr);
	pthread_attr_setdetachstate(&rx_attr, PTHREAD_CREATE_DETACHED);

	/* Allocate memory for transmit thread data */
	tx_data = (struct th_data *)malloc(sizeof(struct th_data));
	if (tx_data == NULL) {
		LOG_ERR("%s", "malloc failed");
		goto exit_0;
	}

	/* Allocate memory for receive thread data */
	rx_data = (struct th_data *)malloc(sizeof(struct th_data));
	if (rx_data == NULL) {
		LOG_ERR("%s", "malloc failed");
		goto exit_1;
	}

	/* Queue is only created for Transmit thread */
	tx_data->cid = cid;
	tx_data->sock = csock;
	tx_data->tx_q = tx_q;

	if (pthread_create
	    (tid, &tx_attr, jrpcd_client_transmit_thread,
	     (void *)tx_data) < 0) {
		LOG_ERR("%s", "pthread_create failed");
		goto exit_2;
	}

	rx_data->cid = cid;
	rx_data->sock = csock;

	if (pthread_create
	    (rid, &rx_attr, jrpcd_client_receive_thread, (void *)rx_data) < 0) {
		LOG_ERR("%s", "pthread_create failed");
		goto exit_3;
	}

	return 0;
 exit_3:
	pthread_cancel(*tid);
 exit_2:
	free(rx_data);
 exit_1:
	free(tx_data);
 exit_0:
	return -1;
}
