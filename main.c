#include <stdio.h>

/* PROGRAM MODE DEFS */
#define PM_ARC 1
#define PM_EXT 2


void archive(char *[], int);
int extract(char *);

int main(int argc, char *argv[]) {
	char program_mode = PM_ARC;

	while (--argc > 0 && **++argv == '-') switch (*++*argv) {
		case 'e':
			program_mode = PM_EXT;
			break;
		default:
			printf("Unknown argument: %s\n", *argv);
			return 0;
	}
	if (argc <= 0) {
		printf("No files specified.\n");
		return 0;
	}

	if (program_mode & PM_ARC)
		archive(argv, argc);
	else if (program_mode & PM_EXT)
		while (argc-- > 0) {
			printf("Extracting %s\n", *argv);
			extract(*argv++);
		}
	return 0;
}
