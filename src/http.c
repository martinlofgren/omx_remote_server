#define _GNU_SOURCE // strcasestr()

#include <stdio.h>
#include <string.h>
#include <unistd.h> // write()
#include <stdlib.h> // malloc()
#include <sys/types.h>
#include <ev.h>

#include "ev_sock.h"
#include "http.h"
#include "html.h"

int is_http_connection(const char* msg) {
  return (strcasestr(msg, "http/")) ? 1 : 0;
}

static void parse_http(http_request* req, const char* msg) {
  
}

static void http_100_continue(http_response* res) {
  res->status_line.version = "HTTP/1.1";
  res->status_line.status = "100 Continue";
}

static void respond_http(http_response* res, http_request* req) {
  res->status_line.version = "HTTP/1.1";
  res->status_line.status = "200 OK";
  res->body = (char*) www_index_html;
  res->headers.content_type = "text/html";
  res->headers.content_length = strlen(res->body);
}

static int create_response_msg(http_response* res, char** msg) {
  int pos=0;
  int str_len = strlen(res->status_line.version) +  strlen(res->status_line.status);
  if (res->body)
    str_len +=  strlen(res->body) + strlen(res->headers.content_type) + 10;
  char *buf = malloc(sizeof(char) * str_len);
  pos = sprintf(buf, "%s %s\n", res->status_line.version, res->status_line.status);
  if (res->headers.content_type)
    pos += sprintf(buf+pos, "Content-Type: %s\n", res->headers.content_type);
  if (res->headers.content_length)
    pos += sprintf(buf+pos, "Content-Length: %d\n", res->headers.content_length);
  pos += sprintf(buf+pos, "\n");
  if (res->body)
    pos += sprintf(buf+pos, "%s\n", res->body);
  *msg = buf;
  return pos;
}

void http_init(ev_sock *w, const char *msg, const int len) {
#ifdef DEBUG
  puts("Setting up http connection");
#endif
  http_request request;
  memset(&request, 0, sizeof(http_request));
  parse_http(&request, msg);

  http_response cont;
  memset(&cont, 0, sizeof(http_response));
  http_100_continue(&cont);
  
  http_response response;
  memset(&response, 0, sizeof(http_response));
  respond_http(&response, &request);

  char* buf;
  int leng;

  leng = create_response_msg(&cont, &buf);
  printf("%s",buf);
  write(w->io.fd, buf, (size_t) leng);

  leng = create_response_msg(&response, &buf);
  printf("%s",buf);
  write(w->io.fd, buf, (size_t) leng);
}

