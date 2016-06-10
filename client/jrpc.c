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

struct node_details {
	char name[NAME_SIZE];
	int n_if;
	struct if_details *ifl;
};

/******************************************************************************
 *  global variables
 */
pthread_t RecvThread;
enum jrpc_states ClientState;
int SockFd;
struct node_details ThisNode;
json_t *JMsg;

pthread_mutex_t rcall_mutex;
pthread_cond_t rcall_condvar;
pthread_mutex_t return_mutex;
pthread_cond_t return_condvar;

/******************************************************************************
 * static functions
 */
static int get_sockfd(void)
{
	if (ClientState != JRPC_CONNECTED)
		return -1;
	else
		return SockFd;
}


static json_t* get_msgjson(void)
{
	return JMsg;
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
	jmsg = get_msgjson();
	jarray = json_object_get(jmsg, "args");
	if ((jarray == NULL) || (0 == (argc = json_array_size(jarray))))
	{
		printf("%s(), no array element \'ecus\' found\n", __func__);
		return -1;
	}


	va_start(ap, fmt); /* make ap to point 1st unamed arg */
	for (i = c = 0, p = fmt; *p; p++, c++) {
		if (*p != '%') {
			printf("%s(): check format string @ %d\n", __func__, c);
			continue;
		}

		jrow = json_array_get(jarray, i++);
		if (!json_is_object(jrow)) {
			printf("%s(), json array access failure\n", __func__);
			return -1;
		}
		ej_get_string(jrow, "type", type);

		switch (*++p) {
		case 'd':
			/* check if types requested and received are matching */
			if ((type[0] != '%') || (type[1] != 'd')) {
			       printf("%s(): arg %d type error!\n", __func__,i);
			       return -1;
			}
			ej_get_int(jrow, "val", va_arg(ap, int *));
			break;
		case 's':
			/* check if types requested and received are matching */
			if ((type[0] != '%') || (type[1] != 's')) {
			       printf("%s(): arg %d type error!\n", __func__,i);
			       return -1;
			}
			ej_get_string(jrow, "val", va_arg(ap, char *));
			break;
		default:
			printf("Error: unsupported argument type\n");
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
	pthread_mutex_destroy(&return_mutex);
	pthread_cond_destroy(&return_condvar);

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
	char *rfmt;
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
			JMsg = jroot; // note: consumed by jrpc_scanargs()
			retval = fnptr(result, ThisNode.ifl[i].afmt);
			JMsg = NULL;
			rfmt = ThisNode.ifl[i].rfmt;
			break;
		}
	}
	json_decref(jroot);
	if ((retval < 0) || (result == NULL)) {
		printf("%s(): rcall failed!\n", __func__);
		printf("retval %d, result %p\n", retval, result);
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
		printf("Error: jrpc_rcall cannot be completed!\n");
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
	int retval, sockfd, i;
	va_list ap; /* var argument pointer */
	char *p;
	char buffer[BUFF_SIZE];
	char token[NAME_SIZE];
	char rfmt[NAME_SIZE];

	json_t *jroot;
	json_t *jarray;
	json_t *jrow;

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
			printf("Error: unsupported argument type\n");
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
		printf("Error: jrpc_call cannot be completed!\n");
		return -1;
	}
	write(sockfd, buffer, BUFF_SIZE);

#if JRPC_UNIT_TESTING
	char dummy[] = "{\"if\": \"add\", \"api\": \"return\", \"dnode\": \"this\", \"snode\": \"this\", \"ret\": {\"type\": \"%d\", \"val\": 11}}";
	jroot = json_object();
	ej_load_buf(dummy, &jroot);
#else
	/* wait for the call to return and process the ret val */
	pthread_cond_wait(&rcall_condvar, &rcall_mutex);
	jroot = get_msgjson();
#endif
	jrow = json_object_get(jroot, "ret");
	ej_get_string(jrow, "type", token);

	/* validate return types */
	for (i = 0, rfmt[i] = '\0'; i < ThisNode.n_if; i++) {
		if (strcmp(if_name, ThisNode.ifl[i].if_name) == 0) {
			strcpy(rfmt, ThisNode.ifl[i].rfmt);
			break;
		}
	}

	/* check validation results, previous errors and set ret value */
	if ((retval == -1) || (rfmt[i] == '\0') || (strcmp(rfmt, token) != 0)) {
		printf("%s(): error! check arg and ret formats\n",  __func__);
		*((int*)ret) = 0;
		json_decref(jrow);
		return -1;
	}
	if ((rfmt[0] == '%') && (rfmt[1] == 'd'))
		ej_get_int(jrow, "val", ((int*)ret));
	else
		ej_get_string(jrow, "val", ((char*)ret));

	/* clean up and notify */
	json_decref(jrow);
	pthread_cond_signal(&return_condvar);

	return 0;
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

	if ( (node == NULL) || ((n_if > 0) && (ifl == NULL))) {
		printf("Error: input pointers not correct\n");
		return -1;
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
		printf("Error: jrpc_call cannot be completed!\n");
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

	sockfd = *((int *)arg);
	while (ClientState == JRPC_CONNECTED) {
		len = read(sockfd, buffer, BUFF_SIZE);
		if (len < 0) {
			printf("%s(): received error message, retrying...\n",
			       __func__);
			continue;
		};

		/* at this point it is expected that the buffer contains a valid
		 * message from jrpcd in json format */
		jroot = json_object();
		ej_load_buf(buffer, &jroot);
		ej_get_string(jroot, "api", token);

		/* check for valid api */
		if (strcmp(token, "call") == 0) {
			jrpc_rcall(buffer);
		}
		else if (strcmp(token, "return") == 0) {
			pthread_cond_signal(&rcall_condvar);
			JMsg = jroot;
			pthread_cond_wait(&return_condvar, &return_mutex);
			JMsg = NULL;
		}
		else {
			printf("%s(): received an invalid message\n", __func__);
		}

		json_decref(jroot);
	}

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

	if (ClientState != JRPC_OFF) {
		printf("Error: jrpc_init shall be called only once\n");
		return -1;
	}
	ClientState = JRPC_CONNECTING;

	/* create a TCP socket for connecting to jrpcd */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		printf("Socket error\n");
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
		printf("inet_pton error\n");
		goto error;
	}

	/* Connect to jrpcd */
	status = connect(sockfd, (const struct sockaddr *) &servaddr,
			 sizeof(servaddr));
	if (status < 0) {
		printf("\nError: connect failed!\n");
		goto error;
	}
	ClientState = JRPC_CONNECTED;

	/* Initialize mutex and condition variable objects */
	pthread_mutex_init(&rcall_mutex, NULL);
	pthread_cond_init(&rcall_condvar, NULL);

	/* Create a thread to manage the connection */
	status = pthread_attr_init(&attr);
	if (status != 0) {
		printf("\nError: unable to init thread attr\n");
		goto error;
	}

	status = pthread_create(&RecvThread, &attr, jrpc_rx_thread, &sockfd);
	pthread_attr_destroy(&attr);
	if (status != 0) {
		printf("\nError: can't create receive thread!\n");
		goto error;
	}


	return 0;
error:
	SockFd = 0;
	ClientState = JRPC_OFF;
	return -1;
}
