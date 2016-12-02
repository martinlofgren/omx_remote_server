
SRCS   	= $(wildcard src/*.c)
OUT     = build

CFLAGS 	= -Wall -lev -lssl -lcrypto

$(OUT)/omx_remote_server : $(SRCS) src/*.h src/html.h src/macros.h
	cc $(SRCS) -o $(OUT)/omx_remote_server $(CFLAGS)

src/html.h : www/index.html
	cat www/index.html | \
	tr -d "\n" | \
	awk -f tools/stripspace.awk > www_client
	xxd -i www_client > src/html.h
	rm www_client

src/macros.h : tools/build_macros.c src/hash.c
	cc tools/build_macros.c src/hash.c -o build_macros
	./build_macros src/macros.h
	rm build_macros

.PHONY  : clean
clean   :
	-rm $(OUT)/omx_remote_server

