#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include "core_ops.h"

/* supported format */
extern _Bool jpeg_init();


_Bool prepare()
{
	prepare_core();
	if (jpeg_init() == false)
		return false;
	
	return true;
}

