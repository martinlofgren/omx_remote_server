#define _GNU_SOURCE // strcasestr()

#include <stdio.h>
#include <string.h>
#include <unistd.h> // write()
#include <stdlib.h> // malloc()
#include <ctype.h>
#include <regex.h>
#include <sys/types.h>

#include <ev.h>

#include "ev_sock.h"
#include "http.h"
#include "html.h"
#include "header_hashes.h"
#include "hash.h"
#include "ws.h"

/*
 * Regex definitions used to identify and HTTP messages and parse them.
 */
#define REGEX_REQUEST_LINE "([[:alpha:]]+)[[:space:]]*(/[.[:alnum:]|/]*)[[:space:]]*(HTTP/[0-9].[0-9])"
#define REGEX_HEADER_LINE "([[:alpha:]-]+)[[:space:]]*:[[:space:]]*(.*)"

/*
 * Macro: APPLY_REQ_LINE
 * --------------------
 * One big ugly macro which expands to a application of the given macro to
 * every request header field
 *
 * macro: the name of the macro to apply
 */
#define APPLY_REQ_LINE(macro)			\
  macro(ACCEPT);				\
  macro(ACCEPT_CHARSET);			\
  macro(ACCEPT_ENCODING);			\
  macro(ACCEPT_LANGUAGE);			\
  macro(ACCEPT_DATETIME);			\
  macro(AUTHORIZATION);				\
  macro(CACHE_CONTROL);				\
  macro(CONNECTION);				\
  macro(COOKIE);				\
  macro(CONTENT_LENGTH);			\
  macro(CONTENT_MD5);				\
  macro(CONTENT_TYPE);				\
  macro(DATE);					\
  macro(EXPECT);				\
  macro(FORWARDED);				\
  macro(FROM);					\
  macro(HOST);					\
  macro(IF_MATCH);				\
  macro(IF_MODIFIED_SINCE);			\
  macro(IF_NONE_MATCH);				\
  macro(IF_RANGE);				\
  macro(IF_UNMODIFIED_SINCE);			\
  macro(MAX_FORWARDS);				\
  macro(ORIGIN);				\
  macro(PRAGMA);				\
  macro(PROXY_AUTHORIZATION);			\
  macro(RANGE);					\
  macro(REFERER);				\
  macro(TE);					\
  macro(USER_AGENT);				\
  macro(UPGRADE);				\
  macro(VIA);					\
  macro(WARNING);				\
  macro(SEC_WEBSOCKET_PROTOCOL);		\
  macro(SEC_WEBSOCKET_KEY)
  
#define BUILD_HEADER_LINE(field)					\
  case HTTP ## _ ## field ## _ ## HASH:					\
  req->header.HTTP ## _ ## field ## _ ## NAME = strndup(OFFSET((char*)buf, 2), LEN(2) - 2); \
  break
#define CLEAN_HEADER_LINE(field)			\
  if (request.header.HTTP ## _ ## field ## _ ## NAME)		\
    free(request.header.HTTP ## _ ## field ## _ ## NAME)
#define OFFSET(str, n) str + match[n].rm_so
#define LEN(n) match[n].rm_eo - match[n].rm_so

#define DEBUG

regex_t request_line, header;

/*
 * Function: http_setup
 * --------------------
 * Initialize http module. Should be called before any other functions defined
 * in this file.
 */
void http_setup() {
  regcomp(&request_line, REGEX_REQUEST_LINE, REG_EXTENDED);
  regcomp(&header, REGEX_HEADER_LINE, REG_EXTENDED);
}

/*
 * Function: http_cleanup
 * ----------------------
 * Clean up when http resources is not needed.
 */
void http_cleanup () {
  regfree(&request_line);
  regfree(&header);
}

/*
 * Function: is_http_connection
 * ----------------------------
 * Check if the first line of a string seems to be a HTTP request line (e.g.
 * "GET / HTTP1.1").
 *
 * msg: the string to be matched against
 *
 * Returns: 1 if match, 0 otherwise.
 */
int is_http_connection(const char* msg) {
  return ((regexec(&request_line, msg, 0, NULL, 0)) == 0) ? 1 : 0;
}

/*
 * Function: locase
 * ----------------
 * Helper function for http_parse to get the lowercase version of a string.
 *
 * str: original string
 * ret: lowercase string
 */
static void locase(const char* str, char* ret) {
  while ((*ret++ = isupper(*str) ? tolower(*str) : *str))
    str++;
  *ret = '\0';
}

/*
 * Function: http_parse
 * --------------------
 * Parse a string and try to populate a request structure with the result.
 *
 * req: the request structure to populate. The request line and the header
 *      fields will be malloced via strndup, and must be freed by the
 *      callse.
 * msg: the string to parse
 */

#define BUFSIZE 1024
#define BUILD_REQ_LINE(post, n) req->request_line.post = strndup(OFFSET(msg, n), LEN(n))

static void http_parse(http_request* req, const char* msg) {
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
  
  char
    *buf = malloc(BUFSIZE * sizeof(char)),
    *tmpbuf = malloc(80 * sizeof(char)),
    lostr[80];
  size_t n = BUFSIZE;
  ssize_t len;

  while ((len = getline(&buf, &n, stream)) != -1) {
    if ((ret = regexec(&header, buf, 3, match, 0)) == 0) {
      tmpbuf = strndup(OFFSET(buf, 1), LEN(1));
      locase(tmpbuf, lostr);
      switch (hash(lostr)) {
	APPLY_REQ_LINE(BUILD_HEADER_LINE);
      }
    }
  }
  free(buf);
  free(tmpbuf);
  fclose(stream);

#ifdef DEBUG
  printf("\n---[ request parse results ]---\n");
  printf("Method: %s\nURI: %s\nVersion: %s\n",
	  req->request_line.method,
	  req->request_line.uri,
	  req->request_line.version);

#define PRINT_DEBUG(str)						\
  if ( req->header.HTTP ## _ ## str ## _ ## NAME )			\
    printf(#str": %s\n", req->header.HTTP ## _ ## str ## _ ## NAME )
  APPLY_REQ_LINE(PRINT_DEBUG);
  printf("---[ end request parse results ]---\n\n");
#endif
}

/*
 * Function: http_respond
 * ----------------------
 * Build the HTTP response based on the request.
 *
 * res: the response structure to populate. Some of the fields have been
 *      allocated dynamicly and must be freed by the caller.
 * req: the request structure to react on.
 */
static void http_respond(http_response* res, http_request* req) {
  if (!strcasecmp("GET", req->request_line.method)) {
    if (!strcmp("/", req->request_line.uri)) {
      res->status_line.version = "HTTP/1.1";
      res->status_line.status = "200 OK";
      res->body = (char*) www_client;
      res->headers.content_type = "text/html";
      res->headers.content_length = strlen(res->body);
    }
    else if (!strncmp("/ws", req->request_line.uri, 3)) {
      res->status_line.version = "HTTP/1.1";
      res->status_line.status = "101 Switching Protocols";
      res->headers.upgrade = "websocket";
      res->headers.connection = "Upgrade";
      char* accept = ws_accept_string(req->header.sec_websocket_key);
      res->headers.sec_websocket_accept = accept;
    }
    else {
      res->status_line.version = "HTTP/1.1";
      res->status_line.status = "404 Not Found";
    }
  }
  else {
    res->status_line.version = "HTTP/1.1";
    res->status_line.status = "501 Not Implemented";
  }
}

/*
 * Function: http_create_respond_msg
 * ---------------------------------
 * Create the actual message which can be send to the client.
 *
 * TODO: Does send the message, should return instead and let the caller
 *       send.
 *
 * w: SHOULD NOT NEED THIS
 * res: the response structure to base the message on.
 *
 * Returns: SHOULD RETURN STRING CONTAINING THE RESPONSE MSG
 */
static void http_create_response_msg(ev_sock *w, http_response* res) {
  int pos = 0;

  int str_len = strlen(res->status_line.version) +  strlen(res->status_line.status);
  if (res->body)
    str_len += strlen(res->body) + strlen(res->headers.content_type) + 10;
#ifdef DEBUG
  printf("str_len: %d\n", str_len);
#endif

  char *buf = malloc(4096 * sizeof(char));  // Fix calculation of message length!
  //char *buf = malloc(str_len * sizeof(char)); <- Should look like this, with correct str_len

  pos = sprintf(buf, "%s %s\r\n", res->status_line.version, res->status_line.status);

  /* This macro doesn't work now because content_length is an integer. Fix!

#define BUILD_HEADER(TYPE, STRING)					\
  if (res->headers.TYPE)						\
    pos += sprintf(buf+pos, STRING ": %s\r\n", res->headers.TYPE)

  BUILD_HEADER(content_type, "Content-Type");
  BUILD_HEADER(content_length, "Content-Length");
  BUILD_HEADER(upgrade, "Upgrade");
  BUILD_HEADER(connection, "Connection");
  BUILD_HEADER(sec_websocket_accept, "Sec-WebSocket-Accept");
  */
  
  if (res->headers.content_type)
    pos += sprintf(buf+pos, "Content-Type: %s\r\n", res->headers.content_type);
  
  if (res->headers.content_length)
    pos += sprintf(buf+pos, "Content-Length: %d\r\n", res->headers.content_length);

  if (res->headers.upgrade)
    pos += sprintf(buf+pos, "Upgrade: %s\r\n", res->headers.upgrade);
  
  if (res->headers.connection)
    pos += sprintf(buf+pos, "Connection: %s\r\n", res->headers.connection);
  
  if (res->headers.sec_websocket_accept)
    pos += sprintf(buf+pos, "Sec-WebSocket-Accept: %s\r\n", res->headers.sec_websocket_accept);

  pos += sprintf(buf+pos, "\r\n");
  
  if (res->body)
    pos += sprintf(buf+pos, "%s\r\n", res->body);

  
#ifdef DEBUG
  printf("pos: %d\n", pos);
#endif
  
#ifdef DEBUG
  printf("\n---[ sent response ]---\n%s---[end send response]---\n\n", buf);
#endif
  write(w->io.fd, buf, (size_t) pos);
  free(buf);
}

/*
 * Function: http_client
 * ---------------------
 * Main function responsible for a http connection. 
 *
 * w: the sock structure called by the event loop, containing relevant file
 *    descriptor for communication.
 * msg: the message read from the socket
 * len: length of msg
 */
void http_client(ev_sock *w, const char *msg, const int len) {
#ifdef DEBUG
  puts("++++++[ enter http_client ]++++++");
#endif
  
  http_request request;
  http_response response;

  memset(&request, 0, sizeof(http_request));
  memset(&response, 0, sizeof(http_response));

  http_parse(&request, msg);
  http_respond(&response, &request);
  http_create_response_msg(w, &response);

  if (response.headers.sec_websocket_accept) {
    puts("UPGRADE TO WEBSOCKET");
    w->msg_consumer = ws_client;
  }
  
  // Clean up
  APPLY_REQ_LINE(CLEAN_HEADER_LINE);
  free(response.headers.sec_websocket_accept);
  
#ifdef DEBUG
  puts("++++++[ exit http_client ]++++++");
#endif
}

