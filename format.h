#ifndef _FILE_FORMAT_H_
#define _FILE_FORMAT_H_

#include "list.h"

struct file_format {
	struct list_head head;
	char *name;
	char *record_filename;
	struct format_operation *fops;
};

struct format_operation {
	_Bool (*check) (struct file_format*, char *);
	_Bool (*scan) (struct file_format*);
};

#endif
