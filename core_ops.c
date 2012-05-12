#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include "core_ops.h"
struct list_head format_head;
_Bool core_is_ready = false;

_Bool register_format(struct file_format *format)
{
	assert(format);
	if (format->name == NULL || format->fops == NULL)
		return false;
	list_add(&format->head, &format_head);
	return true;
}

void prepare_core()
{
	INIT_LIST_HEAD(&format_head);
	core_is_ready = true;
}

_Bool scan(char *path, struct list_head *list)
{
	struct file_format *entry, *next;
	_Bool ret;

	assert(path);
	assert(list);

	if (core_is_ready == false || list_empty(&format_head)) {
		printf("Not any format supported!\n");
		return false;
	}
	list_for_each_entry_safe(entry, next, struct file_format, &format_head, head) {
		if (entry->fops->scan)
			ret = entry->fops->scan(entry, path, list);
		if (ret == false) {
			printf("Scan for %s format error!\n", entry->name);
			return false;
		}
	}
	return true;
}

