#define _GNU_SOURCE // strcasestr()

#include <stdio.h>
#include <string.h>
#include <unistd.h> // write()
#include <stdlib.h> // malloc()
#include <ctype.h>
#include <sys/types.h>
#include <ev.h>
#include <regex.h>

#include "ev_sock.h"
#include "http.h"
#include "html.h"
#include "header_hashes.h"

#define REGEX_REQUEST_LINE "([[:alpha:]]+)[[:space:]]*(/[[:alnum:]|/]*)[[:space:]]*(HTTP/[0-9].[0-9])"
#define REGEX_HEADER_UPGRADE "Upgrade:[[:space:]]*(.*)"
#define REGEX_HEADER_SEC_WEBSOCKET_KEY "Sec-WebSocket-Key:[[:space:]]*(.*)"
#define REGEX_HEADER_LINE "([[:alpha:]-]+)[[:space:]]*:[[:space:]]*(.*)"

#define BUILD_REQ_LINE(post, n) req->request_line.post = strndup(OFFSET(msg, n), LEN(n))
#define BUILD_HEADER_LINE(head)						\
  case HTTP ## _ ## head ## _ ## HASH:					\
  req->header.HTTP ## _ ## head ## _ ## NAME = strndup(OFFSET(buf, 2), LEN(2) - 1); \
  break
#define OFFSET(str, n) str + match[n].rm_so
#define LEN(n) match[n].rm_eo - match[n].rm_so

#define DEBUG

regex_t request_line, header;

// http://www.cse.yorku.ca/~oz/hash.html
unsigned int hash(char *str) {
  unsigned int hash = 5381;
  int c;

  while ((c = *str++))
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  
  return hash % 4096;
}

void http_setup() {
  regcomp(&request_line, REGEX_REQUEST_LINE, REG_EXTENDED);  // <-- Malloc failing!
  regcomp(&header, REGEX_HEADER_LINE, REG_EXTENDED);
}

void http_cleanup () {
  regfree(&request_line);
  regfree(&header);
}

int is_http_connection(const char* msg) {
  return ((regexec(&request_line, msg, 0, NULL, 0)) == 0) ? 1 : 0;
}

void locase(char* str, char* ret) {
  while ((*ret++ = isupper(*str) ? tolower(*str) : *str))
    str++;
  *ret = '\0';
}

static void parse_http(http_request* req, const char* msg) {
  regmatch_t match[4];
  int ret;
  
  FILE *stream;
  stream = fmemopen((char*) msg, strlen(msg), "r");
  
  // Parse request line (i.e. GET / HTTP/1.1)
  if ((ret = regexec(&request_line, msg, 4, match, 0)) == 0) {
    BUILD_REQ_LINE(method, 1);
    BUILD_REQ_LINE(uri, 2);
    BUILD_REQ_LINE(version, 3);
  }

  
#define BUFSIZE 1024
  puts("parse_1");
  char *buf = malloc(BUFSIZE * sizeof(char));
  puts("parse_2");
  char *tmpbuf = malloc(80 * sizeof(char));
  puts("parse_3");
  char lostr[80];
  puts("parse_4");
  size_t n = BUFSIZE;
  ssize_t len;

  while ((len = getline(&buf, &n, stream)) != -1) {
    if ((ret = regexec(&header, buf, 3, match, 0)) == 0) {
      tmpbuf = strndup(OFFSET(buf, 1), LEN(1));
      locase(tmpbuf, lostr);
      switch (hash(lostr)) {
	BUILD_HEADER_LINE(ACCEPT);
	BUILD_HEADER_LINE(ACCEPT_CHARSET);
	BUILD_HEADER_LINE(ACCEPT_ENCODING);
	BUILD_HEADER_LINE(ACCEPT_LANGUAGE);
	BUILD_HEADER_LINE(ACCEPT_DATETIME);
	BUILD_HEADER_LINE(AUTHORIZATION);
	BUILD_HEADER_LINE(CACHE_CONTROL);
	BUILD_HEADER_LINE(CONNECTION);
	BUILD_HEADER_LINE(COOKIE);
	BUILD_HEADER_LINE(CONTENT_LENGTH);
	BUILD_HEADER_LINE(CONTENT_MD5);
	BUILD_HEADER_LINE(CONTENT_TYPE);
	BUILD_HEADER_LINE(DATE);
	BUILD_HEADER_LINE(EXPECT);
	BUILD_HEADER_LINE(FORWARDED);
	BUILD_HEADER_LINE(FROM);
	BUILD_HEADER_LINE(HOST);
	BUILD_HEADER_LINE(IF_MATCH);
	BUILD_HEADER_LINE(IF_MODIFIED_SINCE);
	BUILD_HEADER_LINE(IF_NONE_MATCH);
	BUILD_HEADER_LINE(IF_RANGE);
	BUILD_HEADER_LINE(IF_UNMODIFIED_SINCE);
	BUILD_HEADER_LINE(MAX_FORWARDS);
	BUILD_HEADER_LINE(ORIGIN);
	BUILD_HEADER_LINE(PRAGMA);
	BUILD_HEADER_LINE(PROXY_AUTHORIZATION);
	BUILD_HEADER_LINE(RANGE);
	BUILD_HEADER_LINE(REFERER);
	BUILD_HEADER_LINE(TE);
	BUILD_HEADER_LINE(USER_AGENT);
	BUILD_HEADER_LINE(UPGRADE);
	BUILD_HEADER_LINE(VIA);
	BUILD_HEADER_LINE(WARNING);
      }
    }
  }
  free(buf);
  free(tmpbuf);
  fclose(stream);

#ifdef DEBUG
  printf("---[ request parse results ]---\n");
  printf("Method: %s\nURI: %s\nVersion: %s\n",
	  req->request_line.method,
	  req->request_line.uri,
	  req->request_line.version);

#define PRINT_DEBUG(str)						\
  if ( req->header.HTTP ## _ ## str ## _ ## NAME )			\
    printf(#str": %s\n", req->header.HTTP ## _ ## str ## _ ## NAME )
  
  PRINT_DEBUG(ACCEPT);
  PRINT_DEBUG(ACCEPT_CHARSET);
  PRINT_DEBUG(ACCEPT_ENCODING);
  PRINT_DEBUG(ACCEPT_LANGUAGE);
  PRINT_DEBUG(ACCEPT_DATETIME);
  PRINT_DEBUG(AUTHORIZATION);
  PRINT_DEBUG(CACHE_CONTROL);
  PRINT_DEBUG(CONNECTION);
  PRINT_DEBUG(COOKIE);
  PRINT_DEBUG(CONTENT_LENGTH);
  PRINT_DEBUG(CONTENT_MD5);
  PRINT_DEBUG(CONTENT_TYPE);
  PRINT_DEBUG(DATE);
  PRINT_DEBUG(EXPECT);
  PRINT_DEBUG(FORWARDED);
  PRINT_DEBUG(FROM);
  PRINT_DEBUG(HOST);
  PRINT_DEBUG(IF_MATCH);
  PRINT_DEBUG(IF_MODIFIED_SINCE);
  PRINT_DEBUG(IF_NONE_MATCH);
  PRINT_DEBUG(IF_RANGE);
  PRINT_DEBUG(IF_UNMODIFIED_SINCE);
  PRINT_DEBUG(MAX_FORWARDS);
  PRINT_DEBUG(ORIGIN);
  PRINT_DEBUG(PRAGMA);
  PRINT_DEBUG(PROXY_AUTHORIZATION);
  PRINT_DEBUG(RANGE);
  PRINT_DEBUG(REFERER);
  PRINT_DEBUG(TE);
  PRINT_DEBUG(USER_AGENT);
  PRINT_DEBUG(UPGRADE);
  PRINT_DEBUG(VIA);
  PRINT_DEBUG(WARNING);
  printf("---[ end request parse results ]---\n");
#endif
}

	
static void http_100_continue(http_response* res) {
  res->status_line.version = "HTTP/1.1";
  res->status_line.status = "100 Continue";
}

static void respond_http(http_response* res, http_request* req) {
  res->status_line.version = "HTTP/1.1";
  res->status_line.status = "200 OK";
  res->body = (char*) http_client;
  res->headers.content_type = "text/html";
  res->headers.content_length = strlen(res->body);
}

void create_response_msg(ev_sock *w, http_response* res) {
  int pos=0;

  int str_len = strlen(res->status_line.version) +  strlen(res->status_line.status);
  if (res->body)
    str_len += strlen(res->body) + strlen(res->headers.content_type) + 10;
  printf("str_len: %d\n", str_len);

  char *buf = malloc(1024 * sizeof(char));  // Fix calculation of message length!
  //char *buf = malloc(str_len * sizeof(char));
  //char buf[4096];
  pos = sprintf(buf, "%s %s\n", res->status_line.version, res->status_line.status);
  
  if (res->headers.content_type)
    pos += sprintf(buf+pos, "Content-Type: %s\n", res->headers.content_type);
  
  if (res->headers.content_length)
    pos += sprintf(buf+pos, "Content-Length: %d\n", res->headers.content_length);
    pos += sprintf(buf+pos, "\n");

  if (res->body)
    pos += sprintf(buf+pos, "%s\n", res->body);

  printf("pos: %d\n", pos);
  
  printf("---[ sent response ]---\n%s---[end send response]---\n", buf);
  write(w->io.fd, buf, (size_t) pos);
  printf("before free(buf)\n");
  free(buf);
  printf("after free(buf)\n");
}

void http_init(ev_sock *w, const char *msg, const int len) {
#ifdef DEBUG
  puts("Setting up http connection");
#endif
  puts("Yipp");
  http_request request;
  puts("Yapp");
  memset(&request, 0, sizeof(http_request));
  puts("Yopp");
  parse_http(&request, msg);
  puts("Yäpp");

  http_response cont;
  memset(&cont, 0, sizeof(http_response));
  http_100_continue(&cont);
  
  http_response response;
  memset(&response, 0, sizeof(http_response));
  respond_http(&response, &request);

  //  create_response_msg(w, &cont);
  create_response_msg(w, &response);
}

