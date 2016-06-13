/****************************************************************************** 
 * Author: Aananth C N <caananth@visteon.com>
 * Date: 06 Jun 2016
 *
 * Description: This is the main file that implements the functions for 
 *              libjrpc.so
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <pthread.h>

#include <jansson.h>
#include "jrpc.h"
#include "ejson.h"
#include "debug.h"

struct node_details {
	char name[NAME_SIZE];
	int n_if;
	struct if_details *ifl;
};

/******************************************************************************
 *  global variables
 */
pthread_t RecvThread;
enum jrpc_states ClientState = JRPC_OFF;
enum jrpc_states RxThreadState = JRPC_OFF;
int SockFd;
struct node_details ThisNode;
json_t *JMsgCall, *JMsgRcall;

pthread_mutex_t rcall_mutex;
pthread_cond_t rcall_condvar;
pthread_mutex_t ret_mutex;
pthread_cond_t ret_condvar;

/******************************************************************************
 * static functions
 */
static int get_sockfd(void)
{
	if (ClientState < JRPC_CONNECTED)
		return -1;
	else
		return SockFd;
}


static json_t* get_rcalljson(void)
{
	return JMsgRcall;
}


static json_t* get_calljson(void)
{
	return JMsgCall;
}



/******************************************************************************
 * jrpc_scanargs
 *
 * This is a jrpc standard to get arguments for executing a remote procedural
 * call.
 */
int jrpc_scanargs(const char *fmt, ...)
{
	json_t *jmsg, *jarray, *jrow;
	const char *p;
	int argc, retval = 0, i, c;
	va_list ap; /* var argument pointer */
	char type[16];

	/* parse the argument secion of incoming message */
	jmsg = get_rcalljson();
	jarray = json_object_get(jmsg, "args");
	if ((jarray == NULL) || (0 == (argc = json_array_size(jarray))))
	{
		LOG_ERR("%s", "no array element \'ecus\' found");
		return -1;
	}


	va_start(ap, fmt); /* make ap to point 1st unamed arg */
	for (i = c = 0, p = fmt; *p; p++, c++) {
		if (*p != '%') {
			LOG_ERR("%s %d", "check format string @", c);
			continue;
		}

		jrow = json_array_get(jarray, i++);
		if (!json_is_object(jrow)) {
			LOG_ERR("%s", "json array access failure");
			return -1;
		}
		ej_get_string(jrow, "type", type);

		switch (*++p) {
		case 'd':
			/* check if types requested and received are matching */
			if ((type[0] != '%') || (type[1] != 'd')) {
			       LOG_ERR("%s%d", "type error with arg ", i);
			       return -1;
			}
			ej_get_int(jrow, "val", va_arg(ap, int *));
			break;
		case 's':
			/* check if types requested and received are matching */
			if ((type[0] != '%') || (type[1] != 's')) {
			       LOG_ERR("%s%d", "type error with arg ", i);
			       return -1;
			}
			ej_get_string(jrow, "val", va_arg(ap, char *));
			break;
		default:
			LOG_ERR("%s", "Error: unsupported argument type");
			retval = -1;
			break;
		}

		json_decref(jrow);
	}
	json_decref(jarray);
	va_end(ap);

	return retval;
}


/******************************************************************************
 * jrpc_exit
 *
 * This function informs daemon to destroy all the interfaces published and 
 * destroys any local memory
 */
int jrpc_exit(void)
{
	int retval;

	retval = 0;

	ClientState = JRPC_OFF;
	usleep(10000); /* give 10ms deadline for the rx thread to exit */

	if(ThisNode.ifl != NULL)
		free(ThisNode.ifl);

	pthread_mutex_destroy(&rcall_mutex);
	pthread_cond_destroy(&rcall_condvar);
	pthread_mutex_destroy(&ret_mutex);
	pthread_cond_destroy(&ret_condvar);

	return retval;
}


/******************************************************************************
 * jrpc_rcall
 *
 * This function does a reverse call by translating the json text received from
 * the socket connection to a function call.
 */
int jrpc_rcall(char *buffer)
{
	json_t *jroot;
	json_t *jobj;
        void *result;
	char *rfmt, *afmt;
	int i, sockfd, retval;
	char interface[NAME_SIZE];
	char caller[NAME_SIZE];
	char sendbuf[BUFF_SIZE], resultbuf[BUFF_SIZE];
	int (*fnptr)(void*, char*);

	result = (void *)resultbuf;

	/* load buffer */
	jroot = json_object();
	ej_load_buf(buffer, &jroot);

	/* decode the interface name */
	ej_get_string(jroot, "if", interface);
	ej_get_string(jroot, "snode", caller);
	for (i = 0; i < ThisNode.n_if; i++) {
		if (strcmp(interface, ThisNode.ifl[i].if_name) == 0) {
			fnptr = ThisNode.ifl[i].fnptr;
			afmt = ThisNode.ifl[i].afmt;
			JMsgRcall = jroot; // note: consumed by jrpc_scanargs()
			LOG_VERBOSE("%s(void*, %s)", interface, afmt);
			retval = fnptr(result, afmt);
			JMsgRcall = NULL;
			rfmt = ThisNode.ifl[i].rfmt;
			break;
		}
	}
	json_decref(jroot);

	if (i >= ThisNode.n_if) {
		LOG_ERR("%s: %s()", "invalid interface", interface);
		return -1;
	}

	if ((retval < 0) || (result == NULL)) {
		LOG_ERR("%s", "rcall failed!");
		LOG_INFO("retval %d, result %p", retval, result);
		return -1;
	}

	// populate the result and send it back to the caller
	jroot = json_object();
	ej_add_string(&jroot, "api", "return");
	ej_add_string(&jroot, "snode", ThisNode.name);
	ej_add_string(&jroot, "dnode", caller);
	ej_add_string(&jroot, "if", interface);

	jobj = json_object();
	json_object_set(jroot, "ret", jobj);

	ej_add_string(&jobj, "type", rfmt);
	if ((rfmt[0] == '%') && (rfmt[1] == 'd'))
		ej_add_int(&jobj, "val", *((int*)result));
	else
		ej_add_string(&jobj, "val", ((char*)result));

	ej_store_buf(jroot, sendbuf, BUFF_SIZE);
	json_decref(jobj);
	json_decref(jroot);

	/* send the return info to jrpcd */
	sockfd = get_sockfd();
	if(sockfd < 0) {
		LOG_ERR("%s", "Error: jrpc_rcall cannot be completed!");
		return -1;
	}
	write(sockfd, sendbuf, BUFF_SIZE);

	return 0;
}


/******************************************************************************
 * jrpc_call
 *
 * This function converts local function call into a remote call by translating
 * the information into a json formatted buffer and transmit the same to jrpc
 * daemon process.
 */
int jrpc_call(char *node, char *if_name, void *ret, char *afmt, ...)
{
	int retval, sockfd;
	va_list ap; /* var argument pointer */
	char *p;
	char buffer[BUFF_SIZE];
	char rfmt[NAME_SIZE];
	int retry_cnt;

	json_t *jroot;
	json_t *jarray;
	json_t *jrow;

	struct timespec ts;

	/* wait till libjrpc is initialized */
	retry_cnt = 10;
	while (ClientState != JRPC_INITIALISED) {
		usleep(100*1000);
		if (retry_cnt-- <= 0)
			break;
	}

	/* translate the call info to json format */
	jroot = json_object();
	ej_add_string(&jroot, "api", "call");
	ej_add_string(&jroot, "snode", ThisNode.name);
	ej_add_string(&jroot, "dnode", node);
	ej_add_string(&jroot, "if", if_name);
	jarray = json_array();
	json_object_set(jroot, "args", jarray);

	retval = 0;
	va_start(ap, afmt); /* make ap to point 1st unamed arg */
	for (p = afmt; *p; p++) {
		if (*p != '%')
			continue;

		jrow = json_object();

		switch(*++p) {
		case 'd':
			ej_add_string(&jrow, "type", "%d");
			ej_add_int(&jrow, "val", va_arg(ap, int));
			break;
		case 's':
			ej_add_string(&jrow, "type", "%s");
			ej_add_string(&jrow, "val", va_arg(ap, char *));
			break;
		default:
			LOG_ERR("%s", "Error: unsupported argument type");
			retval = -1;
			break;
		}

		json_array_append(jarray, jrow);
		json_decref(jrow);
	}
	va_end(ap);
	ej_store_buf(jroot, buffer, BUFF_SIZE);
	json_decref(jarray);
	json_decref(jroot);

	/* send the translated call info to jrpcd */
	sockfd = get_sockfd();
	if(sockfd < 0) {
		LOG_ERR("%s", "Error: jrpc_call cannot be completed!");
		return -1;
	}
	write(sockfd, buffer, BUFF_SIZE);

	/* wait for the call to return and process the ret val */
	(void) pthread_mutex_lock(&rcall_mutex);
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += 5;
	pthread_cond_timedwait(&rcall_condvar, &rcall_mutex, &ts);
	(void) pthread_mutex_unlock(&rcall_mutex);

	/* get the json structure from remote node and check for errors */
	jroot = get_calljson();
	if (jroot == NULL) {
		LOG_ERR("%s", "call-json pointer is NULL");
		goto error;
	}
	jrow = json_object_get(jroot, "ret");
	if (jrow == NULL) {
		LOG_ERR("%s", "can't get return structure");
		goto error;
	}

	/* get return value from json and check for errors */
	ej_get_string(jrow, "type", rfmt);
	if ((retval == -1) || (rfmt == NULL) || (rfmt[0] != '%')) {
		LOG_ERR("%s", "error! check arg and ret formats");
		LOG_VERBOSE("if_name: %s, afmt = %s, rfmt = %s", if_name, afmt,
			    rfmt);
		*((int*)ret) = 0;
		json_decref(jrow);
		goto error;
	}

	/* copy result to return argument from the caller */
	if ((rfmt[0] == '%') && (rfmt[1] == 'd'))
		ej_get_int(jrow, "val", ((int*)ret));
	else
		ej_get_string(jrow, "val", ((char*)ret));

	/* clean up and notify */
	json_decref(jrow);
	pthread_cond_signal(&ret_condvar);

	return 0;

error:
	pthread_cond_signal(&ret_condvar);
	return -1;
}


/****************************************************************************** 
 * jrpc_register
 *
 * This function converts the interface list to json formatted text via socket
 */
int jrpc_register(char *node, int n_if, struct if_details *ifl, void *cbptr)
{
	json_t *jroot;
	json_t *jarray;
	int size, i, sockfd;
	char buffer[BUFF_SIZE];
	int retry_cnt;

	if ( (node == NULL) || ((n_if > 0) && (ifl == NULL))) {
		LOG_ERR("%s", "Error: input pointers not correct");
		return -1;
	}

	/* wait till libjrpc is initialized */
	retry_cnt = 10;
	while (ClientState != JRPC_INITIALISED) {
		usleep(100*1000);
		if (retry_cnt-- <= 0)
			break;
	}

	/* convert if_details to json format */
	jroot = json_object();
	ej_add_string(&jroot, "api", "register");
	ej_add_string(&jroot, "snode", node);
	jarray = json_array();
	json_object_set(jroot, "interfaces", jarray);
	for (i = 0; i < n_if; i++) {
		json_t *jrow = json_object();

		ej_add_string(&jrow, "if", ifl[i].if_name);
		ej_add_string(&jrow, "arg", ifl[i].afmt);
		ej_add_string(&jrow, "ret", ifl[i].rfmt);

		json_array_append(jarray, jrow);
		json_decref(jrow);
	}
	ej_store_buf(jroot, buffer, BUFF_SIZE);
	json_decref(jarray);
	json_decref(jroot);

	/* send the json data to jrpcd daemon */
	sockfd = get_sockfd();
	if(sockfd < 0) {
		LOG_ERR("%s", "Error: jrpc_call cannot be completed!");
		return -1;
	}
	write(sockfd, buffer, BUFF_SIZE);

	/* take a copy of if_details to realize jrpc_rcall */
	size = n_if * sizeof(struct if_details);
	strcpy(ThisNode.name, node);
	ThisNode.n_if = n_if;
	if (n_if > 0)
		ThisNode.ifl = malloc(size);
	else
		ThisNode.ifl = NULL;
	memcpy(ThisNode.ifl, ifl, size);

	return 0;
}


/******************************************************************************
 * jrpc_rx_thread
 *
 * This function is created after init. Will send and receive messages from or
 * to jrpc server / daemon
 */
void * jrpc_rx_thread(void *arg)
{
	json_t *jroot;
	int sockfd, len;
	char buffer[BUFF_SIZE];
	char token[NAME_SIZE];

	struct timespec ts;

	sockfd = *((int *)arg);
	LOG_VERBOSE("%s %d", "wait for messages from server socket ", sockfd);
	RxThreadState = JRPC_INITIALISED;
	while (ClientState >= JRPC_CONNECTED) {
		len = read(sockfd, buffer, BUFF_SIZE);
		if (len < 0) {
			LOG_ERR("%s", "received error message, retrying...");
			continue;
		};
		LOG_VERBOSE("%s", "received a message...");

		/* at this point it is expected that the buffer contains a valid
		 * message from jrpcd in json format */
		jroot = json_object();
		ej_load_buf(buffer, &jroot);
		ej_get_string(jroot, "api", token);

		/* check for valid api */
		if (strcmp(token, "call") == 0) {
			LOG_VERBOSE("%s", "invoking remote call");
			jrpc_rcall(buffer);
		}
		else if (strcmp(token, "return") == 0) {
			JMsgCall = jroot; // jrpc_call will use this!!
			LOG_VERBOSE("%s", "handling return of prev call\n");
			pthread_cond_signal(&rcall_condvar);

			/* wait for the caller to read the return value */
			(void) pthread_mutex_lock(&ret_mutex);
			clock_gettime(CLOCK_REALTIME, &ts);
			ts.tv_sec += 5;
			pthread_cond_timedwait(&ret_condvar, &ret_mutex, &ts);
			(void) pthread_mutex_unlock(&ret_mutex);
			JMsgCall = NULL;
		}
		else if (strcmp(token, "ack") == 0) {
			LOG_VERBOSE("%s", "acknowledgment for prev message");
		}
		else {
			LOG_ERR("%s", "received an invalid message");
		}

		json_decref(jroot);
	}
	RxThreadState = JRPC_OFF;

	return NULL;
}


/****************************************************************************** 
 * jrpc_init
 *
 * This function initializes local variables and establish connection with 
 * jrpc daemon process.
 */
int jrpc_init(void)
{
	int sockfd, status;
	struct sockaddr_in servaddr;
	pthread_attr_t attr;
	int port;
	char ip[64];
	int retry_cnt;

	if (ClientState != JRPC_OFF) {
		LOG_ERR("%s", "Error: jrpc_init shall be called only once");
		return -1;
	}
	ClientState = JRPC_CONNECTING;

	/* create a TCP socket for connecting to jrpcd */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		LOG_ERR("%s", "Socket error");
		goto error;
	}

	SockFd = sockfd;
	port = DEFAULT_PORT;
	strcpy(ip, DEFAULT_IP);

        bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	status = inet_pton(AF_INET, ip, &servaddr.sin_addr);
	if (status <= 0) {
		LOG_ERR("%s", "inet_pton error");
		goto error;
	}

	/* Connect to jrpcd */
	status = connect(sockfd, (const struct sockaddr *) &servaddr,
			 sizeof(servaddr));
	if (status < 0) {
		LOG_ERR("%s", "Error: connect failed!");
		goto error;
	}
	ClientState = JRPC_CONNECTED;

	/* Initialize mutex and condition variable objects */
	pthread_mutex_init(&rcall_mutex, NULL);
	pthread_cond_init(&rcall_condvar, NULL);

	/* Create a thread to manage the connection */
	status = pthread_attr_init(&attr);
	if (status != 0) {
		LOG_ERR("%s", "Error: unable to init thread attr");
		goto error;
	}

	status = pthread_create(&RecvThread, &attr, jrpc_rx_thread, &sockfd);
	pthread_attr_destroy(&attr);
	if (status != 0) {
		LOG_ERR("%s", "Error: can't create receive thread!");
		goto error;
	}

	/* wait till rx thread is initialized */
	retry_cnt = 10;
	while (RxThreadState != JRPC_INITIALISED) {
		usleep(100*1000);
		if (retry_cnt-- <= 0)
			goto error;
	}
	ClientState = JRPC_INITIALISED;


	return 0;
error:
	SockFd = 0;
	ClientState = JRPC_OFF;
	return -1;
}
