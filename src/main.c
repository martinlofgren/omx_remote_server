#define _GNU_SOURCE // strcasestr()

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>  // write()
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <ev.h>

#include "ev_sock.h"
#include "http.h"
#include "main.h"
#include "ws.h"

#define DEBUG

ev_sock listening_sock_watcher;

#ifdef DEBUG
/*
 * Function: print_clients
 * -----------------------
 * Helper function during development, prints current active 
 * file descriptors.
 */
static void print_clients() {
  ev_sock *tmp = &listening_sock_watcher;
  printf("Active fds: ");
  while (NULL != (tmp = tmp->next)) {
    printf("%d, ", (int)(tmp->io.fd));
  }
  printf("\n");
}
#endif

/*
 * Function: listening_sock_cb
 * ---------------------------
 * Callback for the listening socket which sets up a new client and connects
 * it to the event loop.
 *
 * loop: 
 * w:
 * revents:
 */
static void listening_sock_cb(struct ev_loop *loop, ev_io *w, int revents) {
#ifdef DEBUG
  printf("listening socket event, fd=%d\n", w->fd);
#endif
  int new_client;
  if ((new_client = accept(w->fd, NULL, NULL)) < 0)
    perror("accept() failed");
#ifdef DEBUG
  printf("new_client, fd:=%d\n", new_client);
#endif
  ev_sock *client_sock_watcher = (ev_sock *) malloc(sizeof(ev_sock));
  link_client(client_sock_watcher);
  client_sock_watcher->msg_consumer = detect_client;
  ev_io_init(&client_sock_watcher->io, client_sock_cb, new_client, EV_READ);
  ev_io_start(loop, &client_sock_watcher->io);
#ifdef DEBUG
  print_clients();
#endif
}

/*
 * Function: client_sock_cb
 * ------------------------
 * 
 *
 * loop:
 * w_:
 * revents:
 */
static void client_sock_cb(struct ev_loop *loop, ev_io *w_, int revents) {
  ev_sock *w = (ev_sock*) w_;
  if (revents & EV_READ) {
#ifdef DEBUG
    printf("client socket read event, fd=%d\n", w->io.fd);
#endif
    char buffer[1024];
    ssize_t len;
    if ((len = recv(w->io.fd, buffer, sizeof(buffer), 0)) <  0)
	  perror("recv() failed");
    if (len == 0) {
      ev_io_stop(loop, &w->io);
      unlink_client(w); 
      free(w);
#ifdef DEBUG
      puts("client disconnect");
      print_clients();
#endif
    }
    else
      w->msg_consumer(w, buffer, len);
  }
  else if (revents & EV_WRITE) {
#ifdef DEBUG
    printf("client socket write event, fd=%d\n", w->io.fd);
#endif
    
  }
}

/*
 * Function: detect_client
 * -----------------------
 * 
 *
 * w:
 * msg:
 * len:
 */
void detect_client(ev_sock *w, const char *msg, const int len) {
#ifdef DEBUG
  char tmp[1024];
  snprintf(tmp, len, msg);
  printf("\n---[ received %d bytes ]---\n%s\n---[ end recieved ]---\n\n", (int)len, tmp);
#endif
  w->msg_consumer = (is_http_connection(msg)) ? http_client : native_client;
  w->msg_consumer(w, msg, len);
}

/*
 * Function: native_client
 * -----------------------
 *
 *
 * w:
 * msg: 
 * len: 
 */
void native_client(ev_sock *w, const char *msg, const int len) {
  puts("Communication established");
  broadcast(msg, len);
}

/*
 * Function: setup_socket
 * ----------------------
 * Setup TCP server socket.
 *
 * port: the port to bind listening socket to
 *
 * Returns: listening socket file descriptor
 */
int setup_socket(const int port) {
#ifdef DEBUG
  puts ("setup_socket()");
#endif
  int sockfd, on = 1;
  struct sockaddr_in serv_addr;

  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  serv_addr.sin_addr.s_addr = INADDR_ANY;

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    perror ("socket() failed");
  if ((setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on))) < 0)
    perror ("setsockopt() failed");
  if ((ioctl(sockfd, FIONBIO, (char *)&on)) < 0) // Make non-blocking
    perror ("ioctl() failed");
  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    perror("bind() failed");
  if ((listen(sockfd, 32)) < 0)
    perror("listen() failed");

  return(sockfd);
}

/*
 * Function: main
 * --------------
 * Nothing much to say about this one.
 */
int main (void) {
#ifdef DEBUG
  printf("\n\n    --------------------[ PROGRAM STARTING ]--------------------\n\n");
#endif
  
  struct ev_loop *loop = EV_DEFAULT;

  const int sock_fd = setup_socket(12321);
  ev_io_init(&listening_sock_watcher.io, listening_sock_cb, sock_fd, EV_READ | EV_WRITE);
  ev_io_start(loop, &listening_sock_watcher.io);

  http_setup();

#ifdef DEBUG
  puts("starting event loop");
#endif
  ev_run (loop, 0);
  
  return 0;
}

