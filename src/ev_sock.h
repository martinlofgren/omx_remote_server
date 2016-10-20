#ifndef EV_SOCK_H
#define EV_SOCK_H

typedef struct ev_sock {
  ev_io io;
  struct ev_sock *prev;
  struct ev_sock *next;
  void (*msg_consumer)(struct ev_sock *w, char* buf, const int len);
} ev_sock;

void link_client(ev_sock *w_);
void unlink_client(ev_sock *w_);
void broadcast(char* buf, const ssize_t len);

#endif // EV_SOCK_H

