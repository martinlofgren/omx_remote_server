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
  // Allocate sock structure and initialize it
  ev_sock *client_sock_watcher = (ev_sock *) malloc(sizeof(ev_sock));
  link_client(client_sock_watcher);
  client_sock_watcher->msg_consume = detect_client;
  client_sock_watcher->msg_produce = NULL;
  ev_io_init(&client_sock_watcher->io, client_sock_cb, new_client, EV_READ);
  ev_io_start(loop, &client_sock_watcher->io);
#ifdef DEBUG
  print_clients();
#endif
}

/*
 * Function: client_sock_cb
 * ------------------------
 * The callback associated with all clients. Does the standard reading of the
 * socket and checking for disconnected clients; if connected client is
 * sending data, this is handled by associated function.
 *
 * loop: the main libev event loop
 * w_: the ev_io struct, which is casted to ev_sock to get hold of function
 *     pointer and more
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
      w->msg_consume(w, buffer, len);
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
 * Simple init function for newly connected clients to decide whether the
 * client is a native client or http -> ws client; then invoke associated
 * function.
 *
 * w: the ev_sock structure which triggered the callback
 * msg: received message
 * len: length of msg
 */
void detect_client(ev_sock *w, const char *msg, const int len) {
#ifdef DEBUG
  char tmp[1024];
  snprintf(tmp, len, msg);
  printf("\n---[ received %d bytes ]---\n%s\n---[ end recieved ]---\n\n", (int)len, tmp);
#endif
  if (is_http_connection(msg)) {
    w->msg_consume = http_client_consumer;
    w->msg_produce = NULL;
  } else {
    w->msg_consume = native_client_consumer;
    w->msg_produce = native_client_producer;
  }
  w->msg_consume(w, msg, len);
}

/*
 * Function: native_client_consumer
 * --------------------------------
 * Function to handle incoming messages from native clients.
 *
 * w: the ev_sock structure which triggered the callback
 * msg: received message
 * len: length of msg
 */
void native_client_consumer(ev_sock *w, const char *msg, const int len) {
  puts("Incoming message from native client");
  broadcast(msg, len);
}

/*
 * Function: native_client_producer
 * --------------------------------
 * Function to handle outgoing messages to native clients.
 *
 * w: the ev_sock structure which triggered the callback
 * msg: received message
 * len: length of msg
 */
void native_client_producer(ev_sock *w, const char *msg, const int len) {
  printf("native_client_produce event: %s\n", msg);
  write(w->io.fd, msg, (size_t) len);
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

