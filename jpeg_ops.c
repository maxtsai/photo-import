#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#include "core_ops.h"
#include "os_api.h"

#define DEBUG 0
#if DEBUG
#define dprintf printf
#else
#define dprintf(fmt, a...)
#endif

static _Bool jpeg_check(struct format*, char *);
_Bool jpeg_get_copied_fname (struct format*, char *, char *);

struct format_operation jpeg_fops = {
	.check	= jpeg_check,
	.get_copied_fname = jpeg_get_copied_fname,
};

struct format jpeg_format = {
	.name			= "JPEG",
	.fops			= &jpeg_fops,
};

enum {
	INTEL		= true,
	MOTOROLA	= false,
};

#define APP0	0xe0ff /* JFIF */
#define APP1	0xe1ff /* EXIF */
#define APP2	0xe2ff

static inline void byte_swap(_Bool endian, unsigned char *ptr, int len)
{                               
	unsigned char tmp;
	if (endian == INTEL)
		return;
	assert(ptr);
	assert((len>0) || (len%2 == 0));
	for (int i = 0; i < len-1; i+=2) {
		tmp = ptr[i];
		ptr[i] = ptr[i+1];
		ptr[i+1] = tmp;
	}
}

static inline void word_swap(_Bool endian, unsigned char *ptr, int len)
{
	unsigned char tmp;
	if (endian == INTEL)
		return;
	assert(ptr);
	assert((len>0) || (len%4 == 0));
	for (int i = 0; i < len-1; i+=4) {
		tmp = ptr[i];
		ptr[i] = ptr[i+2];
		ptr[i+2] = tmp;
		tmp = ptr[i+1];
		ptr[i+1] = ptr[i+3];
		ptr[i+3] = tmp;
	}
}

/* simple search */
static _Bool exif(FILE *fp, unsigned short tag, void *content, unsigned short content_len, long marker_start_pos)
{
	_Bool endian = MOTOROLA;
	unsigned short rtag, format, markers;
	unsigned int len, offset;
	unsigned char buf[16];
	long pos;

	assert(fp && content);
	assert((tag > 0) && (content_len > 0));

	fseek(fp, marker_start_pos, SEEK_SET);
	if (fread(buf, 1, 1, fp) != 1)
		goto fault;
	if (buf[0] == 0x49)
		endian = INTEL;
	fseek(fp, 7, SEEK_CUR);
	if (fread(&markers, 1, 2, fp) != 2)
		goto fault;
	byte_swap(endian, (unsigned char*) &markers, 2);

	for (int i = 0; i < markers; i++) {
		if (fread(&rtag, 1, 2, fp) != 2)
			goto fault;
		if (fread(&format, 1, 2, fp) != 2)
			goto fault;
		if (fread(&len, 1, 4, fp) != 4)
			goto fault;
		if (fread(&offset, 1, 4, fp) != 4)
			goto fault;
		byte_swap(endian, (unsigned char*) &rtag, 2);
		byte_swap(endian, (unsigned char*) &format, 2);
		byte_swap(endian, (unsigned char*) &len, 4);
		byte_swap(endian, (unsigned char*) &offset, 4);
		word_swap(endian, (unsigned char*) &len, 4);
		word_swap(endian, (unsigned char*) &offset, 4);

		dprintf("(tag, format, len, offset) = %x, %x, %x, %x\n", rtag, format, len, offset);

		if (rtag == 0x8769) {
			unsigned short tmp;
			fseek(fp, offset + marker_start_pos, SEEK_SET);
			if (fread(&tmp, 1, 2, fp) != 2)
				goto fault;
			byte_swap(endian, (unsigned char*) &tmp, 2);
			markers += tmp;
			continue;
		} else if ((format == 2) && (rtag == tag)) { /* only return ascii string */
			pos = ftell(fp);
			fseek(fp, offset + marker_start_pos, SEEK_SET);
			if (fread(content, 1, len, fp) != len)	
				goto fault;
			dprintf("\t content = %s\n", (char *) content);
			fseek(fp, pos, SEEK_SET);
			return true;
		}
	}

	dprintf("markers = %d\n", markers);
fault:
	return false;
}

_Bool jpeg_get_ifd(char *fname, unsigned short tag, void *content, unsigned short content_len)
{
	FILE *fp;
	unsigned short app;
	_Bool ret;
	long marker_start_pos = 0xc;

	assert(content);
	assert(content_len > 0 && tag > 0);

	if ((fp = fopen(fname, "rb")) == NULL) {
		printf("%s: %s [%s]\n", __FUNCTION__, strerror(errno), fname);
		return false;
	}
	fseek(fp, 0x2, SEEK_SET);
	if (fread(&app, 1, 2, fp) != 2)
		goto fault;
	if (app == APP0) { /* check if contain EXIF later */
		fseek(fp, 0x14, SEEK_SET);
		if (fread(&app, 1, 2, fp) != 2)
			goto fault;
		marker_start_pos = 0x1e;
		dprintf("app = %x\n", app);
	}
	if (app != APP1) {
		printf("\tNot support %s [%s]\n", app == APP0 ? "JFIF" : "APP2", fname);
		goto fault;
	}
	ret = exif(fp, tag, content, content_len, marker_start_pos);

	fclose(fp);
	return ret;
fault:
	fclose(fp);
	return false;
}

_Bool jpeg_get_copied_fname(struct format *format, char *fname, char *cfname)
{
	/* tag = 0x9003 -> Date and Time (Original) */
	_Bool ret = jpeg_get_ifd(fname, 0x9003, cfname, MAX_NAME);
	if (ret) {
		for (int i = 0; (cfname[i] != '\0') && (i < MAX_NAME); i++) {
			if ((cfname[i] == ' ') || (cfname[i] == ':')) {
				cfname[i] = '-';
			}
		}
		strcat(cfname, ".jpg");
		dprintf("\tcfname = '%s' [%s]\n", cfname, fname);
	}
	return ret;
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
        if (fread((void*) maker, 1, 2, fp) != 2)
		goto fault;
        if ((maker[1] != 0xd8) || (maker[0] != 0xff))
		goto fault1;

        fseek(fp, -2, SEEK_END);
        if (fread((void*) maker, 1, 2, fp) != 2)
		goto fault;
        if ((maker[1] != 0xd9) || (maker[0] != 0xff))
		goto fault2;

        fclose(fp);
	return true;
fault2:
	printf("\tfile broken? (no EOI) [%s]\n", fname);
fault1:
	dprintf("\t\tmaker = 0x%x%x\n", maker[0], maker[1]);
fault:
        fclose(fp);
        return false;
}


_Bool jpeg_init()
{
	return register_format(&jpeg_format);
}
