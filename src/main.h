#ifndef MAIN_H
#define MAIN_H

int init_socket(const int port);
static void listening_sock_cb(struct ev_loop *loop, ev_io *w, int revents);
static void client_sock_cb(struct ev_loop *loop, ev_io *w, int revents);
void communication_init(ev_sock *w, char *buf, const int len);
void http_init(ev_sock *w, char *buf, const int len);
void communication_established(ev_sock *w, char *buf, const int len);

#endif // MAIN_H

