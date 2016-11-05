#ifndef OMX_REMOTE_SERVER_WS_H
#define OMX_REMOTE_SERVER_WS_H

typedef struct ws_msg {
  unsigned char fin;
  unsigned char opcode;
  unsigned char mask;
  unsigned int payload_len;
  unsigned char mask_key[4];
  unsigned char* payload_data;
} ws_msg;

char* ws_accept_string(const char *key);
void ws_client(ev_sock *w, const char *msg, const int len);

#endif // OMX_REMOTE_SERVER_WS_H
