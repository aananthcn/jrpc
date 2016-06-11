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
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "jrpcd_server.h"
#include "jrpcd.h"
#include "debug.h"

/* Socket to accept incoming clients */
static int sock_fd;

int8_t jrpcd_server_init(char *host, uint32_t port)
{
	struct sockaddr_in addr;

	/* Create Socket */
	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd < 0) {
		LOG_ERR("%s", "cannot open socket");
		goto exit_0;
	}

	memset(&addr, 0, sizeof(struct sockaddr_in));

	addr.sin_family = AF_INET;

	/* If host is provided bind to that IP else bind to any interface */
	if (host == NULL) {
		addr.sin_addr.s_addr = INADDR_ANY;
	} else {
		addr.sin_addr.s_addr = inet_addr(host);
	}
	addr.sin_port = htons(port);

	if (bind(sock_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in))
	    < 0) {
		LOG_ERR("%s", "cannot bind socket");
		goto exit_1;
	}

	/* Listen on the socket */
	if (listen(sock_fd, 5) < 0) {
		LOG_ERR("%s", "cannot listen on socket");
		goto exit_1;
	}

	LOG_INFO("%s", "jrpcd_server_init: success");

	return 0;

 exit_1:
	close(sock_fd);
 exit_0:
	return -1;
}

void jrpcd_server_cleanup(void)
{
	LOG_INFO("%s", "jrpcd_server_cleanup");
	close(sock_fd);
}

void jrpcd_server_loop(void)
{
	fd_set readfds;
	int32_t rc;
	int32_t csock;

	LOG_INFO("%s", "jrpcd_server_loop: begin");

	while (0 == jrpcd_exit_pending()) {
		/* Wait for clients to connect */
		FD_ZERO(&readfds);
		FD_SET(sock_fd, &readfds);
		rc = select(sock_fd + 1, &readfds, NULL, NULL, NULL);
		if (rc < 0 && errno != EINTR) {
			LOG_ERR("%s", "select error");
			continue;
		} else if (rc == 0) {
			continue;
		}

		if ((0 == jrpcd_exit_pending()) && FD_ISSET(sock_fd, &readfds)) {

			csock = accept(sock_fd, NULL, 0);
			if (csock < 0) {
				LOG_ERR("%s", "accept error");
			}
			LOG_INFO("New Client Connected %d\n", csock);

			/* Create a new jrpcd client */
			if (jrpcd_new_client(csock) < 0) {
				LOG_ERR("%s", "Error creating client threads");
				close(csock);
			}
		}
	}

	LOG_INFO("%s", "jrpcd_server_loop: end");

	/* Do clean up */
	jrpcd_server_cleanup();
}
