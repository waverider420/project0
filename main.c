#include "std/outstr.h"


/* PROGRAM MODE DEFS */
#define PM_ARC 1
#define PM_EXT 2

void print_usage_info(char *);

void archive(char *[], int);
int extract(char *);

int main(int argc, char *argv[]) {
	char program_mode = PM_ARC;
	char *prog_name = *argv;

	while (--argc > 0 && **++argv == '-') switch (*++*argv) {
		case 'e':
			program_mode = PM_EXT;
			break;
		case 'h':
			print_usage_info(prog_name);
			return 0;
		default:
			outputstr(bstdout, "Unknown argument: *s. Help is available with *s -h\n", *argv, prog_name);
			return 0;
	}
	if (argc <= 0) {
		outputstr(bstderr, "No files specified\n");
		exit(EXIT_FAILURE);
	}

	if (program_mode & PM_ARC)
		archive(argv, argc);
	else if (program_mode & PM_EXT)
		while (argc-- > 0) {
			extract(*argv++);
			outputstr(bstdout, "Extracted *s\n", *argv);
		}

	return 0;
}

void print_usage_info(char *prog_name) {
	outputstr(bstdout, "Of course, here's some help:\nTo archive files and directories:\n\t*s [file(s)]\n\nTo extract an archive file:\n\t*s -e [archive_name]\n\nTo get out of the Matrix, you have to revert your brain back to factory settings and stop outsourcing thought.\n", prog_name, prog_name);
}
