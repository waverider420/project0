#include "archive.h"
#include "arch_decs.h"


void archive(char *fnames[], int fnames_len) {
	struct prog_mode program_mode;
	int i;

	/* Initialize the root record */
	Memdir root;
	i = cstrlen(*fnames);
	if ((root.name = (char *) malloc(i + PROG_EXT_LEN + 1)) == NULL) {
		outputstr(bstderr, ER_MALLOC); exit(EXIT_FAILURE);
	}

	cstrcpy(root.name, *fnames);
	dirs_size += i + 1;

	program_mode.mode		= M_TREE_INIT;
	program_mode.content.files.p	= fnames + 1;
	program_mode.content.files.len	= fnames_len - 1;

	create_drec(&program_mode, &root);

	IOBUF *arch_fp;
	cstrcat(root.name, PROG_EXT);
	if ((arch_fp = bopen(root.name, IOBM_WO)) == NULL) {
		outputstr(bstderr, ER_CREATE, root.name); exit(EXIT_FAILURE);
	}
	*(root.name + i)  = '\0';

	bwrites(arch_fp, (char *) &dirs_size, sizeof dirs_size);
	file_offset = dirs_size + sizeof (DIRS_SIZE);
	if (write_dir_meta(arch_fp, &root) < 0) {
		outputstr(bstderr, "An error occured while writing.\n");
		unlink(arch_fp->name);
		exit(EXIT_FAILURE);
	}

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

int write_dir_meta(IOBUF *arch_fp, Memdir *mdp) {
	if (arch_fp == NULL || mdp == NULL) return -1;

	OFFSET_SIZE savedpos;
	Memfile *flistp;
	Memdir *dlistp;
	IOBUF *fp;
	char c;
	int i;

	OFFSET_LIST offset_list = sizeof (OFFSET_SIZE) * (mdp->nfiles + mdp->ndirs);

	if (bwrites(arch_fp, mdp->name, cstrlen(mdp->name) + 1) != cstrlen(mdp->name) + 1) 	return -1;
	if (bwrites(arch_fp, (char *) &mdp->perms, PERMS_SIZE) != PERMS_SIZE) 			return -1;
	if (bwrites(arch_fp, (char *) &offset_list, sizeof offset_list) != sizeof offset_list)  return -1;

	for (dlistp = mdp->dlist; dlistp - mdp->dlist < mdp->ndirs; dlistp++) {
		if (bwrites(arch_fp, (char *) &dlistp->offset, sizeof dlistp->offset) != sizeof dlistp->offset) return -1;

		savedpos = btell(arch_fp);
		if (savedpos != dlistp->offset)
			bseek(arch_fp, dlistp->offset, 0);

		if (write_dir_meta(arch_fp, dlistp) < 0) return -1;

		if (savedpos != dlistp->offset)
			bseek(arch_fp, savedpos, 0);
	}
	for (flistp = mdp->flist; flistp - mdp->flist < mdp->nfiles; flistp++) {
		if (bwrites(arch_fp, (char *) &file_offset, sizeof file_offset) != sizeof file_offset) return -1;

		savedpos = btell(arch_fp);
		bseek(arch_fp, file_offset, 0);

		i = last_dir_oset(flistp->name);
		if (bwrites(arch_fp, flistp->name + i, cstrlen(flistp->name + i) + 1) != cstrlen(flistp->name + i) + 1) return -1;
		if (bwrites(arch_fp, (char *) &flistp->perms, PERMS_SIZE) != PERMS_SIZE) 				return -1;
		if (bwrites(arch_fp, (char *) &flistp->size, sizeof flistp->size) != sizeof flistp->size) 		return -1;

		if ((fp = bopen(flistp->name, IOBM_RO)) == NULL) {
			outputstr(bstderr, ER_OPEN, flistp->name);
			return -1;
		}

		while((c = readc(fp)) != EOF)
			writec(arch_fp, c);
		bclose(fp);

		file_offset += cstrlen(flistp->name + i) + 1 + PERMS_SIZE + sizeof flistp->size + flistp->size;
		bseek(arch_fp, savedpos, 0);
	}

	return 0;
}

void create_drec(struct prog_mode *program_mode, Memdir *mdp) {
	struct dirent *dent;
	struct stat finfo;

	Memdir  *dlistp;
	Memfile	*flistp;
	
	char **p, *namep, c;
	long savedpos;
	DIR *dp;
	int i;

	mdp->offset = dirs_size + sizeof (DIRS_SIZE);

	mdp->flist = NULL;
	mdp->dlist = NULL;

	append_file_counts(program_mode, mdp);

	mdp->perms = 0;
	dirs_size += PERMS_SIZE;

	dirs_size += sizeof (OFFSET_LIST) + (mdp->nfiles + mdp->ndirs) * sizeof (OFFSET_SIZE);
	if (mdp->nfiles > 0) {
		if ((mdp->flist = flistp = (Memfile *) malloc(sizeof (Memfile) * mdp->nfiles)) == NULL) {
			outputstr(bstderr, ER_MALLOC); exit(EXIT_FAILURE);
		}
	} else mdp->flist = flistp = NULL;

	if (mdp->ndirs > 0) {
		if ((mdp->dlist = dlistp = (Memdir *) malloc(sizeof (Memdir) * mdp->ndirs)) == NULL) {
			outputstr(bstderr, ER_MALLOC); exit(EXIT_FAILURE);
		}
	} else mdp->dlist = dlistp = NULL;

	if (program_mode->mode == M_TREE_INIT) {
		mdp->perms |= ROOT_DIR_PERMS;

		i = program_mode->content.files.len;
		p = program_mode->content.files.p;
		program_mode->mode = M_TREE_CLAS;
		
		for (; i-- > 0; p++) {
			if (faccessat(AT_FDCWD, *p, R_OK, AT_SYMLINK_NOFOLLOW) < 0) continue;
			stat(*p, &finfo);
			if (S_ISREG(finfo.st_mode)) {
				create_frec(*p, flistp);
				flistp++;
			} else if (S_ISDIR(finfo.st_mode)) {
				cstrcpy(program_mode->content.dname, *p);
				create_drec(program_mode, dlistp++);
				dlistp++;
			}
		}
	} else if (program_mode->mode == M_TREE_CLAS) {
		stat(program_mode->content.dname, &finfo);
		mdp->perms |= (S_IRWXU | S_IRWXG | S_IRWXO) & finfo.st_mode;

		namep = program_mode->content.dname + last_dir_oset(program_mode->content.dname);
		i = cstrlen(namep) + 1;

		if ((c = (cstrcmp(namep, ".") == 0 || cstrcmp(namep, "..") == 0))) i++;
		if ((mdp->name = (char *) malloc(i)) == NULL) {
			outputstr(bstderr, ER_MALLOC); exit(EXIT_FAILURE);
		}

		*mdp->name = '\0';
		if (c) cstrncpy(mdp->name, "d", 1);
		cstrcat(mdp->name, namep);
		dirs_size += i;

		namep = program_mode->content.dname + cstrlen(program_mode->content.dname);

		dp = opendir(program_mode->content.dname);
		for (savedpos = 0; (dent = readdir(dp)) != NULL; savedpos++) {
			joinpath(program_mode->content.dname, dent->d_name);
			if (faccessat(dp->__dd_fd, dent->d_name, R_OK, AT_SYMLINK_NOFOLLOW) < 0) {
				*namep = '\0';
				continue;
			}
			if (dent->d_type & DT_REG) {
				create_frec(program_mode->content.dname, flistp++);
			} else if (dent->d_type & DT_DIR && NOT_SPECIAL_DIR(dent->d_name)) {
				closedir(dp);
				create_drec(program_mode, dlistp++);

				*namep = '\0';
				dp = opendir(program_mode->content.dname);
				cseekdir(dp, savedpos);
			}
			if (*namep != '\0') *namep = '\0';
		}
		closedir(dp);
	}
}

void cseekdir(DIR *dp, long nentrs) {
	rewinddir(dp);
	while ((readdir(dp) != NULL) && nentrs--);
}

void create_frec(char *name, Memfile *mfp) {
	struct stat finfo;

	if (stat(name, &finfo) < 0) {
		outputstr(bstderr, ER_OPEN, name); exit(EXIT_FAILURE); }
	if ((mfp->name = (char *) malloc(cstrlen(name) + 1)) == NULL) {
		outputstr(bstderr, ER_MALLOC); exit(EXIT_FAILURE); }
	cstrcpy(mfp->name, name);

	mfp->perms = 0;
	mfp->perms |= (S_IRWXU | S_IRWXG | S_IRWXO) & finfo.st_mode;

	mfp->size = finfo.st_size;
}


void append_file_counts(struct prog_mode *program_mode, Memdir *memdir) {
	struct dirent *dent;
	struct stat finfo;
	DIR *dp;
	char **p;
	int i;

	memdir->nfiles = memdir->ndirs = 0;

	if (program_mode->mode == M_TREE_INIT) {
		i = program_mode->content.files.len;
		p = program_mode->content.files.p; 
		while (i-- > 0) {
			if (faccessat(AT_FDCWD, *p, R_OK, AT_SYMLINK_NOFOLLOW) < 0) {
				outputstr(bstderr, "Error accessing *s. This file will be skipped. Running the program with 'sudo' may resolve this.\n", *p);
				continue;
			}
			if (stat(*p, &finfo) < 0) {
				outputstr(bstderr, ER_OPEN, *p); exit(EXIT_FAILURE);
			} else p++;
			if (S_ISREG(finfo.st_mode))
				memdir->nfiles++;
			else if (S_ISDIR(finfo.st_mode))
				memdir->ndirs++;
		}
	} else if (program_mode->mode == M_TREE_CLAS) {
		if ((dp = opendir(program_mode->content.dname)) == NULL) {
			outputstr(bstderr, ER_OPEN, program_mode->content.dname); exit(EXIT_FAILURE);
		}
		while ((dent = readdir(dp)) != NULL) {
			if (faccessat(dp->__dd_fd, dent->d_name, R_OK, AT_SYMLINK_NOFOLLOW) < 0) {
				outputstr(bstderr, "Error accessing *s/*s. This file will be skipped. Running the program with 'sudo' may resolve this.\n", program_mode->content.dname, dent->d_name);
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

void clearbuf(char *buf, int len) {
	while (len-- > 0) *buf++ = 0;
}
