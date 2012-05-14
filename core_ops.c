#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include "core_ops.h"
#include "os_api.h"

_Bool core_is_ready = false;

_Bool register_format(struct file_format *format)
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
	struct file_format *entry, *next;
	char fname[MAX_PATH];

	list_for_each_entry_safe(entry, next, struct file_format, &format_head, head) {
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

