#ifndef _FILE_FORMAT_H_
#define _FILE_FORMAT_H_

#include "list.h"

struct file_format {
	/* list head shall declair by the 1st member,
	 * since we don't want to use 'typeof' extended by compiler
	 */
	struct list_head head;
	char *name;
	char *record_filename;
	struct format_operation *fops;
};

struct file_info {
	/* list head shall declair by the 1st member,
	 * since we don't want to use 'typeof' extended by compiler
	 */
	struct list_head head;
	char *name;
	char *copied_fname;
	_Bool duplicated;
};

struct format_operation {
	_Bool (*check) (struct file_format*, char *fname);
	_Bool (*scan) (struct file_format*, char *path, struct list_head *list);
};

#endif
