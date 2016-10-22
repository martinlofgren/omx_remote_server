#ifndef OMX_REMOTE_SERVER_HTTP_H
#define OMX_REMOTE_SERVER_HTTP_H

typedef struct http_request {
  struct {
    int method;
    char* uri;
    char* version;
  } request_line;
  struct {
    char* host;
    char* from;
    char* user_agent;
  } headers;
  char* body;
} http_request;

typedef struct http_response {
  struct {
    char* version;
    char* status;
  } status_line;
  struct {
    char* date;
    char* last_modified;
    char* content_type;
    int content_length;
  } headers;
  char* body;
} http_response;
  
int is_http_connection(const char* msg);
void http_init(ev_sock *w, const char *msg, const int len);

#endif // OMX_REMOTE_SERVER_HTTP_H

