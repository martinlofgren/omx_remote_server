#ifndef MAIN_H
#define MAIN_H

int init_socket(const int port);
static void listening_sock_cb(struct ev_loop *loop, ev_io *w, int revents);
static void client_sock_cb(struct ev_loop *loop, ev_io *w, int revents);

#endif // MAIN_H

