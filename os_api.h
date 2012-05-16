#ifndef _OS_API_H_
#define _OS_API_H_

#include "list.h"

_Bool get_dir_contents(char *, struct list_head *);
_Bool get_file_time(char *, char *);

#endif
