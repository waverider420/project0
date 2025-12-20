#include "archive.h"
#include "ext_decs.h"

int extract(char *arch_name) {
	DIRS_SIZE dirs_size;
	struct stat finfo;
	IOBUF *arch_fp;
	Memdir root;

	if ((arch_fp = bopen(arch_name, IOBM_RO)) == NULL) {
		printf("Error: can't open %s. Does the file exist?\n", arch_name);
		return -1;
	}

	stat(arch_name, &finfo);
	arch_size = finfo.st_size;
	printf("size of archive is %ld\n", arch_size);

	/* Read DIRS_SIZE */
	breads(arch_fp, (char *) &dirs_size, sizeof dirs_size);

	/* Reconstruct directory structure */
	if (reconstruct(&root, dirs_size, arch_fp) < 0) {
		printf("Couldn't complete extraction\n");
		return -1;
	}

	/* Write everything */
	joinpath(cpath, root.name);
	if (bring2system(&root, arch_fp) < 0) {
		printf("Extraction failed. Terminating.\n");
		return -1;
	}

	return 0;
}

int bring2system(Memdir *mdp, IOBUF *arch_fp) {
	char *cnamebord;

	if (mkdir(cpath, 0777) < 0) {
		printf("Error. Can't create directory %s\n", cpath);
		return -1;
	}

	/* Set permissions */
	if (chmod(cpath, mdp->perms) < 0)
		printf("Can't set permissions for directory %s. This error is ignored. [Error code %d]\n", mdp->name, errno);

	/* Loop over entries */
	Memdir  *dlistp;
	Memfile *flistp;
	unsigned i;

	dlistp = mdp->dlist;
	flistp = mdp->flist;
	cnamebord = cpath + cstrlen(cpath);
	for (i = 0; i < mdp->ndirs; i++) {
		joinpath(cpath, dlistp->name);
		printf("calling bring2system with cpath = %s\n", cpath);
		if (bring2system(dlistp++, arch_fp) < 0) return -1;
		*cnamebord = '\0';
	}
	for (i = 0; i < mdp->nfiles; i++) {
		joinpath(cpath, flistp->name);
		printf("dir %s has a file %s\n", mdp->name, flistp->name);
		if (createfile(flistp++, arch_fp) < 0) return -1;
		*cnamebord = '\0';
	}

	return 0;
}

int createfile(Memfile *mfp, IOBUF *arch_fp) {
	IOBUF *fp;

	if ((fp = bopen(cpath, IOBM_WO)) == NULL) {
		printf("Error. Can't create file %s\n", mfp->name);
		return -1;
	}

	/* Set permissions */
	if (chmod(cpath, mfp->perms) < 0)
		printf("Can't set permissions for file %s. This error is ignored. [Error code %d]\n", cpath, errno);

	/* Copy contents */
	FILE_SIZE i;
	long savedpos;
	int c;

	if (mfp->size > 0) {
		savedpos = btell(arch_fp);
		bseek(arch_fp, mfp->offset, 0);

		for (i = 0; i++ < mfp->size; writec(fp, readc(arch_fp)));
		bseek(arch_fp, savedpos, 0);
	}

	bclose(fp);
	return 0;
}

int reconstruct(Memdir *mdp, DIRS_SIZE dirs_size, IOBUF *arch_fp) {
	/* Get name */
	long savedpos;
	int name_len;
	char c, *p;

	savedpos = btell(arch_fp);
	for (name_len = 0; (c = readc(arch_fp)) != '\0'; name_len++);
	if (name_len > __DARWIN_MAXNAMLEN) {
		printf("Error. %s is likely corrupted (directory name too large).\n", arch_fp->name);
		return -1;
	}
	if ((mdp->name = malloc(name_len + 1)) == NULL) {
		printf("Error. Can't allocate space (%d bytes). Sure you have enough free RAM?\n", name_len);
		return -1;
	}
	bseek(arch_fp, savedpos, 0);
	breads(arch_fp, mdp->name, name_len +1);

	printf("Successfully extracted name %s\n", mdp->name);
	/* Get permissions */
	if (breads(arch_fp, (char *) &mdp->perms, PERMS_SIZE) != PERMS_SIZE) {
		printf("Error (134). %s is likely corrupted (can't read directory entry).\n", arch_fp->name);
		return -1;
	}

	/* Get the size of offsets */
	OFFSET_LIST offset_list_size, offseti;
	OFFSET_SIZE offset;

	if (breads(arch_fp, (char *) &offset_list_size, sizeof offset_list_size) != sizeof offset_list_size ||
	    offset_list_size % sizeof offset != 0) {
		printf("Error (144). %s is likely corrupted (can't read directory entry).\n", arch_fp->name);
		return -1;
	}

	mdp->ndirs = mdp->nfiles = 0;
	if (offset_list_size == 0) {
		mdp->dlist = NULL;
		mdp->flist = NULL;

		return 0;
	}

	/* Append counts of directories and files to mdp */
	savedpos = btell(arch_fp);

	for (offseti = 0; offseti < offset_list_size; offseti += sizeof offset) {
		if (breads(arch_fp, (char *) &offset, sizeof offset) != sizeof offset) {
			printf("Error (155). %s is likely corrupted (can't read directory entry).\n", arch_fp->name);
			return -1;
		}
		if (offset < (dirs_size + sizeof dirs_size)) mdp->ndirs++;
		else if (offset < arch_size) mdp->nfiles++;
		else {
			printf("Error. %s is likely corrupted (invalid offset value \"%lu\").\n", arch_fp->name, offset);
			return -1;
		}
	}
	bseek(arch_fp, savedpos, 0);
	printf("Directory entry %s contains %d files and %d other dirs\n", mdp->name, mdp->nfiles, mdp->ndirs);

	/* Allocate space for pointers */
	Memdir  *dlistp;
	Memfile *flistp;

	if (mdp->ndirs > 0) {
		if ((mdp->dlist = dlistp = (Memdir *) malloc(mdp->ndirs * sizeof (Memdir))) == NULL) {
			printf("Error. Can't allocate space (%lu bytes). Sure you have enough free RAM?\n", mdp->ndirs * sizeof (Memdir));
			return -1;
		}
	} else mdp->dlist = dlistp = NULL;

	if (mdp->nfiles > 0) {
		if ((mdp->flist = flistp = (Memfile *) malloc(mdp->nfiles * sizeof (Memfile))) == NULL) {
			printf("Error. Can't allocate space (%lu bytes). Sure you have enough free RAM?\n", mdp->nfiles * sizeof (Memfile));
			return -1;
		}
	} else mdp->flist = flistp = NULL;
	printf("offset_list_size: %d\toffseti: %d\n", offset_list_size, offseti);

	/* Assign eash pointer a corresponding struct */
	for (; offseti > 0; offseti -= sizeof offset) {
		breads(arch_fp, (char *) &offset, sizeof offset);
		printf("offset is %lu\n", offset);

		savedpos = btell(arch_fp);
		bseek(arch_fp, offset, 0);
		if (offset < (dirs_size + sizeof dirs_size)) {
			if (reconstruct(dlistp++, dirs_size, arch_fp) < 0) return -1;
		} else {
			if (reconst_frec(flistp++, arch_fp) < 0) return -1;
	 	}
		bseek(arch_fp, savedpos, 0);
	}

	return 0;
}

int reconst_frec(Memfile *mfp, IOBUF *arch_fp) {
	/* Get name */
	long savedpos;
	int name_len, tmp;
	char c;

	savedpos = btell(arch_fp);
	for (name_len = 0; (c = readc(arch_fp)) != '\0'; name_len++);
	bseek(arch_fp, savedpos, 0);
	printf("Calculated namelen and rewound %d characters back to %lu\n", name_len, btell(arch_fp));

	if ((mfp->name = (char *) malloc(name_len)) == NULL) {
		printf("Error. Can't allocate space (%d bytes). Sure you have enough free RAM?\n", name_len);
		return -1;
	}
	breads(arch_fp, mfp->name, name_len + 1);
	printf("Extracted filename %s\n", mfp->name);

	/* Get permissions */
	if ((tmp = breads(arch_fp, (char *) &mfp->perms, PERMS_SIZE)) != PERMS_SIZE) {
		printf("Error (221). %s is likely corrupted (can't read directory entry). Read %d bytes instead of %d\n", arch_fp->name, tmp, PERMS_SIZE);
		return -1;
	}

	/* Get file size */
	if (breads(arch_fp, (char *) &mfp->size, sizeof mfp->size) != sizeof mfp->size) {
		printf("Error (227). %s is likely corrupted (can't read file size).\n", arch_fp->name);
		return -1;
	}

	/* Set file content offset */
	mfp->offset = btell(arch_fp);

	return 0;
}
