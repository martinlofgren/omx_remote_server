#ifndef OMX_REMOTE_SERVER_EV_SOCK_H
#define OMX_REMOTE_SERVER_EV_SOCK_H

typedef struct ev_sock {
  ev_io io;
  struct ev_sock *prev;
  struct ev_sock *next;
  void (*msg_consumer)(struct ev_sock *w, const char* msg, const int len);
} ev_sock;

void link_client(ev_sock *w_);
void unlink_client(ev_sock *w_);
void broadcast(const char* msg, const ssize_t len);

#endif // OMX_REMOTE_SERVER_EV_SOCK_H

