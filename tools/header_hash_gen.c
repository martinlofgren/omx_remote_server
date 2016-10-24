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
  "warning"
};

static unsigned long sdbm(unsigned char* str) {
  unsigned long hash = 0;
  int c;
  
  while (c = *str++)
    hash = c + (hash << 6) + (hash << 16) - hash;
  
  return hash;
}

void ucase(char* str, char* ret) {
  while (*ret++ = islower(*str) ? toupper(*str) : *str)
    str++;
  *ret = '\0';
}

int main () {
  int i, j;
  char buf[1024];

  for (i = 0; headers[i]; i++) {
    for (j = i + 1 ; headers[j]; j++) {
      if (headers[i] == headers[j]) {
	printf("Collision: %s and %s share the same hash!\n", headers[i], headers[j]);
	exit(EXIT_FAILURE);
      }
    }
  }
  
  for (i = 0; headers[i]; i++) {
    ucase(headers[i], buf);
    printf("#define %s %lu\n", buf, sdbm(headers[i]));
  }
}

