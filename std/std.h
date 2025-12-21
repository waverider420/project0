#ifndef STD_H
#define STD_H

void cstrncpy(char *, char *, int);
void cstrcat(char *, char*);
int cstrcmp(char *, char *);
int cstrncmp(char *, char *, int);
void cstrcpy(char *, char *);
int cstrlen(char *);
long c_pow(long, int);
long cnumlen(long);
void joinpath(char *, char *);
char *i2s(long);
long catoi(char *, long);

#endif
