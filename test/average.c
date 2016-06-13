/* JRPCD (Json RPC Daemon)
 * Author: Karthik Shanmugam
 * Email: kshanmu4@visteon.com
 * Date: 10-June-2016
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <sys/time.h>

#include "jrpc.h"

int getinfo(void *ret, char *afmt);

struct if_details ifs[] = {
	{"getinfo", getinfo, "", "%s"}
};

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
	int result, a, b, x, y, z;
	char string[4096];
	struct timeval t1, t2;
	long time;

	printf("Initializing jrpc...\n");
	jrpc_init();		// establishes connection with server and creates a thread

	printf("Registering this's public interfaces with jrpc...\n");
	jrpc_register("app_avg", sizeof(ifs) / sizeof(ifs[0]), ifs, NULL);

	a = 1;
	b = (1 << 31) - 1;

	x = 10000;
	y = 20000;
	z = 30000;

	printf("Going to call functions of \"app_sum\" remotely...\n");
	gettimeofday(&t1, NULL);
	jrpc_call("app_sum", "add2", &result, "%d%d", a, b);
	gettimeofday(&t2, NULL);
	printf("a = %d, b = %d, a+b = %d\n", a, b, result);
	time = (t2.tv_sec - t1.tv_sec)*1000000;
	time += (t2.tv_usec - t1.tv_usec);
	printf("Duration of last call = %ld us\n\n", time);

	gettimeofday(&t1, NULL);
	jrpc_call("app_sum", "add3", &result, "%d%d%d", x, y, z);
	gettimeofday(&t2, NULL);
	printf("x = %d, y = %d, z = %d, x+y+z = %d\n", x, y, z, result);
	time = (t2.tv_sec - t1.tv_sec)*1000000;
	time += (t2.tv_usec - t1.tv_usec);
	printf("Duration of last call = %ld us\n\n", time);

	gettimeofday(&t1, NULL);
	jrpc_call("app_sum", "getinfo", string, "");
	gettimeofday(&t2, NULL);
	printf("Return string = %s\n", string);
	time = (t2.tv_sec - t1.tv_sec)*1000000;
	time += (t2.tv_usec - t1.tv_usec);
	printf("Duration of last call = %ld us\n\n", time);

	printf("Application will exit now!!\n");
	jrpc_exit();
}
