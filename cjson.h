#ifndef CJSON_H
#define CJSON_H

#include <stdint.h>
#include <stdlib.h>

#define cj_malloc(size) malloc(size)
#define cj_realloc(ptr, size) realloc(ptr, size)
#define cj_free(ptr) free(ptr)

#define CJ_TYPE_OBJECT 1
#define CJ_TYPE_ARRAY  2
#define CJ_TYPE_STRING 3
#define CJ_TYPE_NUMBER 4
#define CJ_TYPE_TRUE   5
#define CJ_TYPE_FALSE  6
#define CJ_TYPE_NULL   7

typedef struct cj_string cj_string;
typedef struct cj_value cj_value;

struct cj_string {
  uint64_t len;
  char data[];
};

struct cj_value {
  int type;
  cj_string *name;
  union {
    cj_value *members;
    cj_value *elements;
    cj_string *string;
    double number;
  } value;
  cj_value *next;
};

cj_value *cj_parse(const char *text, char **end);

void cj_clean(cj_value *value);

char *cj_stringify(cj_value *value, uint64_t *len);

#endif
