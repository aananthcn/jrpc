/* 
 * Author: Aananth C N
 * email: c.n.aananth@gmail.com
 *
 * License: MPL v2
 * Date: 10 July 2015
 */

#ifndef EJSON_H
#define EJSON_H

#include <jansson.h>


#define BUFF_SIZE	(4*1024)
#define NAME_SIZE	(256)



/*    E A S Y   J S O N   A P I ' S   */
int ej_load_buf(char *buf, json_t **root);
int ej_store_buf(json_t *root, char *buf, int max);
int ej_load_file(char *file, json_t **root);
int ej_store_file(json_t *root, char *file);
int ej_get_int(json_t *root, char *name, int *value);
int ej_get_string(json_t *root, char *name, char *value);
int ej_set_int(json_t *root, char *name, int value);
int ej_set_string(json_t *root, char *name, char *value);
int ej_add_int(json_t **root, char *name, int value);
int ej_add_string(json_t **root, char *name, char *value);


#endif
