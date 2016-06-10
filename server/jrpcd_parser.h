/* JRPCD (Json RPC Daemon)
 * Author: Karthik Shanmugam
 * Email: kshanmu4@visteon.com
 * Date: 10-June-2016
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef JRPCD_PARSER_H
#define JRPCD_PARSER_H

#include <stdint.h>

#define JRPCD_API_REGISTER		0x0
#define JRPCD_API_CALL			0x1
#define JRPCD_API_RETURN		0x2
#define JRPCD_API_EXIT			0x3

void *jrpcd_parser_init(int8_t * data, uint32_t size);
void jrpcd_parser_cleanup(void *obj);
int8_t jrpcd_parser_get_api(void *obj, uint8_t * api_type);
int8_t jrpcd_parser_get_snode(void *obj, int8_t * snode, uint16_t size);
int8_t jrpcd_parser_get_dnode(void *obj, int8_t * dnode, uint16_t size);
int8_t jrpcd_parser_register_get_num_intf(void *obj, uint16_t * num_intf);
void *jrpcd_parser_register_get_intf(void *obj, uint16_t index);
int8_t jrpcd_parser_register_intf_get_name(void *vintf, int8_t * name,
					   uint16_t size);
int8_t jrpcd_parser_register_intf_get_arg(void *vintf, int8_t * arg,
					  uint16_t size);
int8_t jrpcd_parser_register_intf_get_ret(void *vintf, int8_t * ret,
					  uint16_t size);
int8_t jrpcd_parser_call_get_intf(void *obj, int8_t * intf, uint16_t size);

#endif	//JRPCD_PARSER_H
