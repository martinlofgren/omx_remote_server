#ifndef OMX_REMOTE_SERVER_EV_SOCK_H
#define OMX_REMOTE_SERVER_EV_SOCK_H

#include <ev.h>
#include <sys/types.h>

#define CLIENT_UNKNOWN          0x00
#define CLIENT_BROADCAST_ENABLE 0x01
#define CLIENT_NATIVE           0x02
#define CLIENT_HTTP             0x04
#define CLIENT_WS               0x08

typedef struct ev_sock {
  ev_io io;
  struct ev_sock *prev;
  struct ev_sock *next;
  void (*msg_consume)(struct ev_sock *w, const char* msg, const int len);
  void (*msg_produce)(struct ev_sock *w, const char* msg, const int len);
} ev_sock;

void link_client(ev_sock *w_);
void unlink_client(ev_sock *w_);
void broadcast(const char* msg, const ssize_t len);

#endif // OMX_REMOTE_SERVER_EV_SOCK_H

