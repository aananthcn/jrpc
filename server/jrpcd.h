/* JRPCD (Json RPC Daemon)
 * Author: Karthik Shanmugam
 * Email: kshanmu4@visteon.com
 * Date: 10-June-2016
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef JRPCD_H
#define JRPCD_H

#include <stdint.h>
#include <stdbool.h>

#define JRPCD_MAX_MSG_SZ		(4 * 1024u)

int8_t jrpcd_main(char *host, uint32_t port);
int8_t jrpcd_new_client(uint32_t csock);
int8_t jrpcd_process_recv(uint32_t cid, uint8_t *data, uint32_t size);
void jrpcd_exit(void);
bool jrpcd_exit_pending(void);

#endif				//JRPCD_H
