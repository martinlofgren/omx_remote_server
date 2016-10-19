#include <unistd.h>  // write
#include <ev.h>

#include "ev_sock.h"

extern ev_sock listening_sock_watcher;

void link_client(ev_sock *w) {
  w->next = listening_sock_watcher.next;
  w->prev = &listening_sock_watcher;
  (w->prev)->next = w;
  if (w->next)
    (w->next)->prev = w;
}

void unlink_client(ev_sock *w) {
  (w->prev)->next = w->next;
  if (w->next)
    (w->next)->prev = w->prev;
}

void broadcast(char* buf, const ssize_t len) {
  ev_sock *tmp = &listening_sock_watcher;
  while (NULL != (tmp = tmp->next))
    write(tmp->io.fd, buf, (size_t) len);
}

