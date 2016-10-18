#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <ev.h>

#define DEBUG

typedef struct ev_sock {
  ev_io io;
  struct ev_sock *prev;
  struct ev_sock *next;
} ev_sock;

ev_sock listening_sock_watcher;

// FUNCTION DECLARATIONS //

int init_socket(const int port);
static void listening_sock_cb(struct ev_loop *loop, ev_io *w, int revents);
static void client_sock_cb(struct ev_loop *loop, ev_io *w, int revents);
static void link_client(ev_sock *w_);
static void unlink_client(ev_sock *w_);
static void broadcast(char* buf, const ssize_t len);

// FUNCTION DEFINITIONS //

static void link_client(ev_sock *w) {
  w->next = listening_sock_watcher.next;
  w->prev = &listening_sock_watcher;
  (w->prev)->next = w;
  if (w->next)
    (w->next)->prev = w;
}

static void unlink_client(ev_sock *w) {
  (w->prev)->next = w->next;
  if (w->next)
    (w->next)->prev = w->prev;
}

#ifdef DEBUG
static void print_clients() {
  ev_sock *tmp = &listening_sock_watcher;
  printf("Active fds: ");
  while (NULL != (tmp = tmp->next)) {
    printf("%d, ", (int)(tmp->io.fd));
  }
  printf("\n");
}
#endif

static void broadcast(char* buf, const ssize_t len) {
  ev_sock *tmp = &listening_sock_watcher;
  while (NULL != (tmp = tmp->next))
    write(tmp->io.fd, buf, (size_t) len);
}

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
  ev_sock *client_sock_watcher = malloc(sizeof(ev_sock));
  link_client(client_sock_watcher);
  ev_io_init(&client_sock_watcher->io, client_sock_cb, new_client, EV_READ);
  ev_io_start(loop, &client_sock_watcher->io);
#ifdef DEBUG
  print_clients();
#endif
}

static void client_sock_cb(struct ev_loop *loop, ev_io *w, int revents) {
  if (revents & EV_READ) {
#ifdef DEBUG
    printf("client socket read event, fd=%d\n", w->fd);
#endif
    char buffer[1024];
    ssize_t len;
    if ((len = recv(w->fd, buffer, sizeof(buffer), 0)) <  0)
      perror("recv() failed");
    if (len == 0) {
#ifdef DEBUG
      puts("client disconnect");
#endif
      ev_io_stop(loop, w);
      unlink_client((ev_sock *)w); 
      free(w);
#ifdef DEBUG
      print_clients();
#endif
    } else {
#ifdef DEBUG
      char tmp[1024];
      snprintf(tmp, len, buffer);
      printf("received %d bytes\n%s\n", (int)len, tmp);
#endif
      broadcast(buffer, len);
    }
  }
  else if (revents & EV_WRITE) {
#ifdef DEBUG
    printf("client socket write event, fd=%d\n", w->fd);
#endif
    
  }
}

int init_socket(const int port) {
#ifdef DEBUG
  puts ("init_socket()");
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

int main (void) {
  struct ev_loop *loop = EV_DEFAULT;

  const int sock_fd = init_socket(12321);
  ev_io_init(&listening_sock_watcher.io, listening_sock_cb, sock_fd, EV_READ | EV_WRITE);
  ev_io_start(loop, &listening_sock_watcher.io);

#ifdef DEBUG
  puts("starting event loop");
#endif
  ev_run (loop, 0);
  
  return 0;
}
