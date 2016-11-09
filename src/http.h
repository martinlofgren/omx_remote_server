#ifndef OMX_REMOTE_SERVER_HTTP_H
#define OMX_REMOTE_SERVER_HTTP_H

typedef struct http_request {
  struct {
    char* method;
    char* uri;
    char* version;
  } request_line;
  struct {
    char* accept;
    char* accept_charset;
    char* accept_encoding;
    char* accept_language;
    char* accept_datetime;
    char* authorization;
    char* cache_control;
    char* connection;
    char* cookie;
    char* content_length;
    char* content_md5;
    char* content_type;
    char* date;
    char* expect;
    char* forwarded;
    char* from;
    char* host;
    char* if_match;
    char* if_modified_since;
    char* if_none_match;
    char* if_range;
    char* if_unmodified_since;
    char* max_forwards;
    char* origin;
    char* pragma;
    char* proxy_authorization;
    char* range;
    char* referer;
    char* te;
    char* user_agent;
    char* upgrade;
    char* via;
    char* warning;
    char* sec_websocket_protocol;
    char* sec_websocket_key;
  } header;
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
    char* upgrade;
    char* connection;
    char* sec_websocket_accept;
    int content_length;
  } headers;
  char* body;
} http_response;
  
void http_setup();
int is_http_connection(const char* msg);
void http_client_consumer(ev_sock* w, const char* msg, const int len);
void http_client_producer(ev_sock* w, const char* msg, const int len);

#endif // OMX_REMOTE_SERVER_HTTP_H

