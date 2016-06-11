/****************************************************************************** 
 * Author: Aananth C N <caananth@visteon.com>
 * Date: 06 Jun 2016
 *
 * Description: This is the main interface file for libjrpc.so
 *****************************************************************************/
#ifndef JRPC_H
#define JRPC_H

#include <sys/queue.h>
#include "ejson.h"

enum jrpc_states {
	JRPC_OFF,
	JRPC_CONNECTING,
	JRPC_CONNECTED,
	JRPC_INITIALISED
};

#define DEFAULT_IP		"127.0.0.1"
#define DEFAULT_PORT		5000
#define RETURN_POINTER(p, t)	((t*)p)

struct if_details {
	char if_name[NAME_SIZE];
	int (*fnptr)(void *ret, char *afmt);	/* interface pointer */
	char afmt[NAME_SIZE];			/* argument format - refer printf */
	char rfmt[NAME_SIZE];			/* return format */
};


int jrpc_init(void);
int jrpc_register(char *node, int n_if, struct if_details *ifl, void *cbptr);
int jrpc_call(char *node, char *ifname, void *ret, char *afmt, ...);
int jrpc_scanargs(const char *fmt, ...);
int jrpc_exit(void);


#endif
