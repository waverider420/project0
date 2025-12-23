#include <stdlib.h>


void cstrncpy(char *s, char *t, int len) {
	while (len-- > 0 && (*s++ = *t++));
	while (len-- > 0) *s++ = '\0';
}

void cstrcat(char *s, char *t) {
	while (*s != '\0') s++;
	while ((*s++ = *t++));
}

int cstrcmp(char *s, char *t) {
	for ( ; *s && *s == *t; s++, t++);
	return *s - *t;
}

int cstrncmp(char *s, char *t, int len) {
	while (len-- > 0 && *s && *s == *t) 
		if (len > 0) { s++; t++; }
	return *s - *t;
}

void cstrcpy(char *s, char *t) {
	while ((*s++ = *t++));
}

int cstrlen(char *s) {
	int len;
	for (len = 0; *s++ != '\0'; len++);
	return len;
}

long c_pow(long base, int power) {
	if (power == 0) return 1;
	long result = 1;

	while (power-- > 0) result *= base;
	return result;
}

long cnumlen(long num) {
	unsigned result;

	if (num < 0) num *= -1;
	result = 0;
	for (; num > 0; num /= 10) result++;

	return result;
}

void joinpath(char *path, char *fname) {
	if (*path != '\0') {
		while (*path) path++;
		if (*(path - 1) != '/') *path++ = '/';
	}
	while ((*path++ = *fname++));
}

char *i2s(long d) {
	long dlen, dtmp;
	char *result, *rp;

	dlen = cnumlen(d) + ((d < 0) ? 1 : 0);
	if ((result = rp = (char *) malloc(dlen + 1)) == NULL) return NULL;
	if (d < 0) {
		*rp++ = '-';
		d *= -1;
		dlen--;
	}

	dtmp = d;
	while (--dlen >= 0) {
		dtmp = d / c_pow(10, dlen);
		*rp++ = dtmp - (dtmp / 10) * 10 + '0';
	}
	*rp = '\0';
	return result;
}

long c_atoi(char *s, long len) {
	long result;
	int sign;

	result = 0;
	sign = (*s == '-') ? -1 : 1;
	if (*s == '+' || *s == '-') s++;

	while (len-- > 0 && *s != '\0')
		result += result * 10 + (*s++ - '0');

	return result * sign;
}

int findchar(char *s, char c) {
	char *start;
	for (start = s; *s != c && *s != '\0'; s++);

	return (*s == '\0') ? -1 : s - start;
}
