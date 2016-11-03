#ifndef OMX_REMOTE_SERVER_MAIN_H
#define OMX_REMOTE_SERVER_MAIN_H

int init_socket(const int port);
static void listening_sock_cb(struct ev_loop *loop, ev_io *w, int revents);
static void client_sock_cb(struct ev_loop *loop, ev_io *w, int revents);
void detect_client(ev_sock *w, const char *msg, const int len);
void native_client(ev_sock *w, const char *msg, const int len);

#endif // OMX_REMOTE_SERVER_MAIN_H

