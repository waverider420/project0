CC = gcc
OBJS = main.c archive.c extract.c std/iobuf.c std/std.c std/outstr.c
CFLAGS = -Wall -Wextra

taro: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@
