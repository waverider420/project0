#include <stdarg.h>
#include "iobuf.h"

/* A stripped version of printf(). Only accepts the flags *s and *d (the character * acts here as % in the real printf) */
int outputstr(IOBUF *fp, char *fmt, ...) {
	va_list ap;
	char *reader;
	char *sval, *p;
	int dval;

	va_start(ap, fmt);
	for (reader = fmt; *reader != '\0'; reader++) {
		if (*reader == '*') {
			switch (*++reader) {
				case 'd':
					dval = va_arg(ap, int);
					sval = i2s(dval);
					bwrites(fp, sval, cstrlen(sval));
					free(sval);
					break;
				case 's':
					sval = va_arg(ap, char *);
					bwrites(fp, sval, cstrlen(sval));
					break;
				case '*':
					writec(fp, '*');
					break;
				default:
					writec(fp, '*');
					writec(fp, *reader);
			}
		} else
			writec(fp, *reader);
	}

	_flushbuf(fp);
	return reader - fmt;
}
