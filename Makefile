
SRCS   	= $(wildcard src/*.c)
OUT     = build

CFLAGS 	= -Wall -lev

$(OUT)/omx_remote_server : $(SRCS) src/html.h
	cc $(CFLAGS) $(SRCS) -o $(OUT)/omx_remote_server

src/html.h : www/index.html
	xxd -i www/index.html > src/html.h

.PHONY  : clean
clean   :
	-rm $(OUT)/omx_remote_server

