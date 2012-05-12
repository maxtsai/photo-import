#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include "core_ops.h"

static _Bool jpeg_check(struct file_format*, char *);
static _Bool jpeg_scan(struct file_format*, char *, struct list_head *);

struct format_operation jpeg_fops = {
	.check	= jpeg_check,
	.scan	= jpeg_scan,
};

struct file_format jpeg_format = {
	.name			= "JPEG",
	.record_filename	= ".jpeg_index",
	.fops			= &jpeg_fops,
};

static void byte_swap(char *ptr, int len)
{                               
	char tmp;
	int i;
	assert(ptr);
	assert((len>0) || (len%2 == 0));
	for (i = 0; i < len-1; i+=2) {
		tmp = ptr[i];
		ptr[i] = ptr[i+1];
		ptr[i+1] = tmp;
	}
}    

#define TIFF_HEADER_OFF		12
#define EOI			0xffd9
#define SOI			0xffd8
static _Bool jpeg_check(struct file_format* format, char *fname)
{
        FILE *fp;
        char maker;
        int ret;
        unsigned short endian;

	assert(format);
	assert(fname);

        if ((fp = fopen(fname, "r")) == NULL) {
                perror("fopen");
		return false;
        }
        fseek(fp, TIFF_HEADER_OFF, SEEK_SET);
        ret = fread((void*) &endian, 1, sizeof(endian), fp);

        fseek(fp, 0, SEEK_SET);
        if (fread((void*) &maker, 1, sizeof(maker), fp) != 2)
		goto fault;
        byte_swap(&maker, sizeof(maker));
        if (maker != SOI)
		goto fault;
        fseek(fp, -2, SEEK_END);
        if (fread((void*) &maker, 1, sizeof(maker), fp) != 2)
		goto fault;
        byte_swap(&maker, sizeof(maker));
        if (maker != EOI)
		goto fault;
        fclose(fp);
	return true;
fault:
        fclose(fp);
        return false;
}

static _Bool jpeg_scan(struct file_format *format, char *path, struct list_head *result)
{
	assert(format);
	assert(result);

	/*
	struct file_info *finfo = malloc(sizeof(struct file_info));
	memset(finfo, 0, sizeof(struct file_info));
	*/

	printf("[%s:%d] under construction (list = %p)\n", __FUNCTION__, __LINE__, result);
	return true;
}

_Bool jpeg_init()
{
	return register_format(&jpeg_format);
}