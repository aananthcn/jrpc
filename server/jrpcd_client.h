/* JRPCD (Json RPC Daemon)
 * Author: Karthik Shanmugam
 * Email: kshanmu4@visteon.com
 * Date: 10-June-2016
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef JRPCD_CLIENT_H
#define JRPCD_CLIENT_H

#include <stdint.h>
#include <pthread.h>

int8_t jrpcd_client_create(uint32_t csock, uint32_t cid, pthread_t * tid,
			   pthread_t * rid, void *tx_q);

#endif	//JRPCD_CLIENT_H
