
SRCS   	= $(wildcard src/*.c)
OUT     = build

CFLAGS 	= -Wall -lev

### Compile, link and create load file
$(OUT)/omx_remote_server : $(SRCS)
	cc $(CFLAGS) $(SRCS) -o $(OUT)/omx_remote_server


### Cleanup
.PHONY  : clean
clean   :
	-rm $(OUT)/omx_remote_server

