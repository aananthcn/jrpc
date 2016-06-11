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
#include <jansson.h>

#include "jrpcd_parser.h"
#include "debug.h"

void jrpcd_parser_init(void **root, char *data)
{
	*root = NULL;
	*root = (void *)json_loads(data, 0, NULL);
}

void jrpcd_parser_cleanup(void *obj)
{
	json_t *root = (json_t *) obj;
	json_decref(root);
	return;
}

int8_t jrpcd_parser_get_api(void *obj, uint8_t * api_type)
{
	json_t *root = (json_t *) obj;
	json_t *api;
	const char *api_str;

	if (root == NULL) {
		LOG_ERR("%s",
			"Json root is null, did you call jrpcd_parser_init()?");
		goto exit_0;
	}

	if (!json_is_object(root)) {
		LOG_ERR("%s", "Json root is not object");
		goto exit_0;
	}

	api = json_object_get(root, "api");
	if (api == NULL) {
		LOG_ERR("%s", "api not found in the JSON");
		goto exit_0;
	} else if (!json_is_string(api)) {
		LOG_ERR("%s", "api is not string");
		goto exit_0;
	} else {
		api_str = json_string_value(api);
		if (api_str == NULL) {
			LOG_ERR("%s", "cannot extract api string");
			goto exit_0;
		} else {
			if (strcmp("register", api_str) == 0) {
				*api_type = JRPCD_API_REGISTER;
			} else if (strcmp("call", api_str) == 0) {
				*api_type = JRPCD_API_CALL;
			} else if (strcmp("return", api_str) == 0) {
				*api_type = JRPCD_API_RETURN;
			} else if (strcmp("exit", api_str) == 0) {
				*api_type = JRPCD_API_EXIT;
			} else {
				LOG_ERR("Unknown API : %s", api_str);
				goto exit_0;
			}
		}
	}
	return 0;
 exit_0:
	return -1;
}

int8_t jrpcd_parser_get_snode(void *obj, char *snode, uint16_t size)
{
	json_t *root = (json_t *) obj;
	json_t *node;
	const char *node_str;

	if (root == NULL) {
		LOG_ERR("%s",
			"Json root is null, did you call jrpcd_parser_init()?");
		goto exit_0;
	}

	if (!json_is_object(root)) {
		LOG_ERR("%s", "Json root is not object");
		goto exit_0;
	}

	node = json_object_get(root, "snode");
	if (node == NULL) {
		LOG_ERR("%s", "snode not found in the JSON");
		goto exit_0;
	} else if (!json_is_string(node)) {
		LOG_ERR("%s", "snode is not string");
		goto exit_0;
	} else {
		node_str = json_string_value(node);
		if (node_str == NULL) {
			LOG_ERR("%s", "cannot extract snode string");
			goto exit_0;
		} else {
			strncpy(snode, node_str, size);
		}
	}
	return 0;
 exit_0:
	return -1;
}

int8_t jrpcd_parser_get_dnode(void *obj, char *dnode, uint16_t size)
{
	json_t *root = (json_t *) obj;
	json_t *node;
	const char *node_str;

	if (root == NULL) {
		LOG_ERR("%s",
			"Json root is null, did you call jrpcd_parser_init()?");
		goto exit_0;
	}

	if (!json_is_object(root)) {
		LOG_ERR("%s", "Json root is not object");
		goto exit_0;
	}

	node = json_object_get(root, "dnode");
	if (node == NULL) {
		LOG_ERR("%s", "dnode not found in the JSON");
		goto exit_0;
	} else if (!json_is_string(node)) {
		LOG_ERR("%s", "dnode is not string");
		goto exit_0;
	} else {
		node_str = json_string_value(node);
		if (node_str == NULL) {
			LOG_ERR("%s", "cannot extract dnode string");
			goto exit_0;
		} else {
			strncpy(dnode, node_str, size);
		}
	}
	return 0;
 exit_0:
	return -1;

}

int8_t jrpcd_parser_register_get_num_intf(void *obj, uint16_t * num_intf)
{
	json_t *root = (json_t *) obj;
	json_t *intf;

	if (root == NULL) {
		LOG_ERR("%s",
			"Json root is null, did you call jrpcd_parser_init()?");
		goto exit_0;
	}

	if (!json_is_object(root)) {
		LOG_ERR("%s", "Json root is not object");
		goto exit_0;
	}

	intf = json_object_get(root, "interfaces");
	if (intf == NULL) {
		LOG_ERR("%s", "interfaces not found in the JSON");
		goto exit_0;
	} else if (!json_is_array(intf)) {
		LOG_ERR("%s", "interfaces is not array");
		goto exit_0;
	} else {
		*num_intf = json_array_size(intf);
	}
	return 0;
 exit_0:
	return -1;
}

void *jrpcd_parser_register_get_intf(void *obj, uint16_t index)
{
	json_t *root = (json_t *) obj;
	json_t *intf = NULL;
	json_t *intf_item = NULL;
	uint32_t num_intf;

	if (root == NULL) {
		LOG_ERR("%s",
			"Json root is null, did you call jrpcd_parser_init()?");
		goto exit_0;
	}

	if (!json_is_object(root)) {
		LOG_ERR("%s", "Json root is not object");
		goto exit_0;
	}

	intf = json_object_get(root, "interfaces");
	if (intf == NULL) {
		LOG_ERR("%s", "interfaces not found in the JSON");
		goto exit_0;
	} else if (!json_is_array(intf)) {
		LOG_ERR("%s", "interfaces is not array");
		goto exit_0;
	} else {
		num_intf = json_array_size(intf);
		if (num_intf > index) {
			intf_item = json_array_get(intf, index);
		} else {
			LOG_ERR("%s", "index out of bounds");
			goto exit_0;
		}
	}
	return (void *)intf_item;
 exit_0:
	return NULL;
}

int8_t jrpcd_parser_register_intf_get_name(void *vintf, char *name,
					   uint16_t size)
{
	json_t *intf = (json_t *) vintf;
	json_t *node;
	const char *node_str;

	if (intf == NULL) {
		LOG_ERR("%s", "Interface pointer is NULL");
		goto exit_0;
	}

	if (!json_is_object(intf)) {
		LOG_ERR("%s", "Interface pointer is not object");
		goto exit_0;
	}

	node = json_object_get(intf, "if");
	if (node == NULL) {
		LOG_ERR("%s", "if not found in the interface object");
		goto exit_0;
	} else if (!json_is_string(node)) {
		LOG_ERR("%s", "if is not string");
		goto exit_0;
	} else {
		node_str = json_string_value(node);
		if (node_str == NULL) {
			LOG_ERR("%s", "cannot extract if string");
			goto exit_0;
		} else {
			strncpy(name, node_str, size);
		}
	}
	return 0;
 exit_0:
	return -1;
}

int8_t jrpcd_parser_register_intf_get_arg(void *vintf, char *arg,
					  uint16_t size)
{
	json_t *intf = (json_t *) vintf;
	json_t *node;
	const char *node_str;

	if (intf == NULL) {
		LOG_ERR("%s", "Interface pointer is NULL");
		goto exit_0;
	}

	if (!json_is_object(intf)) {
		LOG_ERR("%s", "Interface pointer is not object");
		goto exit_0;
	}

	node = json_object_get(intf, "arg");
	if (node == NULL) {
		LOG_ERR("%s", "arg not found in the interface object");
		goto exit_0;
	} else if (!json_is_string(node)) {
		LOG_ERR("%s", "arg is not string");
		goto exit_0;
	} else {
		node_str = json_string_value(node);
		if (node_str == NULL) {
			LOG_ERR("%s", "cannot extract arg string");
			goto exit_0;
		} else {
			strncpy(arg, node_str, size);
		}
	}
	return 0;
 exit_0:
	return -1;
}

int8_t jrpcd_parser_register_intf_get_ret(void *vintf, char *ret,
					  uint16_t size)
{
	json_t *intf = (json_t *) vintf;
	json_t *node;
	const char *node_str;

	if (intf == NULL) {
		LOG_ERR("%s", "Interface pointer is NULL");
		goto exit_0;
	}

	if (!json_is_object(intf)) {
		LOG_ERR("%s", "Interface pointer is not object");
		goto exit_0;
	}

	node = json_object_get(intf, "ret");
	if (node == NULL) {
		LOG_ERR("%s", "ret not found in the interface object");
		goto exit_0;
	} else if (!json_is_string(node)) {
		LOG_ERR("%s", "ret is not string");
		goto exit_0;
	} else {
		node_str = json_string_value(node);
		if (node_str == NULL) {
			LOG_ERR("%s", "cannot extract ret string");
			goto exit_0;
		} else {
			strncpy(ret, node_str, size);
		}
	}
	return 0;
 exit_0:
	return -1;
}

int8_t jrpcd_parser_call_get_intf(void *obj, char *intf, uint16_t size)
{
	json_t *root = (json_t *) obj;
	json_t *node;
	const char *node_str;

	if (root == NULL) {
		LOG_ERR("%s",
			"Json root is null, did you call jrpcd_parser_init()?");
		goto exit_0;
	}

	if (!json_is_object(root)) {
		LOG_ERR("%s", "Json root is not object");
		goto exit_0;
	}

	node = json_object_get(root, "if");
	if (node == NULL) {
		LOG_ERR("%s", "if not found in the JSON");
		goto exit_0;
	} else if (!json_is_string(node)) {
		LOG_ERR("%s", "if is not string");
		goto exit_0;
	} else {
		node_str = json_string_value(node);
		if (node_str == NULL) {
			LOG_ERR("%s", "cannot extract if string");
			goto exit_0;
		} else {
			strncpy(intf, node_str, size);
		}
	}
	return 0;
 exit_0:
	return -1;
}
