/* JRPCD (Json RPC Daemon)
 * Author: Karthik Shanmugam
 * Email: kshanmu4@visteon.com
 * Date: 10-June-2016
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>

#include "jrpc.h"

int add_2(void *ret, char *afmt);
int add_3(void *ret, char *afmt);
int getinfo(void *ret, char *afmt);

struct if_details ifs[] = {
	{"add2", add_2, "%d%d", "%d"},
	{"add3", add_3, "%d%d%d", "%d"},
	{"getinfo", getinfo, "", "%s"}
};

int add_2(void *ret, char *afmt)
{
	int a, b;
	int *result;

	/* jrpc interfaces needs to call the following to get arguments */
	if (jrpc_scanargs(afmt, &a, &b) < 0)
		return -1;

	/* get the return address using the macro call */
	result = RETURN_POINTER(ret, int);

	/* store the value */
	*result = a + b;

	return 0;
}

int add_3(void *ret, char *afmt)
{
	int a, b, c;
	int *result;

	if (jrpc_scanargs(afmt, &a, &b, &c) < 0)
		return -1;
	result = RETURN_POINTER(ret, int);
	*result = a + b + c;

	return 0;
}

int getinfo(void *ret, char *afmt)
{
	int a, b, c;
	char *result;

	result = RETURN_POINTER(ret, char);
	sprintf(result, "This is \"%s()\", responding from %s file!\n",
		__func__, __FILE__);

	return 0;
}

void main(void)
{
	int sleep_s = 5 * 60;
	int i;

	printf("Initializing jrpc...\n");
	jrpc_init();		// establishes connection with server and creates a thread

	printf("Registering this's public interfaces with jrpc...\n");
	jrpc_register("app_sum", sizeof(ifs) / sizeof(ifs[0]), ifs, NULL);

	for (i = 0; i < sleep_s; i++) {
		printf("\"app_sum\" will sleep for %d secs\n", sleep_s--);
		sleep(1);
	}

	printf("Application will exit now!!\n");
	jrpc_exit();
}
