#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char* headers[] = {
  "accept",
  "accept-charset",
  "accept-encoding",
  "accept-language",
  "accept-datetime",
  "authorization",
  "cache-control",
  "connection",
  "cookie",
  "content-length",
  "content-md5",
  "content-type",
  "date",
  "expect",
  "forwarded",
  "from",
  "host",
  "if-match",
  "if-modified-since",
  "if-none-match",
  "if-range",
  "if-unmodified-since",
  "max-forwards",
  "origin",
  "pragma",
  "proxy-authorization",
  "range",
  "referer",
  "te",
  "user-agent",
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

