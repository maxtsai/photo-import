#ifndef FILE_FORMAT__H
#define FILE_FORMAT__H

struct file_format {
	char *name;
	struct file_operation *fops;
};

struct file_operation {
	int check(struct file_format*, char *fname);
};

#endif
