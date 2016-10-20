
SRCS   	= $(wildcard src/*.c)
OUT     = build

CFLAGS 	= -Wall -lev

$(OUT)/omx_remote_server : $(SRCS) src/index_html.h
	cc $(CFLAGS) $(SRCS) -o $(OUT)/omx_remote_server

src/index_html.h : src/index.html
	xxd -i src/index.html > src/index_html.h

.PHONY  : clean
clean   :
	-rm $(OUT)/omx_remote_server

