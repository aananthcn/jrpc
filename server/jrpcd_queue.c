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
#include <pthread.h>
#include <time.h>
#include <sys/queue.h>

#include "jrpcd_queue.h"
#include "debug.h"

#define Q_MAX_ITEMS			32

struct jrpcd_item_desc {
	void *data;
	uint32_t size;
	 LIST_ENTRY(jrpcd_item_desc) entries;
};

struct jrpcd_queue_desc {
	/* Connection ID of the client creating this queue */
	uint32_t cid;
	/* Queue lenght or number of items in the queue */
	uint16_t len;
	/* Mutex and condition variable to support blocking queue */
	pthread_mutex_t mutex;
	pthread_cond_t dq_cv;
	/* Linked list to hold the queue items */
	 LIST_HEAD(q_head, jrpcd_item_desc) q;
};

int8_t jrpcd_queue_init(void)
{
	LOG_VERBOSE("%s", "jrpcd_queue_init");

	return 0;
}

void jrpcd_queue_cleanup(void)
{
	LOG_VERBOSE("%s", "jrpcd_queue_cleanup");
}

void jrpcd_queue_destroy(void *queue)
{
	struct jrpcd_queue_desc *qdesc = (struct jrpcd_queue_desc *)queue;

	LOG_VERBOSE("jrpcd_queue_destroy for cid %d", qdesc->cid);

	while (!LIST_EMPTY(&(qdesc->q))) {
		struct jrpcd_item_desc *q = LIST_FIRST(&(qdesc->q));
		if (q->data != NULL) {
			free(q->data);
		}
		LIST_REMOVE(q, entries);
		free(q);
	}
	pthread_mutex_destroy(&qdesc->mutex);
	pthread_cond_destroy(&qdesc->dq_cv);
	free(qdesc);
}

void *jrpcd_queue_create(uint32_t cid)
{
	struct jrpcd_queue_desc *qdesc;

	LOG_VERBOSE("jrpcd_queue_create for cid %d", cid);

	qdesc =
	    (struct jrpcd_queue_desc *)malloc(sizeof(struct jrpcd_queue_desc));
	if (qdesc == NULL) {
		LOG_ERR("%s", "malloc failed");
		goto exit_0;
	}

	qdesc->cid = cid;
	qdesc->len = 0;
	pthread_mutex_init(&qdesc->mutex, NULL);
	pthread_cond_init(&qdesc->dq_cv, NULL);
	LIST_INIT(&(qdesc->q));

	return ((void *)qdesc);
 exit_0:
	return NULL;
}

uint32_t jrpcd_queue_get(void *queue, void **data)
{
	struct jrpcd_queue_desc *qdesc = (struct jrpcd_queue_desc *)queue;
	struct jrpcd_item_desc *qitem = NULL;
	uint32_t size = 0;

	LOG_VERBOSE("%s", "jrpcd_queue_get");
	*data = NULL;

	pthread_mutex_lock(&qdesc->mutex);
	if (LIST_EMPTY(&qdesc->q)) {
		pthread_cond_wait(&qdesc->dq_cv, &qdesc->mutex);
	}

	qitem = LIST_FIRST(&qdesc->q);
	*data = qitem->data;
	size = qitem->size;
	LIST_REMOVE(qitem, entries);
	free(qitem);
	qdesc->len--;
	pthread_mutex_unlock(&qdesc->mutex);
	return size;
}

int8_t jrpcd_queue_put(void *queue, void *data, uint32_t size)
{
	struct jrpcd_queue_desc *qdesc = (struct jrpcd_queue_desc *)queue;
	struct jrpcd_item_desc *qitem = NULL;

	pthread_mutex_lock(&qdesc->mutex);

	if (qdesc->len == Q_MAX_ITEMS) {
		LOG_ERR("%s", "!!!Q OVERFLOW!!!");
		goto exit_0;
	}

	qitem =
	    (struct jrpcd_item_desc *)malloc(sizeof(struct jrpcd_item_desc));
	if (qitem == NULL) {
		LOG_ERR("%s", "malloc failed");
		goto exit_0;
	}
	qitem->data = data;
	qitem->size = size;

	LIST_INSERT_HEAD(&qdesc->q, qitem, entries);
	qdesc->len++;

	pthread_cond_signal(&qdesc->dq_cv);
	pthread_mutex_unlock(&qdesc->mutex);
	return 0;
 exit_0:
	pthread_mutex_unlock(&qdesc->mutex);
	return -1;
}
