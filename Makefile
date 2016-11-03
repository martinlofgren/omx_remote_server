
SRCS   	= $(wildcard src/*.c)
OUT     = build

CFLAGS 	= -Wall -lev -lssl -lcrypto

$(OUT)/omx_remote_server : $(SRCS) src/*.h src/html.h src/header_hashes.h
	cc $(SRCS) -o $(OUT)/omx_remote_server $(CFLAGS)

src/html.h : www/index.html
	cat www/index.html | \
	tr -d "\n" | \
	awk -f tools/stripspace.awk > www_client
	xxd -i www_client > src/html.h
	rm www_client

src/header_hashes.h : tools/header_hash_gen.c src/hash.c
	cc tools/header_hash_gen.c src/hash.c -o hash_gen
	./hash_gen > src/header_hashes.h
	rm hash_gen

.PHONY  : clean
clean   :
	-rm $(OUT)/omx_remote_server

