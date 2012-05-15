#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include "core_ops.h"

#define DEBUG 0
#if DEBUG
#define dprintf printf
#else
#define dprintf(fmt, a...)
#endif

/* supported format */
extern _Bool jpeg_init() __attribute__((weak));
extern _Bool mov_init() __attribute__((weak));
extern _Bool avi_init() __attribute__((weak));

/* options */
static _Bool scan_only;
static int job_num;

struct _folder_list {
	struct list_head head;
	struct list_head file_head;
	char path[MAX_PATH];
};
LIST_HEAD(folder_head);

static _Bool prepare()
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

static void usage()
{
	printf("Usage: import [-s | -jn]... SOURCE... DIRECTORY\n"
			"import media files from SOURCE to DEST, or multiple SOURCE(s) to DIRECTORY\n\n"
			"-s		only scan without import, generate scan result\n"
			"-jn		n means the number of jobs running simultaneously, n >= 1\n"
			"\n");
}
static void arg_parser(int argc, char **argv)
{
	int i;
	scan_only = false;
	job_num = 1;
	int count = 0;

	if (argc < 3) { usage(); exit(1); }
	if (argc > 3) {
		char *endptr;
		for (i = 0; i < argc; i++) {
			if (argv[i][0] == '-') {
				if (argv[i][1] == 's' || argv[i][1] == 'S') {
					scan_only = true;
					count++;
				}
				if (argv[i][1] == 'j' || argv[i][1] == 'J') {
					errno = 0;
					count++;
					job_num = strtol(&argv[i][2], &endptr, 10);
					if ((errno == ERANGE && (job_num == LONG_MAX || job_num == LONG_MIN))
							|| (errno != 0 && job_num == 0)
							|| (job_num < 1)
							|| (job_num > 32)
							|| (&argv[i][2] == endptr)) {
						printf("-j with invalid number, suggest -j4?\n");
						exit(1);
					}
					printf("multi-jobs is not supported yet.\n");
				}
			}
		}
		if (argc - count < 2) {
			usage();
			exit(1);
		}
	}

	for (i = count + 1; i < argc; i++) {
		struct _folder_list *fl = malloc(sizeof(struct _folder_list));
		INIT_LIST_HEAD(&fl->file_head);
		strncpy(fl->path, argv[i], MAX_PATH);
		list_add_tail(&fl->head, &folder_head);
	}
}

static void cleanup()
{
	struct _folder_list *entry, *next;	
	list_for_each_entry_safe(entry, next, struct _folder_list, &folder_head, head) {
		dprintf("\t'%s'\n", entry->path);
		if (!list_empty(&entry->file_head)) {
			struct file_info *fentry, *fnext;
			list_for_each_entry_safe(fentry, fnext, struct file_info, &entry->file_head, head) {
				dprintf("\t\t'%s'\t\t%s\n", fentry->name, fentry->format);
				list_del(&fentry->head);
				free(fentry);
			}
		}
		list_del(&entry->head);
		free(entry);
	}
}

extern _Bool jpeg_get_ifd(char *fname, unsigned int tag, void *content, int len);

int main(int argc, char **argv)
{
	//arg_parser(argc, argv);

	if (prepare() == false)
		exit(1);

#if 1/* MaxTsai debugs 2012-05-15 */
	unsigned char test[256];
	jpeg_get_ifd(argv[1], 0x9003, test, sizeof(test));
#else
	if (scan_only) {
		struct _folder_list *entry, *next;	
		list_for_each_entry_safe(entry, next, struct _folder_list, &folder_head, head) {
			dprintf("scan '%s'\n", entry->path);
			if (scan_dir(entry->path, &entry->file_head) == false) {
				goto fault;
			}
			if (check_format(entry->path, &entry->file_head) == false)
				printf("some error during check format type\n");
			if (save(entry->path, &entry->file_head) == false)
				goto fault;
			if (load(entry->path, &entry->file_head) == false)
				goto fault;
		}
	}
#endif
fault:
	cleanup();
	return 0;
}


