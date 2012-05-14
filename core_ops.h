#ifndef _CORE_OPS_H_
#define _CORE_OPS_H_

#include "format.h"

struct list_head format_head;

void prepare_core();
_Bool scan_dir(char *path, struct list_head *list);
_Bool register_format(struct file_format *format);
_Bool check_format(char *, struct list_head *);


#endif
