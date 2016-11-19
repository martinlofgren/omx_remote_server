#include "player.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h> // malloc()
#include <ctype.h> // isspace()

extern player player_info;

#define BUFSIZE 4096
#define STREAM_INFO 1
#define STREAM_STATUS 2

struct info {
  char* data;
  int len;
};

struct info reply (const int info) {
  struct info ret;
  ret.data = malloc(BUFSIZE * sizeof(char));

  if (info == STREAM_INFO) {
    ret.len = sprintf(ret.data,
		      "info-url: %s\n"
		      "stream-url: %s\n"
		      "title: %s\n"
		      "duration: %lu\n",
		      player_info.url_public,
		      player_info.url_stream,
		      player_info.title,
		      player_info.duration);
  }
  else if (info == STREAM_STATUS) {
    char* status_string;
    switch (player_info.status) {
    case PLAYER_STATUS_STOPPED:
      status_string = "stopped";
      break;
    case PLAYER_STATUS_PLAYING:
      status_string = "playing";
      break;
    case PLAYER_STATUS_PAUSED:
      status_string = "paused";
    }
    ret.len = sprintf(ret.data,
		      "status: %s\n"
		      "position: %lu\n",
		      status_string,
		      player_info.position);
  }
  else {
    ret.len = 0;
  }

  return ret;
}

void player_broadcast(const int info) {
  struct info msg;
  msg = reply(info);
  broadcast(msg.data, msg.len);
  free(msg.data);
}

void player_reply(ev_sock *w, const int info) {
  struct info msg;
  msg = reply(info);
  w->msg_produce(w, msg.data, msg.len);
  free(msg.data);
}

char* get_args(const char* msg, const int offset, const int len) {
  char *cp;

  // Remove trailing whitespace
  cp = (char*) msg + len - 1;
  while ((isspace(*cp)))
    *cp = 0;
  
  // Remove leading whitespace
  cp = (char*) msg + offset;
  while ((isspace(*cp)))
    cp++;

  return cp;
}

void player_control(ev_sock *w, const char* msg, const int len) {
  char* cp;

  if (strncmp(msg, "open", 4) == 0) {
    player_info.status = PLAYER_STATUS_PLAYING;
    cp = get_args(msg, 4, len);
    player_info.url_public = malloc(strlen(cp) * sizeof(char));
    strcpy(player_info.url_public, cp);
    player_broadcast(STREAM_INFO);
  }

  else if (strncmp(msg, "pauseplay", 9) == 0) {
    if (player_info.status != PLAYER_STATUS_STOPPED) {
      player_info.status =
	(player_info.status == PLAYER_STATUS_PLAYING)
	? PLAYER_STATUS_PAUSED
	: PLAYER_STATUS_PLAYING;
      player_broadcast(STREAM_STATUS);
    }
  }

  else if (strncmp(msg, "stop", 4) == 0) {
    player_info.status = PLAYER_STATUS_STOPPED;
    free(player_info.url_public);
    player_info.position = 0;
    player_broadcast(STREAM_STATUS);
  }

  else if (strncmp(msg, "seek", 4) == 0) {
    cp = get_args(msg, 4, len);
    player_info.position = strtoul(cp, NULL, 0);
    player_broadcast(STREAM_STATUS);
  }

  else if (strncmp(msg, "status", 6) == 0) {
    player_reply(w, STREAM_STATUS);
  }

}

