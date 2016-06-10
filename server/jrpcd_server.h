/* JRPCD (Json RPC Daemon)
 * Author: Karthik Shanmugam
 * Email: kshanmu4@visteon.com
 * Date: 10-June-2016
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
 
#ifndef JRPCD_SERVER_H
#define JRPCD_SERVER_H

#include <stdint.h>

int8_t jrpcd_server_init(int8_t * host, uint32_t port);
void jrpcd_server_loop(void);

#endif	//JRPCD_SERVER_H
