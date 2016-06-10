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
#include <signal.h>
#include <getopt.h>
#include <netinet/in.h>

#include "version.h"
#include "debug.h"
#include "jrpcd_server.h"

#define DEFAULT_PORT                           5000

void handle_sigint(int signal)
{
	LOG_INFO("%s", "jrpcd terminating...");
	jrpcd_exit();
}

void print_usage()
{
	printf("jrpcd -i <host> -p <port> \n");
	exit(0);
}

int main(int argc, char *argv[])
{
	int32_t c;
	int8_t *host = NULL;
	uint32_t port = DEFAULT_PORT;

	LOG_INFO("jrpcd %d.%d.%d starting...", VER_MAJ, VER_MIN, VER_PATCH);

	while ((c = getopt(argc, argv, "i:p:")) != -1) {
		switch (c) {
		case 'i':
			host = optarg;
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 'h':
			print_usage();
			break;
		default:
			print_usage();
		}
	}
	signal(SIGINT, handle_sigint);
	signal(SIGTERM, handle_sigint);

	jrpcd_main(host, port);

	return 0;
}
