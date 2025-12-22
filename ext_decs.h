char cpath[__DARWIN_MAXPATHLEN + 1] = "";
size_t arch_size = 0;

/* Function Declarations */
int bring2system(Memdir *, IOBUF *);
int createfile(Memfile *, IOBUF *);
mode_t generate_perms(char *);
void reconstruct(Memdir *, DIRS_SIZE, IOBUF *);
void reconst_frec(Memfile *, IOBUF *);
void joinpath(char *, char *);
void removedir(char *);
