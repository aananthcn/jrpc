/* JRPCD (Json RPC Daemon)
 * Author: Karthik Shanmugam
 * Email: kshanmu4@visteon.com
 * Date: 10-June-2016
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef JRPCD_QUEUE_H
#define JRPCD_QUEUE_H

#include <stdint.h>

int8_t jrpcd_queue_init(void);
void jrpcd_queue_cleanup(void);
void *jrpcd_queue_create(uint32_t cid);
void jrpcd_queue_destroy(void *queue);
uint32_t jrpcd_queue_get(void *queue, void **data);
int8_t jrpcd_queue_put(void *queue, void *data, uint32_t size);

#endif	//JRPCD_QUEUE_H
