## omx_remote_server

Used to control the playback of the [omxplayer](https://github.com/popcornmix/omxplayer/) media player on the raspberry pi.

### Dependencies

- [libev](http://software.schmorp.de/pkg/libev.html) for the main event loop.
- [openssl](http://www.openssl.org) for upgrading http connection to websocket connection.
- Uses the linux command xxd to convert the html file into a char array.
