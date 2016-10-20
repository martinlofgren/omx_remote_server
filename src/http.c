#define _GNU_SOURCE // strcasestr()

#include <stdio.h>
#include <string.h>
#include <unistd.h> // write()
#include <sys/types.h>
#include <ev.h>

#include "ev_sock.h"
#include "http.h"

int is_http_connection(const char* msg) {
  return (strcasestr(msg, "http/")) ? 1 : 0;
}

static void parse_http(http_request* req, const char* msg) {
  
}

static void respond_http(http_response* res, http_request* req) {

}

void http_init(ev_sock *w, const char *msg, const int len) {
#ifdef DEBUG
  puts("Setting upp http connection");
#endif

  http_request* request;
  parse_http(request, msg);

  http_response* response;
  respond_http(response, request);
  
  // Testing thingies only...
  char buf[1024];
  sprintf(buf, "HTTP/1.1 100 Continue\n\nHTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length: 10\n\nHej Kanin!\n");
  printf("%s", buf);
  write(w->io.fd, buf, (size_t) len);
}

