/* JRPCD (Json RPC Daemon)
 * Author: Karthik Shanmugam
 * Email: kshanmu4@visteon.com
 * Date: 10-June-2016
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define DEBUG_NONE	0
#define DEBUG_VERBOSE	1
#define DEBUG_INFO	2
#define DEBUG_ERR	4

#define DEBUG 		(DEBUG_VERBOSE | DEBUG_INFO | DEBUG_ERR)

#if (DEBUG) & DEBUG_VERBOSE
#define LOG_VERBOSE(fmt, ...)	\
	fprintf(stderr, "[VERSBOSE] : " fmt "\n", __VA_ARGS__)
#else
#define LOG_VERBOSE(fmt, ...)
#endif

#if (DEBUG) & DEBUG_INFO
#define LOG_INFO(fmt, ...)	\
	fprintf(stderr, "[INFO] <%s:%d> : " fmt "\n", __FUNCTION__, __LINE__, __VA_ARGS__)
#else
#define LOG_INFO(fmt, ...)
#endif

#if (DEBUG) & DEBUG_ERR
#define LOG_ERR(fmt, ...)	\
	fprintf(stderr, "[ERROR] <%s:%d> : " fmt "\n", __FUNCTION__, __LINE__, __VA_ARGS__)
#else
#define LOG_ERR(fmt, ...)
#endif

#endif				//DEBUG_H
