#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include "core_ops.h"

static struct list_head formats;

_Bool register_format(struct file_format *format)
{
	assert(format);
	if (format->name == NULL || format->fops == NULL)
		return false;
	list_add(&format->head, &formats);
	return true;
}

void prepare_core()
{
	INIT_LIST_HEAD(&formats);
}

_Bool scan()
{
	struct file_format *entry, *next;

	if (list_empty(&formats)) {
		printf("Can't handle any format!\n");
		return false;
	}
	list_for_each_entry_safe(entry, next, &formats, head) {
	}
}
