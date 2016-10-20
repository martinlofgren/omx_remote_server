#ifndef OMX_REMOTE_SERVER_HTTP_H
#define OMX_REMOTE_SERVER_HTTP_H

typedef struct header_field {
  char* name;
  char* value;
  struct header_field* next;
} header_field;

typedef struct http_request {
  struct {
    char* method;
    char* uri;
    char* version;
  } request;
  struct header_field header;
  char* body;
} http_request;

typedef struct http_response {
  struct {
    char* version;
    char* status;
  } response;
  struct header_field header;
  char* body;
} http_response;
  
int is_http_connection(const char* msg);
void http_init(ev_sock *w, const char *msg, const int len);

#endif // OMX_REMOTE_SERVER_HTTP_H

