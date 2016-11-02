#define _GNU_SOURCE // strcasestr()

#include <stdio.h>
#include <string.h>
#include <unistd.h> // write()
#include <stdlib.h> // malloc()
#include <ctype.h>
#include <regex.h>
#include <sys/types.h>

#include <ev.h>

#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

#include "ev_sock.h"
#include "http.h"
#include "html.h"
#include "header_hashes.h"

#define REGEX_REQUEST_LINE "([[:alpha:]]+)[[:space:]]*(/[[:alnum:]|/]*)[[:space:]]*(HTTP/[0-9].[0-9])"
//#define REGEX_HEADER_UPGRADE "Upgrade:[[:space:]]*(.*)"
//#define REGEX_HEADER_SEC_WEBSOCKET_KEY "Sec-WebSocket-Key:[[:space:]]*(.*)"
#define REGEX_HEADER_LINE "([[:alpha:]-]+)[[:space:]]*:[[:space:]]*(.*)"

#define APPLY_REQ_LINE(func) \
  func(ACCEPT);					\
  func(ACCEPT_CHARSET);				\
  func(ACCEPT_ENCODING);			\
  func(ACCEPT_LANGUAGE);			\
  func(ACCEPT_DATETIME);			\
  func(AUTHORIZATION);				\
  func(CACHE_CONTROL);				\
  func(CONNECTION);				\
  func(COOKIE);					\
  func(CONTENT_LENGTH);				\
  func(CONTENT_MD5);				\
  func(CONTENT_TYPE);				\
  func(DATE);					\
  func(EXPECT);					\
  func(FORWARDED);				\
  func(FROM);					\
  func(HOST);					\
  func(IF_MATCH);				\
  func(IF_MODIFIED_SINCE);			\
  func(IF_NONE_MATCH);				\
  func(IF_RANGE);				\
  func(IF_UNMODIFIED_SINCE);			\
  func(MAX_FORWARDS);				\
  func(ORIGIN);					\
  func(PRAGMA);					\
  func(PROXY_AUTHORIZATION);			\
  func(RANGE);					\
  func(REFERER);				\
  func(TE);					\
  func(USER_AGENT);				\
  func(UPGRADE);				\
  func(VIA);					\
  func(WARNING);				\
  func(SEC_WEBSOCKET_PROTOCOL);			\
  func(SEC_WEBSOCKET_KEY)
  
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
unsigned int hash(const char *str) {
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

#define BUFSIZE 1024
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

char* websocket_accept_string(char *key) {
  const char *magic =  "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  int magic_len = 36, key_len = strlen(key);

  BIO *b64, *mem;
  EVP_MD_CTX *evp_sha1;
    
  unsigned char *buf = malloc(SHA_DIGEST_LENGTH * sizeof(char));

  evp_sha1 = EVP_MD_CTX_create();
  EVP_DigestInit(evp_sha1, EVP_sha1());
  EVP_DigestUpdate(evp_sha1, key, key_len);
  EVP_DigestUpdate(evp_sha1, magic, magic_len);
  EVP_DigestFinal(evp_sha1, buf, NULL);

  mem = BIO_new(BIO_s_mem());
  b64 = BIO_new(BIO_f_base64());
  BIO_push(b64, mem);
  BIO_write(b64, buf, SHA_DIGEST_LENGTH);
  BIO_flush(b64);
  
  BUF_MEM *bufferPtr;
  BIO_get_mem_ptr(mem, &bufferPtr);
  BIO_set_close(mem, BIO_CLOSE);

  char* ret = malloc(bufferPtr->length * sizeof(char));
  memcpy(ret, bufferPtr->data, bufferPtr->length - 1);
  
  free(buf);
  BIO_free_all(mem);

  return(ret);
}

static void respond_http(http_response* res, http_request* req) {
  if (!strcasecmp("GET", req->request_line.method)) {
    if (!strcmp("/", req->request_line.uri)) {
      res->status_line.version = "HTTP/1.1";
      res->status_line.status = "200 OK";
      res->body = (char*) http_client;
      res->headers.content_type = "text/html";
      res->headers.content_length = strlen(res->body);
    } else if (!strncmp("/ws", req->request_line.uri, 3)) {
      printf("Websocket thingie\n");
      res->status_line.version = "HTTP/1.1";
      res->status_line.status = "101 Switching Protocols";
      res->headers.upgrade = "websocket";
      res->headers.connection = "Upgrade";
      char* accept = websocket_accept_string(req->header.sec_websocket_key);
      int accept_len = strlen(accept);
      printf("Returned value: %s\n", accept);
      res->headers.sec_websocket_accept = malloc(accept_len * sizeof(char));
      memcpy(res->headers.sec_websocket_accept, accept, accept_len);
      free(accept);
    } else {
      printf("404 (Får & får)\n");
      // 404
    }
  }
  else {
    // Unimplemented method
  }
}

void create_response_msg(ev_sock *w, http_response* res) {
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

void http_init(ev_sock *w, const char *msg, const int len) {
  puts("++++++[ enter http_init ]++++++");
  http_request request;
  http_response response;

  memset(&request, 0, sizeof(http_request));
  memset(&response, 0, sizeof(http_response));

  parse_http(&request, msg);
  respond_http(&response, &request);
  create_response_msg(w, &response);

  // Clean up
  free(request.header.sec_websocket_key);
  free(response.headers.sec_websocket_accept);
  puts("++++++[ exit http_init ]++++++");
}

