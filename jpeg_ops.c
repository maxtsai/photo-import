#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#include "core_ops.h"
#include "os_api.h"

//#define DEBUG
#ifdef DEBUG
#define dprintf printf
#else
#define dprintf(fmt, a...)
#endif

static _Bool jpeg_check(struct format*, char *);
static _Bool jpeg_get_copied_fname (struct format*, char *, char *);

struct format_operation jpeg_fops = {
	.check	= jpeg_check,
	.get_copied_fname = jpeg_get_copied_fname,
};

struct format jpeg_format = {
	.name			= "JPEG",
	.fops			= &jpeg_fops,
};

static inline void byte_swap(unsigned char *ptr, int len)
{                               
	unsigned char tmp;
	assert(ptr);
	assert((len>0) || (len%2 == 0));
printf("### [%s:%d]\n", __FUNCTION__, __LINE__);
	for (int i = 0; i < len-1; i+=2) {
		tmp = ptr[i];
		ptr[i] = ptr[i+1];
		ptr[i+1] = tmp;
	}
}    

/* simple search */
_Bool jpeg_get_ifd(char *fname, unsigned short tag, void *content, unsigned short content_len)
{
	FILE *fp;
	_Bool swap = false;
	unsigned char buf[256];
	int markers = 0;
	unsigned short rtag, format, len, offset;
	int i;

	assert(content);
	assert(content_len > 0 && tag > 0);

	if ((fp = fopen(fname, "rb")) == NULL) {
		printf("%s: %s [%s]\n", __FUNCTION__, strerror(errno), fname);
		return false;
	}

	fseek(fp, 0xc, SEEK_SET);
	fread(buf, 1, 1, fp);
	if (buf[0] == 0x4d)
		swap = true;
	fseek(fp, 3, SEEK_CUR);
	printf("byte_align = 0x%c\n", buf[0]);
	fread(buf, 1, 2, fp);
	if (swap) byte_swap(buf, 2);
	for (i = 0; i < 2; i++)
		printf("%x ", buf[i]);
	printf("\n");
	printf("shift %d bytes\n", (buf[1] << 8) + buf[0]);

	fseek(fp, (buf[1] << 8) + buf[0] - 6, SEEK_CUR);
	fread(buf, 1, 2, fp);
	if (swap) byte_swap(buf, 2);
	markers = buf[0] + (buf[1] << 8);
	printf("markers = %d\n", markers);

	for (int j = 0; j < markers; j++) {
		fread(&rtag, 1, 2, fp);
		if (swap) byte_swap((unsigned char*) &rtag, 2);
		fread(&format, 1, 2, fp);
		if (swap) byte_swap((unsigned char*) &format, 2);
		fread(&len, 1, 2, fp);
		if (swap) byte_swap((unsigned char*) &len, 2);
		fseek(fp, 2, SEEK_CUR);
		fread(&offset, 1, 2, fp);
		if (swap) byte_swap((unsigned char*) &offset, 2);
		fseek(fp, 2, SEEK_CUR);

		printf("tag, format, len, offset= 0x%x, 0x%x, 0x%x, 0x%x\n", rtag, format, len, offset);
	}

	fclose(fp);
	return true;
}

static _Bool jpeg_check(struct format* format, char *fname)
{
        FILE *fp;
	unsigned char maker[2];

	assert(format);
	assert(fname);

        if ((fp = fopen(fname, "r")) == NULL) {
		printf("\t%s [%s]\n", strerror(errno), fname);
		return false;
        }

	dprintf("\tcheck '%s'\n", fname);

        fseek(fp, 0, SEEK_SET);
        if (fread((void*) maker, 1, 2, fp) != 2) {
		dprintf("\t\t%s:%d, fread error (%s)\n", __FUNCTION__, __LINE__, strerror(errno));
		goto fault;
	}
        if ((maker[1] != 0xd8) || (maker[0] != 0xff))
		goto fault1;

        fseek(fp, -2, SEEK_END);
        if (fread((void*) maker, 1, 2, fp) != 2) {
		dprintf("\t\t%s:%d, fread error (%s)\n", __FUNCTION__, __LINE__, strerror(errno));
		goto fault;
	}
        if ((maker[1] != 0xd9) || (maker[0] != 0xff))
		goto fault2;

        fclose(fp);
	return true;
fault2:
	printf("\tfile is not complete [%s]\n", fname);
fault1:
	dprintf("\t\tmaker = 0x%x%x\n", maker[0], maker[1]);
fault:
        fclose(fp);
        return false;
}

_Bool jpeg_get_copied_fname(struct format *format, char *fname, char *cfname)
{

	return true;
}

_Bool jpeg_init()
{
	return register_format(&jpeg_format);
}
