#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#include "core_ops.h"
#include "os_api.h"

struct list_head format_head;
_Bool core_is_ready = false;

_Bool register_format(struct format *format)
{
	assert(format);
	if (format->name == NULL || format->fops == NULL)
		return false;
	list_add_tail(&format->head, &format_head);
	return true;
}

void prepare_core()
{
	INIT_LIST_HEAD(&format_head);
	core_is_ready = true;
}

_Bool scan_dir(char *path, struct list_head *list)
{
	assert(path);
	assert(list);

	if (core_is_ready == false || list_empty(&format_head)) {
		printf("Not any format supported!\n");
		return false;
	}
	return get_dir_contents(path, list);
}

_Bool check_format(char *root, struct list_head *file_head)
{
	struct format *entry, *next;
	char fname[MAX_PATH];

	list_for_each_entry_safe(entry, next, struct format, &format_head, head) {
		struct file_info *fentry, *fnext;
		list_for_each_entry_safe(fentry, fnext, struct file_info, file_head, head) {
			if (entry->fops->check) {
				strncpy(fname, root, MAX_PATH);
				strcat(fname, "/");
				strncat(fname, fentry->name, MAX_NAME);
				//printf("%s: check '%s'\n", __FUNCTION__, fname);
				if (entry->fops->check(entry, fname))
					strcpy(fentry->format, entry->name);
			}
		}
	}
	return true;
}

_Bool save(char *path, struct list_head *file_head)
{
	struct file_info *fentry, *fnext;
	char fname[MAX_PATH];
	FILE *fp;

	strncpy(fname, path, MAX_PATH);
	strcat(fname, "/");
	strcat(fname, RECORD_FILE);

	fp = fopen(fname, "w");
	if (!fp) {
		printf("unable to open '%s' (%s)", fname, strerror(errno));
		return false;
	}

	list_for_each_entry_safe(fentry, fnext, struct file_info, file_head, head) {
		if (fentry->format[0] != '\0')
			fprintf(fp, "%s\t%s\n", fentry->format, fentry->name);
	}

	fclose(fp);
	return true;
}

_Bool load()
{

	return true;
}

