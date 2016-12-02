
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
#include <ctype.h>

#include "../src/hash.h"

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
  "warning",
  "sec-websocket-protocol",
  "sec-websocket-key",
  0
};

char* endpoints[] = {
  "/media/title",
  "/media/url-stream",
  "/media/url-info",
  "/media/duration",
  "/player/status",
  "/player/position",
  0
};

char* methods[] = {
  "GET",
  "POST",
  0
};

void ucase(char* buf) {
  while ((*buf = islower(*buf) ? toupper(*buf) : *buf))
    buf++;
  *buf = '\0';
}

void subst(char* buf, char from, char to) {
  while ((*buf = (from == *buf) ? to : *buf))
    buf++;
  *buf = '\0';
}

void subst_all(char* buf) {
  while (*buf) {
    *buf = (isalnum(*buf)) ? *buf : '_';
    buf++;
  }
  *buf = '\0';
}

void remove_duplicates(char* buf, char c) {
  char* cp;
  cp = buf;
  int i;i=0;

  do {
    while (*(cp) == c && *(cp + 1) == c) {
      cp++;
    }
    
    *(buf++) = *(cp++);
  } while (*buf);
}

#define MAX_LINE_LEN 128

int detect_collision(char** strings, char* desc) {
  int i, j;
  char buf1[MAX_LINE_LEN], buf2[MAX_LINE_LEN];

  printf("[%s] detecting collisions... ", desc);

  for (i = 0; strings[i]; i++) {
    strcpy(buf1, strings[i]);
    subst(buf1, '_', '-');
    for (j = i + 1 ; strings[j]; j++) {
      strcpy(buf2, strings[j]);
      subst(buf2, '_', '-');
      if (hash(buf1) == hash(buf2)) {
	printf("Collision: %s and %s share the same hash!\n", strings[i], strings[j]);
	exit(EXIT_FAILURE);
      }
    }
  }
  printf("No collisions!\n");
  return 1;
}

void gen_hash(FILE* file, char** strings, char* desc) {
  int i;
  char buf[MAX_LINE_LEN];
  
  printf("[%s] Generating hashes... ", desc);
  fprintf(file, "// %s hashes\n", desc);
  for (i = 0; strings[i]; i++) {
    sprintf(buf, "%s_%s", desc, strings[i]);
    ucase(buf);
    subst_all(buf);
    remove_duplicates(buf, '_');
    fprintf(file, "#define %s_HASH %d\n", buf, hash(strings[i]));
  }
  fprintf(file, "\n");
  printf("Done!\n");
}

int main (int argc, char** argv) {
  if (argc != 2) {
    printf("Usage: hash_gen output\n");
    exit(EXIT_FAILURE);
  }

  detect_collision(endpoints, "Endpoint");
  detect_collision(headers, "Header");

  FILE* file;
  file = fopen(argv[1], "w");

  fprintf(file,
	  "#ifndef OMX_REMOTE_SERVER_MACROS_H\n"
	  "#define OMX_REMOTE_SERVER_MACROS_H\n\n");

  gen_hash(file, endpoints, "ENDPOINT");
  gen_hash(file, headers, "HEADER");

  char buf1[MAX_LINE_LEN], buf2[MAX_LINE_LEN];
  int i;
  
  printf("Generating header struct names\n");
  fprintf(file, "// Header struct names\n");
  for (i = 0; headers[i]; i++) {
    strcpy(buf1, headers[i]);
    strcpy(buf2, headers[i]);
    ucase(buf1);
    subst_all(buf1);
    subst_all(buf2);
    fprintf(file, "#define HEADER_%s_NAME %s\n", buf1, buf2);
  }
  
  
  fprintf(file,
	  "\n"
	  "#endif // OMX_REMOTE_SERVER_MACROS_H\n");
    
  fclose(file);

  return 0;
}

