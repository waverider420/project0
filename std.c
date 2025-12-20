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

void joinpath(char *path, char *fname) {
	if (*path != '\0') {
		while (*path) path++;
		if (*(path - 1) != '/') *path++ = '/';
	}
	while ((*path++ = *fname++));
}
