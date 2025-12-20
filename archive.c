#include "archive.h"
#include "arch_decs.h"

void printbin(short p, int len);

void archive(char *fnames[], int fnames_len) {
	struct prog_mode program_mode;
	struct stat finfo;

	Memdir root, *dlistp;
	Memfile *flistp;

	program_mode.mode		= M_TREE_INIT;
	program_mode.content.files.p	= fnames;
	program_mode.content.files.len	= fnames_len;

	if (create_drec(&program_mode, &root) < 0) {
		printf("Internal error\n");

		/* Free memory */
		if (root.name != NULL) free(root.name);
		for (dlistp = root.dlist; dlistp - root.dlist < root.ndirs; free_mdp(dlistp++));
		for (flistp = root.flist; flistp - root.flist < root.nfiles; free(flistp++));
		return;
	}

	IOBUF *arch_fp;
	if ((arch_fp = bopen("archive.qarch", IOBM_WO)) == NULL) {
		printf("error: couldn't open file for writing. [Error code %d]\n", errno);
		return;
	}

	bwrites(arch_fp, (char *) &dirs_size, sizeof dirs_size);
	file_offset = dirs_size + sizeof (DIRS_SIZE);
	write_dir_meta(arch_fp, &root);

	bclose(arch_fp);
}

int last_dir_oset(char *dname) {
	char *dnamep;

	for (dnamep = dname; *dnamep != '\0'; dnamep++);
	if (dnamep == dname) return 0;
	if (*--dnamep == '/' && dnamep != dname) dnamep--;
	while (dnamep != dname && *dnamep != '/') dnamep--;
	if (dnamep == dname) {
		if (*dname == '/' && cstrlen(dname) > 1) return 1;
		else return 0;
	} else return dnamep - dname + 1;
}

void write_dir_meta(IOBUF *arch_fp, Memdir *mdp) {
	if (arch_fp == NULL || mdp == NULL) return;

	Memfile *flistp;
	Memdir *dlistp;
	long savedpos;
	IOBUF *fp;
	char c;
	int i;

	OFFSET_LIST offset_list = sizeof (OFFSET_SIZE) * (mdp->nfiles + mdp->ndirs);
	printf("\nEntered write_dir_meta with mdp->name = %s\n", mdp->name);

	printf("wrote %ld bytes of name %s\n", bwrites(arch_fp, mdp->name, cstrlen(mdp->name) + 1), mdp->name);
	printf("wrote %ld bytes of perms\n",bwrites(arch_fp, (char *) &mdp->perms, PERMS_SIZE));
	printf("wrote %ld bytes of offset_list\n", bwrites(arch_fp, (char *) &offset_list, sizeof offset_list));

	printf("looping thru dirs of %s\n", mdp->name);
	for (dlistp = mdp->dlist; dlistp - mdp->dlist < mdp->ndirs; dlistp++) {
		printf("wrote %ld bytes of offset %lu. Seeking there now.\n", bwrites(arch_fp, (char *) &dlistp->offset, sizeof dlistp->offset), dlistp->offset);

		savedpos = btell(arch_fp);
		if (savedpos != dlistp->offset) {
			printf("btw current position is %lu\n", savedpos);
			bseek(arch_fp, dlistp->offset, 0);
		}

		write_dir_meta(arch_fp, dlistp);
		if (savedpos != dlistp->offset) {
			bseek(arch_fp, savedpos, 0);
			printf("seeking back to %lu\n", savedpos);
		}
	}
	for (flistp = mdp->flist; flistp - mdp->flist < mdp->nfiles; flistp++) {
		printf("wrote %ld bytes of fileoffset %lu. Seeking there now.\n", bwrites(arch_fp, (char *) &file_offset, sizeof file_offset), file_offset);

		savedpos = btell(arch_fp);
		printf("btw current position is %lu\n", savedpos);
		bseek(arch_fp, file_offset, 0);

		i = last_dir_oset(flistp->name);
		printf("wrote %ld bytes of file name %s", bwrites(arch_fp, flistp->name + i, cstrlen(flistp->name + i) + 1), flistp->name);
		printf("wrote %ld bytes of perms", bwrites(arch_fp, (char *) &flistp->perms, PERMS_SIZE));
		printf("wrote %ld bytes of file size %lu\n", bwrites(arch_fp, (char *) &flistp->size, sizeof flistp->size), flistp->size);

		if ((fp = bopen(flistp->name, IOBM_RO)) == NULL) {
			printf("could not open file %s for reading. Skipping it.\n", flistp->name);
			continue;
		}
		printf("opened buffer with for file %s\n", fp->name);
		while((c = readc(fp)) != EOF)
			writec(arch_fp, c);
		bclose(fp);
		printf("copied and closed the buffer for file %s\n", fp->name);

		file_offset += cstrlen(flistp->name + i) + 1 + PERMS_SIZE + sizeof flistp->size + flistp->size;
		bseek(arch_fp, savedpos, 0);
		printf("seeking back to %lu\n", savedpos);
	}	
}

int create_drec(struct prog_mode *program_mode, Memdir *mdp) {
	struct dirent *dent;
	struct stat finfo;

	Memdir  *dlistp;
	Memfile	*flistp;
	
	char **p, *namep, c;
	long savedpos;
	DIR *dp;
	int i, fd;

	mdp->offset = dirs_size + sizeof (DIRS_SIZE);

	mdp->name  = NULL;
	mdp->flist = NULL;
	mdp->dlist = NULL;

	append_file_counts(program_mode, mdp);

	mdp->perms = 0;
	dirs_size += PERMS_SIZE;

	dirs_size += sizeof (OFFSET_LIST) + (mdp->nfiles + mdp->ndirs) * sizeof (OFFSET_SIZE);
	if (mdp->nfiles > 0) {
		if ((mdp->flist = flistp = (Memfile *) malloc(sizeof (Memfile) * mdp->nfiles)) == NULL) {
			printf("error: create_drec: can't allocate space.\n");
			return -1;
		}
	} else mdp->flist = flistp = NULL;

	if (mdp->ndirs > 0) {
		if ((mdp->dlist = dlistp = (Memdir *) malloc(sizeof (Memdir) * mdp->ndirs)) == NULL) {
			printf("error: create_drec: can't allocate space.\n");
			return -1;
		}
	} else mdp->dlist = dlistp = NULL;

	if (program_mode->mode == M_TREE_INIT) {
		if ((mdp->name = malloc(8)) == NULL) {
			printf("error: can't allocate space.\n");
			return -1;
		}
		cstrcpy(mdp->name, "Extract");
		dirs_size += 8;

		mdp->perms |= ROOT_DIR_PERMS;
		// mdp->perms = mdp->perms >> 7;
		printf("perms for %s:\n", mdp->name);
		printbin(mdp->perms, 9);

		i = program_mode->content.files.len;
		p = program_mode->content.files.p;
		program_mode->mode = M_TREE_CLAS;
		
		for (; i-- > 0; p++) {
			if (faccessat(AT_FDCWD, *p, R_OK, AT_SYMLINK_NOFOLLOW) < 0) continue;
			stat(*p, &finfo);
			if (S_ISREG(finfo.st_mode)) {
				if (create_frec(*p, flistp) < 0) {
					printf("Internal error.\n");
					return -1;
				}
				flistp++;
			} else if (S_ISDIR(finfo.st_mode)) {
				cstrcpy(program_mode->content.dname, *p);
				if (create_drec(program_mode, dlistp++) < 0) {
					printf("Internal error.\n");
					return -1;
				}
				dlistp++;
			}
		}
	} else if (program_mode->mode == M_TREE_CLAS) {
		if (stat(program_mode->content.dname, &finfo) < 0 || (dp = opendir(program_mode->content.dname)) == NULL) {
			printf("create_drec: error: can't open %s. [Error code %d]\n", program_mode->content.dname, errno);
			return -1;
		}

		mdp->perms |= (S_IRWXU | S_IRWXG | S_IRWXO) & finfo.st_mode;
		// mdp->perms = mdp->perms >> 7;
		printf("perms for %s:\n", program_mode->content.dname);
		printbin(mdp->perms, 9);

		namep = program_mode->content.dname + last_dir_oset(program_mode->content.dname);
		i = cstrlen(namep) + 1;

		if ((c = (cstrncmp(namep, ".", 1) == 0 || cstrncmp(namep, "..", 2) == 0))) i++;
		if ((mdp->name = (char *) malloc(i)) == NULL) {
			printf("error: create_frec: can't allocate space.\n");
			return -1;
		}

		*mdp->name = '\0';
		if (c) cstrncpy(mdp->name, "d", 1);
		cstrcat(mdp->name, namep);
		dirs_size += i;

		namep = program_mode->content.dname + cstrlen(program_mode->content.dname);

		for (savedpos = 0; (dent = readdir(dp)) != NULL; savedpos++) {
			joinpath(program_mode->content.dname, dent->d_name);
			if (faccessat(dp->__dd_fd, dent->d_name, R_OK, AT_SYMLINK_NOFOLLOW) < 0) {
				*namep = '\0';
				continue;
			}
			if (dent->d_type & DT_REG) {
				if (create_frec(program_mode->content.dname, flistp++) < 0) {
					printf("Internal error.\n");
					return -1;
				}
			} else if (dent->d_type & DT_DIR && NOT_SPECIAL_DIR(dent->d_name)) {
				closedir(dp);
				if (create_drec(program_mode, dlistp++) < 0) {
					printf("Internal error.\n");
					return -1;
				}
				*namep = '\0';
				dp = opendir(program_mode->content.dname);
				cseekdir(dp, savedpos);
			}
			if (*namep != '\0') *namep = '\0';
		}
		closedir(dp);
	}

	return 0;
}

void cseekdir(DIR *dp, long nentrs) {
	rewinddir(dp);
	while ((readdir(dp) != NULL) && nentrs--);
}

int create_frec(char *name, Memfile *mfp) {
	struct stat finfo;
	int i;

	if (stat(name, &finfo) < 0) {
		printf("File \"%s\" does not exist. [Error code %d]\n", name, errno);
		return -1;
	}
	if ((mfp->name = (char *) malloc(cstrlen(name) + 1)) == NULL) {
		printf("error: create_frec: can't allocate space.\n");
		return -1;
	}
	cstrcpy(mfp->name, name);

	mfp->perms = 0;
	mfp->perms |= (S_IRWXU | S_IRWXG | S_IRWXO) & finfo.st_mode;
	// mfp->perms = mfp->perms >> 7;
	printf("perms for %s\n", name);
	printbin(mfp->perms, 9);

	mfp->size = finfo.st_size;

	return 0;
}


void append_file_counts(struct prog_mode *program_mode, Memdir *memdir) {
	struct dirent *dent;
	struct stat finfo;
	DIR *dp;
	char **p, *namep;
	int i;

	memdir->nfiles = memdir->ndirs = 0;

	if (program_mode->mode == M_TREE_INIT) {
		i = program_mode->content.files.len;
		p = program_mode->content.files.p; 
		while (i-- > 0) {
			if (faccessat(AT_FDCWD, *p, R_OK, AT_SYMLINK_NOFOLLOW) < 0) {
				printf("could not open %s. This file will be skipped. Using 'sudo' may help. [Error code %d]\n", *p, errno);
				continue;
			}
			if (stat(*p++, &finfo) < 0) {
				printf("error: can't open file %s\n", *p);
				return;
			}
			if (S_ISREG(finfo.st_mode))
				memdir->nfiles++;
			else if (S_ISDIR(finfo.st_mode))
				memdir->ndirs++;
		}
	} else if (program_mode->mode == M_TREE_CLAS) {
		if ((dp = opendir(program_mode->content.dname)) == NULL) {
			printf("can't open %s. [Error code %d]\n", program_mode->content.dname, errno);
			return;
		}
		while ((dent = readdir(dp)) != NULL) {
			printf("=== Found file %s\n", dent->d_name);
			if (faccessat(dp->__dd_fd, dent->d_name, R_OK, AT_SYMLINK_NOFOLLOW) < 0) {
				printf("could not open %s/%s. This file will be skipped. Using 'sudo' may help. [Error code %d]\n", program_mode->content.dname, dent->d_name, errno);
				continue;
			}
			if (dent->d_type & DT_REG)
				memdir->nfiles++;
			else if (dent->d_type & DT_DIR && NOT_SPECIAL_DIR(dent->d_name))
				memdir->ndirs++;
		}
		closedir(dp);
	}
}

void free_mdp(Memdir *mdp) {
	Memdir *dlistp;
	Memfile *flistp;

	if (mdp->name != NULL) free(mdp->name);
	for (dlistp = mdp->dlist; dlistp - mdp->dlist < mdp->ndirs; free_mdp(dlistp++));
	for (flistp = mdp->flist; flistp - mdp->flist < mdp->nfiles; free(flistp++));
}

void clearbuf(char *buf, int len) {
	while (len-- > 0) *buf++ = 0;
}

void printbin(short p, int len) {
	int i;
	for (i = len - 1; i >= 0; i--)
		printf("%d", (p >> i) & 1);
	printf("\n");
}
