
/* 
 * header_hash_gen.c
 *
 * Quick n' dirty macro generation of hashes and strings for 
 * http requests.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char* headers[] = {
  "accept",
  "accept_charset",
  "accept_encoding",
  "accept_language",
  "accept_datetime",
  "authorization",
  "cache_control",
  "connection",
  "cookie",
  "content_length",
  "content_md5",
  "content_type",
  "date",
  "expect",
  "forwarded",
  "from",
  "host",
  "if_match",
  "if_modified_since",
  "if_none_match",
  "if_range",
  "if_unmodified_since",
  "max_forwards",
  "origin",
  "pragma",
  "proxy_authorization",
  "range",
  "referer",
  "te",
  "user_agent",
  "upgrade",
  "via",
  "warning",
  "sec_websocket_protocol",
  "sec_websocket_key"
};

unsigned int hash(char *str) {
  unsigned int hash = 5381;
  int c;

  while ((c = *str++))
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  
  return hash % 4096;
}

void ucase(char* str, char* ret) {
  while (*ret++ = islower(*str) ? toupper(*str) : *str)
    str++;
  *ret = '\0';
}

void up_line(char* str, char* ret) {
  while (*ret++ = '_' == *str ? '-' : *str)
    str++;
  *ret = '\0';
}

#define MAX_LINE_LEN 128

int main () {
  int i, j;
  char buf1[MAX_LINE_LEN], buf2[MAX_LINE_LEN];

  for (i = 0; headers[i]; i++) {
    up_line(headers[i], buf1);
    for (j = i + 1 ; headers[j]; j++) {
      up_line(headers[j], buf2);
      if (hash(buf1) == hash(buf2)) {
	printf("Collision: %s and %s share the same hash!\n", headers[i], headers[j]);
	exit(EXIT_FAILURE);
      }
    }
  }
  
  printf("// Header hashes\n");
  for (i = 0; headers[i]; i++) {
    ucase(headers[i], buf1);
    up_line(headers[i], buf2);
    printf("#define HTTP_%s_HASH %lu\n", buf1, hash(buf2));
  }

  printf("\n// Header struct names\n");
  
  for (i = 0; headers[i]; i++) {
    ucase(headers[i], buf1);
    printf("#define HTTP_%s_NAME %s\n", buf1, headers[i]);
  }
}

