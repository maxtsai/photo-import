#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include "core_ops.h"

/* supported format */
extern _Bool jpeg_init() __attribute__((weak));
extern _Bool mov_init() __attribute__((weak));
extern _Bool avi_init() __attribute__((weak));

static _Bool SCAN_ONLY;
static int job_num;

#define MAX_PATH 1024
struct _folder_list {
	struct list_head head;
	struct list_head file_head;
	char path[MAX_PATH];
};

_Bool prepare()
{
	prepare_core();

	if (!jpeg_init) {
		printf("Need to support JPEG!\n");
		return false;
	}
	if (jpeg_init() == false) {
		printf("Fail to register JPEG format\n");
		return false;
	}
	if (mov_init && mov_init() == false) {
		printf("Fail to register MOV format\n");
		return false;
	}
	if (avi_init && avi_init() == false) {
		printf("Fail to register AVI format\n");
		return false;
	}
	
	return true;
}

void usage()
{
	printf("Usage: import [-s | -jn]... SOURCE... DIRECTORY\n"
			"import media files from SOURCE to DEST, or multiple SOURCE(s) to DIRECTORY\n\n"
			"-s		only scan without import, generate scan result\n"
			"-jn		n means the number of jobs running simultaneously, n >= 1\n"
			"\n");
}

int main(int argc, char **argv)
{
	int i;

	SCAN_ONLY = false;
	job_num = 1;

	if (argc < 3) { usage(); return 0; }
	if (argc > 3) {
		char *endptr;
		for (i = 0; i < argc; i++) {
			if (argv[i][0] == '-') {
				if (argv[i][1] == 's' || argv[i][1] == 'S')
					SCAN_ONLY = true;
				if (argv[i][1] == 'j' || argv[i][1] == 'J') {
					errno = 0;
					job_num = strtol(&argv[i][2], &endptr, 10);
					if ((errno == ERANGE && (job_num == LONG_MAX || job_num == LONG_MIN))
							|| (errno != 0 && job_num == 0)
							|| (job_num < 1)
							|| (job_num > 32)
							|| (&argv[i][2] == endptr)) {
						printf("-j with invalid number, suggest -j4?\n");
						return 0;
					}

				}
			}
		}
	}
	return 0;
}
















