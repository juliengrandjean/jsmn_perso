#pragma once

#include <stddef.h>
#include <stdbool.h>
#include "jsmn.h"

char * json_fetch_url(char *url);
char * json_fetch_file(char *path);
jsmntok_t * json_tokenise(char *js);
bool json_token_streq(char *js, jsmntok_t *t, char *s);
char * json_token_tostr(char *js, jsmntok_t *t);
