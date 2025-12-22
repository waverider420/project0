#include "archive.h"
#include "ext_decs.h"

void extract(char *arch_name) {
	DIRS_SIZE dirs_size;
	IOBUF *arch_fp;
	Memdir root;
	int fd;

	if ((arch_fp = bopen(arch_name, IOBM_RO)) == NULL) {
		outputstr(bstderr, ER_OPEN, arch_name);
		exit(EXIT_FAILURE);
	}

	arch_size = arch_fp->info.st_size;

	/* Read DIRS_SIZE */
	breads(arch_fp, (char *) &dirs_size, sizeof dirs_size);

	/* Reconstruct directory structure */
	reconstruct(&root, dirs_size, arch_fp);

	/* Write everything */
	joinpath(cpath, root.name);
	if (bring2system(&root, arch_fp) < 0)
		if ((fd = open(root.name, O_RDONLY, 0)) >= 0) {
			*cpath = '\0';
			removedir(cpath);
			close(fd);
		}
}

int bring2system(Memdir *mdp, IOBUF *arch_fp) {
	char *cnamebord;

	if (mkdir(cpath, 0777) < 0) {
		outputstr(bstderr, "Can't create directory at *s\n", cpath); exit(EXIT_FAILURE);
	}

	/* Set permissions */
	if (chmod(cpath, mdp->perms) < 0)
		outputstr(bstderr, ER_SET_PERMS, cpath);

	/* Loop over entries */
	Memdir  *dlistp;
	Memfile *flistp;
	unsigned i;

	dlistp = mdp->dlist;
	flistp = mdp->flist;
	cnamebord = cpath + cstrlen(cpath);
	for (i = 0; i < mdp->ndirs; i++) {
		joinpath(cpath, dlistp->name);
		if (bring2system(dlistp++, arch_fp) < 0) return -1;
		*cnamebord = '\0';
	}
	for (i = 0; i < mdp->nfiles; i++) {
		joinpath(cpath, flistp->name);
		if (createfile(flistp++, arch_fp) < 0) return -1;
		*cnamebord = '\0';
	}

	return 0;
}

int createfile(Memfile *mfp, IOBUF *arch_fp) {
	IOBUF *fp;

	if ((fp = bopen(cpath, IOBM_WO)) == NULL) {
		outputstr(bstderr, ER_CREATE, cpath);
		return -1;
	}

	/* Set permissions */
	if (chmod(cpath, mfp->perms) < 0)
		outputstr(bstderr, ER_SET_PERMS, cpath);

	/* Copy contents */
	FILE_SIZE i;
	OFFSET_SIZE savedpos;

	if (mfp->size > 0) {
		savedpos = btell(arch_fp);
		bseek(arch_fp, mfp->offset, 0);

		for (i = 0; i++ < mfp->size; writec(fp, readc(arch_fp)));
		bseek(arch_fp, savedpos, 0);
	}

	bclose(fp);
	return 0;
}

void reconstruct(Memdir *mdp, DIRS_SIZE dirs_size, IOBUF *arch_fp) {
	/* Get name */
	OFFSET_SIZE savedpos;
	int name_len;
	char c;

	savedpos = btell(arch_fp);
	for (name_len = 0; (c = readc(arch_fp)) != '\0'; name_len++);
	if (name_len > __DARWIN_MAXNAMLEN) {
		outputstr(bstderr, ER_CORRUPT, arch_fp->name); exit(EXIT_FAILURE);
	}
	if ((mdp->name = malloc(name_len + 1)) == NULL) {
		outputstr(bstderr, ER_MALLOC); exit(EXIT_FAILURE);
	}
	bseek(arch_fp, savedpos, 0);
	breads(arch_fp, mdp->name, name_len +1);

	/* Get permissions */
	if (breads(arch_fp, (char *) &mdp->perms, PERMS_SIZE) != PERMS_SIZE) {
		outputstr(bstderr, ER_CORRUPT, arch_fp->name); exit(EXIT_FAILURE);
	}

	/* Get the size of offsets */
	OFFSET_LIST offset_list_size, offseti;
	OFFSET_SIZE offset;

	if (breads(arch_fp, (char *) &offset_list_size, sizeof offset_list_size) != sizeof offset_list_size ||
	    offset_list_size % sizeof offset != 0) {
		outputstr(bstderr, ER_CORRUPT, arch_fp->name); exit(EXIT_FAILURE);
	}

	mdp->ndirs = mdp->nfiles = 0;
	if (offset_list_size == 0) {
		mdp->dlist = NULL;
		mdp->flist = NULL;
		return;
	}

	/* Append counts of directories and files to mdp */
	savedpos = btell(arch_fp);

	for (offseti = 0; offseti < offset_list_size; offseti += sizeof offset) {
		if (breads(arch_fp, (char *) &offset, sizeof offset) != sizeof offset) {
			outputstr(bstderr, ER_CORRUPT, arch_fp->name); exit(EXIT_FAILURE);
		}
		if (offset < (dirs_size + sizeof dirs_size)) mdp->ndirs++;
		else if (offset < arch_size) mdp->nfiles++;
		else { outputstr(bstderr, ER_CORRUPT, arch_fp->name); exit(EXIT_FAILURE); }
	}
	bseek(arch_fp, savedpos, 0);

	/* Allocate space for pointers */
	Memdir  *dlistp;
	Memfile *flistp;

	if (mdp->ndirs > 0) {
		if ((mdp->dlist = dlistp = (Memdir *) malloc(mdp->ndirs * sizeof (Memdir))) == NULL) {
			outputstr(bstderr, ER_MALLOC); exit(EXIT_FAILURE);
		}
	} else mdp->dlist = dlistp = NULL;

	if (mdp->nfiles > 0) {
		if ((mdp->flist = flistp = (Memfile *) malloc(mdp->nfiles * sizeof (Memfile))) == NULL) {
			outputstr(bstderr, ER_MALLOC); exit(EXIT_FAILURE);
		}
	} else mdp->flist = flistp = NULL;

	/* Assign eash pointer a corresponding struct */
	for (; offseti > 0; offseti -= sizeof offset) {
		breads(arch_fp, (char *) &offset, sizeof offset);

		savedpos = btell(arch_fp);
		bseek(arch_fp, offset, 0);
		if (offset < (dirs_size + sizeof dirs_size))
			reconstruct(dlistp++, dirs_size, arch_fp);
		else
			reconst_frec(flistp++, arch_fp);
		bseek(arch_fp, savedpos, 0);
	}
}

void reconst_frec(Memfile *mfp, IOBUF *arch_fp) {
	/* Get name */
	long savedpos;
	int name_len, tmp;
	char c;

	savedpos = btell(arch_fp);
	for (name_len = 0; (c = readc(arch_fp)) != '\0'; name_len++);
	bseek(arch_fp, savedpos, 0);

	if ((mfp->name = (char *) malloc(name_len)) == NULL) {
		outputstr(bstderr, ER_MALLOC); exit(EXIT_FAILURE);
	}
	breads(arch_fp, mfp->name, name_len + 1);

	/* Get permissions */
	if ((tmp = breads(arch_fp, (char *) &mfp->perms, PERMS_SIZE)) != PERMS_SIZE) {
		outputstr(bstderr, ER_CORRUPT, arch_fp->name); exit(EXIT_FAILURE);
	}

	/* Get file size */
	if (breads(arch_fp, (char *) &mfp->size, sizeof mfp->size) != sizeof mfp->size) {
		outputstr(bstderr, ER_CORRUPT, arch_fp->name); exit(EXIT_FAILURE);
	}

	/* Set file content offset */
	mfp->offset = btell(arch_fp);
}

void removedir(char *dirname) {
	struct dirent *entry;
	struct stat finfo;
	char *nameend;
	DIR *dp;

	nameend = dirname + cstrlen(dirname);
	if ((dp = opendir(dirname)) == NULL) return;
	while ((entry = readdir(dp)) != NULL) {
		joinpath(dirname, entry->d_name);
		stat(entry->d_name, &finfo);

		if ((finfo.st_mode & S_IFMT) != S_IFDIR) {
			unlink(entry->d_name);
		} else removedir(dirname);

		*nameend = '\0';
	}
}
