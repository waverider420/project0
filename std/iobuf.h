#ifndef IOBUF_H
#define IOBUF_H

#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include "std.h"

#define BOPEN_MAX 	20
#define BUFSIZE 	2024
#define EOF		(-1)

/* I/O Modes */
enum {
	IOBM_RO, 	/* Read Only */
	IOBM_WO,	/* Write Only */
	IOBM_RW,	/* Read & Write */
	IOBM_AP		/* Append */
};

struct _mode {
	char is_write	: 1;
	char is_read	: 1;
	char is_err	: 1;
	char is_eof	: 1;
};

typedef struct iobuf {
	char *name;
	int fd, cleft;

	char buf[BUFSIZE];
	char *bufp;

	struct _mode mode;
	struct stat info;
} IOBUF;

extern IOBUF _bstdin;
extern IOBUF _bstdout;
extern IOBUF _bstderr;

#define bstdin  &_bstdin
#define bstdout &_bstdout
#define bstderr &_bstderr

#define readc(fp) ((fp)->cleft-- > 0 ? *(fp)->bufp++ : _fillbuf(fp))
#define writec(fp, c) ((fp)->cleft-- > 0 ? *(fp)->bufp++ = (c) : _flushbufc(fp, c))
#define writeline(fd, lineptr) (write((fd), lineptr, cstrlen(lineptr))

/* Funfction declarations */


IOBUF *bopen(char *, char);
int _fillbuf(IOBUF *);
int _flushbuf(IOBUF *);
int _flushbufc(IOBUF *, char);
void bclose(IOBUF *);
off_t btell(IOBUF *);
off_t bseek(IOBUF *, long, int);
long breads(IOBUF *, char *, long);
long bwrites(IOBUF *, char *, long);

#endif
