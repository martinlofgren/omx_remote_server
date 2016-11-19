#ifndef OMX_REMOTE_SERVER_PLAYER_H
#define OMX_REMOTE_SERVER_PLAYER_H

#include "ev_sock.h"

#define PLAYER_STATUS_STOPPED 0x00
#define PLAYER_STATUS_PLAYING 0x01
#define PLAYER_STATUS_PAUSED  0x02

typedef struct player {
  unsigned char status;
  char* title;
  char* url_public;
  char* url_stream;
  unsigned long duration;
  unsigned long position;
} player;

void player_control(ev_sock *w, const char* msg, const int len);

#endif // OMX_REMOTE_SERVER_PLAYER_H

