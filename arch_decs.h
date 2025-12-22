#define M_TREE_INIT 0 /* Used for initializing a tree (root) */
#define M_TREE_CLAS 1 /* Used for recording the remaining branches of a tree */

#define ROOT_DIR_PERMS 0777

#define NOT_SPECIAL_DIR(dname) (cstrcmp(dname, ".") != 0 && cstrcmp(dname, "..") != 0)

struct prog_mode {
	char mode;
	union _content {
		char dname[__DARWIN_MAXPATHLEN + 1];
		struct _files {
			char **p;
			int len;
		} files;
	} content;
};

static DIRS_SIZE dirs_size = 0;
OFFSET_SIZE file_offset = 0;

int last_dir_oset(char *);
int write_dir_meta(IOBUF *, Memdir *);
void create_drec(struct prog_mode *, Memdir *);
void cseekdir(DIR *, long);
void create_frec(char *, Memfile *);
void write_perms(char *, struct stat *);
void append_file_counts(struct prog_mode *, Memdir *);
void free_mdp(Memdir *);
void clearbuf(char *, int);
