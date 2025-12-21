#include "iobuf.h"
#include <stdio.h>

unsigned open_file_count = 0;

IOBUF _bstdin = {
	.name  	= "stdin",
	.fd 	= 0,
	.cleft 	= 0,
	.bufp	= _bstdin.buf,
	.mode	= {0, 1, 0, 0},
	.info 	= {0}
};

IOBUF _bstdout = {
	.name  	= "stdout",
	.fd 	= 1,
	.cleft 	= 0,
	.bufp	= _bstdout.buf,
	.mode	= {1, 0, 0, 0},
	.info 	= {0}
};

IOBUF _bstderr = {
	.name  	= "stderr",
	.fd 	= 2,
	.cleft 	= 0,
	.bufp	= _bstderr.buf,
	.mode	= {0, 1, 0, 0},
	.info 	= {0}
};


IOBUF *bopen(char *fname, char mode) {
	struct stat finfo;
	int o_mode;
	IOBUF *fp;
	int fd, perms;

	if (open_file_count + 1 >= BOPEN_MAX) return NULL;

	if (mode == IOBM_RO) { o_mode = O_RDONLY; perms = 0; }
	else if ((mode == IOBM_WO) || (mode == IOBM_AP)) { o_mode = O_WRONLY | O_CREAT; perms = 0777; }
	else if (mode == IOBM_RW) { o_mode = O_RDWR; perms = 0777; }
	else return NULL;

	if ((fd = open(fname, o_mode, perms)) < 0) return NULL;
	if (fstat(fd, &finfo) < 0) {
		close(fd);
		return NULL;
	}
	if ((fp = (IOBUF *) malloc(sizeof (IOBUF))) == NULL) {
		close(fd);
		return NULL;
	}
	if ((fp->name = (char *) malloc(cstrlen(fname) + 1)) == NULL) {
		close(fd);
		free(fp);
		return NULL;
	}
	if (mode == IOBM_AP) lseek(fd, 0, 2);

	cstrcpy(fp->name, fname);
	fp->bufp = fp->buf;
	fp->cleft = 0;
	fp->fd = fd;

	fp->mode.is_read  = (mode == IOBM_RO) ? 1 : 0;
	fp->mode.is_write = ((mode == IOBM_WO) || (mode == IOBM_AP)) ? 1 : 0;
	fp->mode.is_err	  = fp->mode.is_eof = 0;
	fp->info = finfo;

	open_file_count++;

	return fp;
}

/* _fillbuf fills the buffer and reads the first character. It doesn't advance bufp */
int _fillbuf(IOBUF *fp) {
	if (fp == NULL) return EOF;
	if (!(fp->mode.is_read) || fp->mode.is_eof || fp->mode.is_err) {
		errno = 1;
		return EOF;
	}

	if ((fp->cleft = read(fp->fd, fp->buf, BUFSIZE)) != BUFSIZE) {
		if (fp->cleft < 0) fp->mode.is_err = 1;
		else fp->mode.is_eof = 1;
	}

	fp->bufp = fp->buf;
	fp->cleft--;
	return *fp->bufp++;
}

int _flushbuf(IOBUF *fp) {
	if (fp == NULL) return EOF;
	if (!(fp->mode.is_write) || fp->mode.is_err) {
		errno = 1;
		return EOF;
	}

	long writelen = fp->bufp - fp->buf;
	if (writelen > 0 && write(fp->fd, fp->buf, writelen) != writelen) {
		fp->mode.is_err = 1;
		return EOF;
	}

	fp->cleft = BUFSIZE;
	fp->bufp = fp->buf;
	return 0;
}

int _flushbufc(IOBUF *fp, char c) {
	if (_flushbuf(fp) < 0) return EOF;
	fp->cleft--;
	return (*fp->bufp++ = c);
}

void bclose(IOBUF *fp) {
	if (fp == NULL) return;
	if (fp->mode.is_write)
		_flushbuf(fp);
	close(fp->fd);
	free(fp->name);
	free(fp);
	open_file_count--;
}

/* tell the position of the writer/reader in a file */
off_t btell(IOBUF *fp) {
	if (fp == NULL) return -1;
	off_t lsk_pos = lseek(fp->fd, 0, 1);

	if (fp->mode.is_read) lsk_pos -= fp->cleft;
	if (fp->mode.is_write) lsk_pos += fp->bufp - fp->buf;

	return lsk_pos;
}

off_t bseek(IOBUF *fp, long offset, int origin) {
	if (fp == NULL || offset < 0 || origin < 0 || origin > 2) return -1;
	if (offset == 0 && origin == 1) return btell(fp);

	if (fp->mode.is_write) _flushbuf(fp);
	if (fp->mode.is_read) {
		if (origin == 1) offset -= fp->cleft;
		if (fp->mode.is_eof && offset <= fp->info.st_size) fp->mode.is_eof = 0;
	}
	fp->cleft = 0;

	return lseek(fp->fd, offset, origin);
}

long breads(IOBUF *fp, char *p, long len) {
	char *start;

	for (start = p; len-- > 0; *p++ = readc(fp));
	return p - start;
}

long bwrites(IOBUF *fp, char *p, long len) {
	char *start;

	for (start = p; len-- > 0; writec(fp, *p++));
	return p - start;
}
