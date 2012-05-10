#include <stdio.h>
#include <stdlib.h>

#include "file_format.h"

static int jpeg_check(struct file_format*, char *);

struct file_operation jpeg_fops {
	.check	= jpeg_check,
};

struct file_format jpeg_format {
	.name	= "JPEG",
	.fops	= &jpeg_fops,
};
