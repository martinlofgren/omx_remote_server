
SRCS   	= $(wildcard src/*.c)
OUT     = build

CFLAGS 	= -Wall -lev

$(OUT)/omx_remote_server : $(SRCS) src/html.h
	cc $(CFLAGS) $(SRCS) -o $(OUT)/omx_remote_server

src/html.h : www/index.html
	cat www/index.html | \
	awk -f tools/expand_includes.awk | \
	tr -d "\n" | \
	awk -f tools/stripspace.awk > http_client
	xxd -i http_client > src/html.h
	rm http_client

.PHONY  : clean
clean   :
	-rm $(OUT)/omx_remote_server

