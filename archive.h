/* Structure of an archive file.
	[DIRS_SIZE]
		[name + '\0']
		[perms] (PERMS_SIZE)
		[sizeof Offsets] (OFFSET_LIST)
			[OFFSET 1] (OFFSET_SIZE)
			[OFFSET 2] (OFFSET_SIZE)
			[OFFSET 3] (OFFSET_SIZE)

		[name + '\0'] ... (next dir)

	[filename + '\0']
	[file permissions] (PERMS_SIZE)
	[file size] (FILE_SIZE)
		[file contents]
	EOF

OFFSET_LIST = n offsets * OFFSET_SIZE

*/
#include <sys/errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include "iobuf.h"
#include "std.h"

#define DIRS_SIZE 	unsigned int
#define FILE_SIZE	unsigned long
#define OFFSET_LIST 	unsigned short
#define OFFSET_SIZE	unsigned long
#define PERMS_SIZE	2


typedef struct _memdir {
	char *name;
	unsigned short perms;

	unsigned nfiles;
	struct _memfile *flist;

	unsigned ndirs;
	struct _memdir *dlist;

	OFFSET_SIZE offset;
} Memdir;

typedef struct _memfile {
	char *name;
	unsigned short perms;

	FILE_SIZE size;
	OFFSET_SIZE offset;
} Memfile;
