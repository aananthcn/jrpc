/* 
 * Author: Aananth C N
 * email: c.n.aananth@gmail.com
 *
 * License: MPL v2
 * Date: 10 July 2015
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <jansson.h>

#include "ejson.h"

/*                    E A S Y   J S O N   A P I ' S                     */

/*************************************************************************
 * function: ej_load_buf
 *
 * This function json formated text buffer into json structure so that
 * application can parse and get json values. 
 *
 * arg1: buffer pointer
 * arg2: json_t pointer reference
 * 
 * return: positive or negative number
 */
int ej_load_buf(char *buf, json_t **root)
{
	int flags = 0;

	json_error_t error;

	if(!buf) {
		printf("%s(), invalid arguments!\n", __func__);
		return -1;
	}

	*root = json_loads(buf, flags, &error);
	if(*root == NULL) {
		printf("error: %s\n", error.text);
		return -1;
	}

	return 0;
}



/*************************************************************************
 * function: ej_store_buf
 *
 * This function stores the encoded json structure into RAM buffer
 *
 * arg1: json object pointer
 * arg2: buffer pointer
 * arg3: max size of buffer
 * 
 * return: positive or negative number
 */
int ej_store_buf(json_t *root, char *buf, int max)
{
	if ((root == NULL) || (buf == NULL)) {
		printf("Error: %s(): invalid arguments\n", __func__);
		return -1;
	}

	strncpy(buf, json_dumps(root, 0), max);

	return 0;
}



/*************************************************************************
 * function: ej_load_file
 *
 * This function copies json file to RAM area, do necessary checks and 
 * pass back json_t* (pointer to the RAM area), to the caller
 *
 * arg1: file path of json file
 * arg2: json_t pointer reference
 * 
 * return: positive or negative number
 */
int ej_load_file(char *file, json_t **root)
{
	int flags = 0;

	json_error_t error;

	if(!file) {
		printf("%s(), invalid file passed!\n", __FUNCTION__);
		return -1;
	}

	*root = json_load_file(file, flags, &error);
	if(*root == NULL) {
		printf("error: %s, line %d: %s\n", file,
		       error.line, error.text);
		return -1;
	}

	return 0;
}



/*************************************************************************
 * function: ej_store_file
 *
 * This function stores the data in RAM pointed by json_t* to the storage
 * media path passed as argument
 *
 * arg1: file path
 * arg2: json_t pointer
 * 
 * return: positive or negative number
 */
int ej_store_file(json_t *root, char *file)
{
	int flags, ret;

	if(!file) {
		printf("%s(), invalid file passed!\n", __FUNCTION__);
		return -1;
	}

	/* dump to a temporary file */
	flags = JSON_INDENT(8);
	ret = json_dump_file(root, file, flags);
	if(ret < 0)
		return -1;

	return 0;
}



/*************************************************************************
 * function: ej_get_int
 *
 * This function gets the integer value from the json object
 *
 * return: positive or negative number
 */
int ej_get_int(json_t *root, char *name, int *value)
{
	json_t *obj;

	obj = json_object_get(root, name);
	if(obj == NULL) {
		*value = 0;
		return -1;
	}

	*value = json_integer_value(obj);
	return 0;
}



/*************************************************************************
 * function: ej_get_string
 *
 * This function gets the string value from the json object
 *
 * return: positive or negative number
 */
int ej_get_string(json_t *root, char *name, char *value)
{
	json_t *obj;

	if(!json_is_object(root)) {
		printf("%s(): invalid json arg passed\n", __FUNCTION__);
		return -1;
	}

	obj = json_object_get(root, name);
	if(obj == NULL) {
		*value = '\0';
		return -1;
	}

	strcpy(value, json_string_value(obj));
	return 0;
}



/*************************************************************************
 * function: ej_set_int
 *
 * This function sets the integer value to the json object
 *
 * return: positive or negative number
 */
int ej_set_int(json_t *root, char *name, int value)
{
	json_t *obj;

	obj = json_object_get(root, name);
	if(obj == NULL) {
		return -1;
	}

	return json_integer_set(obj, value);
}



/*************************************************************************
 * function: ej_set_string
 *
 * This function gets the string value to the json object
 *
 * return: positive or negative number
 */
int ej_set_string(json_t *root, char *name, char *value)
{
	json_t *obj;

	obj = json_object_get(root, name);
	if(obj == NULL) {
		return -1;
	}

	return json_string_set(obj, value);
}


int ej_add_int(json_t **root, char *name, int value)
{
	json_t *new;

	if (name == NULL) {
		printf("%s(): invalid arguments!\n", __func__);
		return -1;
	}

	new = json_pack("{\nsi\n}", name, value);
	if (new  == NULL)
		return -1;

	return json_object_update(*root, new);
}

int ej_add_string(json_t **root, char *name, char *value)
{
	json_t *new;

	if ((name == NULL) || (value == NULL)) {
		printf("%s(): invalid arguments!\n", __func__);
		return -1;
	}

	new = json_pack("{\nss\n}", name, value);
	if(new  == NULL)
		return -1;

	return json_object_update(*root, new);
}

