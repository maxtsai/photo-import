#ifndef _CORE_OPS_H_
#define _CORE_OPS_H_

#include "format.h"

_Bool register_format(struct file_format *format);

void prepare_core();
_Bool scan(char *path, struct list_head *list);

#endif
