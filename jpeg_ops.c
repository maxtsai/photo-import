#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include "core_ops.h"
#include "os_api.h"

static _Bool jpeg_check(struct file_format*, char *);

struct format_operation jpeg_fops = {
	.check	= jpeg_check,
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
        unsigned short maker;
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
        byte_swap((char*) &maker, sizeof(maker));
        if (maker != SOI)
		goto fault;
        fseek(fp, -2, SEEK_END);
        if (fread((void*) &maker, 1, sizeof(maker), fp) != 2)
		goto fault;
        byte_swap((char*) &maker, sizeof(maker));
        if (maker != EOI)
		goto fault;
        fclose(fp);
	return true;
fault:
        fclose(fp);
        return false;
}

_Bool jpeg_init()
{
	return register_format(&jpeg_format);
}
