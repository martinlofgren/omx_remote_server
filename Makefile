
SRCS   	= $(wildcard src/*.c)
OUT     = build

CFLAGS 	= -Wall -lev -lssl -lcrypto

$(OUT)/omx_remote_server : $(SRCS) src/*.h 
	cc $(SRCS) -o $(OUT)/omx_remote_server $(CFLAGS)

src/html.h : www/index.html
	cat www/index.html | \
	tr -d "\n" | \
	awk -f tools/stripspace.awk > http_client
	xxd -i http_client > src/html.h
	rm http_client

src/header_hashes.h : tools/header_hash_gen.c
	cc tools/header_hash_gen.c -o hash_gen
	./hash_gen > src/header_hashes.h
	rm hash_gen

.PHONY  : clean
clean   :
	-rm $(OUT)/omx_remote_server

