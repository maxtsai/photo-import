#ifndef _FILE_FORMAT_H_
#define _FILE_FORMAT_H_

#include "list.h"

#define MAX_NAME 255
#define MAX_PATH 1024

/* record file name */
#define RECORD_FILE ".import_index"

struct format {
	/* list head shall declair by the 1st member,
	 * since we don't want to use 'typeof' extended by compiler
	 */
	struct list_head head;
	char *name; /* < 8 bytes */
	struct format_operation *fops;
};

struct file_info {
	/* list head shall declair by the 1st member,
	 * since we don't want to use 'typeof' extended by compiler
	 */
	struct list_head head;
	char name[MAX_NAME];
	char copied_fname[MAX_NAME];
	char format[8];
	_Bool duplicated;
};

struct format_operation {
	_Bool (*check) (struct format*, char *fname);
	_Bool (*get_copied_fname) (struct format*, char *fname, char *cfname);
};

#endif
