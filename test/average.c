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

int getinfo(void *ret, char *afmt);


struct if_details ifs[] = {
	{ "getinfo", getinfo, "", "%s" }
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
	int result;
	char string[4096];

	printf("Initializing jrpc...\n");
	jrpc_init(); // establishes connection with server and creates a thread

	printf("Registering this's public interfaces with jrpc...\n");
	jrpc_register("app_avg", sizeof(ifs)/sizeof(ifs[0]), ifs, NULL);

	printf("Going to call functions of \"app_sum\" remotely...\n");
	jrpc_call("app_sum", "add2", &result, "%d%d", 108, 27);
	printf("Result = %d\n\n", result);
	jrpc_call("app_sum", "add3", &result, "%d%d%d", 1000000, 1000, 1);
	printf("Result = %d\n\n", result);
	jrpc_call("app_sum", "getinfo", string, "");
	printf("Return string = %s\n", string);

	printf("Application will exit now!!\n");
	jrpc_exit();
}
