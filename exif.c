#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <linux/posix_types.h>
#include <stddef.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/stat.h>


#include "list.h"

#define NAME_MAX 255
#define PATH_MAX 1024

#define SOI	0xffd8
#define EOI	0xffd9

#define EXIF_HEADER_OFF		6
#define TIFF_HEADER_OFF		12
#define EXIF_BIG_ENDIAN		0x4d4d
#define EXIF_LITTLE_ENDIAN	0x4949

#define TAG_DATETIMEORIGINAL	0x9003

#define MAX_THREADS 10

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct jpg_file {
	struct list_head list;
	char filename[NAME_MAX];
	u_int32_t checksum;	
	char datetime[24];
};

struct _pthread_data_struct {
	char filename[NAME_MAX];
	char path[PATH_MAX];
	struct list_head *list;
	int used;
	pthread_t thread;
	int id;
};

static struct _pthread_data_struct pthread_data_struct[MAX_THREADS];
static int pthread_count[MAX_THREADS];

static int byte_swap(char *ptr, int len)
{
	int i;

	if ((len <= 0) || ((len % 2) != 0))
		return -EINVAL;
	for (i = 0; i < len; i+=2) {
		char tmp = ptr[i];
		ptr[i] = ptr[i+1];
		ptr[i+1] = tmp;
	}
	return 0;
}

static int get_file_time(char *filename, char *time)
{
	struct stat sb;
	struct tm *ft;

	if (!filename || !time)
		return -EINVAL;

	if (stat(filename, &sb) == -1) {
		perror("stat");
		return errno;
	}
	ft = localtime(&sb.st_mtime);
	strftime(time, 63, "%Y:%m:%d %H:%M:%S", ft);
	//printf("%s\n", time);
}

static int format_time2filename(char *time, int len)
{
	int i;
	if (!time)
		return -EINVAL;
	for (i = 0; i < len; i++)
		if ((time[i] == ':') || (time[i] == ' '))
			time[i] = '-';
	return 0;
}

static int copy_file(char *src, char *dest)
{
	char cmd[PATH_MAX];
	char tmp[PATH_MAX];
	FILE *fp;
	int i, j;

	memset(cmd, 0, sizeof(cmd));
	strcpy(cmd, "cp -a ");
	memset(tmp, 0, PATH_MAX);
	for (i = j = 0; i < strlen(src); i++, j++) {
		if (src[i] == ' ')
			tmp[j++] = '\\';
		tmp[j] = src[i];
	}
	strcat(cmd, tmp);
	strcat(cmd, " ");
	memset(tmp, 0, PATH_MAX);
	for (i = j = 0; i < strlen(dest); i++, j++) {
		if (src[i] == ' ')
			tmp[j++] = '\\';
		tmp[j] = dest[i];
	}
	strcat(cmd, tmp);

	//printf("cmd: %s\n", cmd);
	if ((fp = popen(cmd, "r")) == NULL) {
		perror("copy_file: popen");
		return -EFAULT;
	}
	pclose(fp);
	return 0;
}
/* Temp function based on md5sum */
static u_int32_t get_checksum(char *filename)
{
	char cmd[PATH_MAX];
	FILE *fp;
	size_t len = 0;
	char *line = NULL;
	char buf[64];
	int i, j;
	u_int32_t ret = 0;

	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "md5sum %s 2>/dev/null", filename);
	if ((fp = popen(cmd, "r")) == NULL) {
		perror("get_checksum: popen");
		return -EFAULT;
	}
	memset(buf, 0, sizeof(buf));
	len = fread(buf, 1, 8, fp);
	pclose(fp);

	for (i = len-1, j = 0; i >= 0; i--, j++) {
		if (buf[i] >= 'a')
			ret += ((buf[i]-'a'+10) << j*4);
		else
			ret += ((buf[i]-'0') << j*4);
	}
	//printf("checksum = %s (%x)\n", buf, ret);
	return ret;
}

/* Temp function based on exif */
int get_value_by_ids(int ids, char *value, char *filename)
{
	char cmd[PATH_MAX];
	FILE *fp;
	size_t len = 0;
	char *line = NULL;

	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "exif -t 0x%x %s 2>/dev/null", ids, filename);
	if ((fp = popen(cmd, "r")) == NULL) {
		perror("get_value_by_ids: popen");
		return -EFAULT;
	}
	while(getline(&line, &len, fp) != -1) {
		char *ptr;
		//printf("%s", line);
		if ((ptr = strstr(line, "Value")) != NULL) {
			strcpy(value, ptr+7);
			value[strlen(value)-1] = '\0';
			//printf("%s\n", value);
			goto out;
		}
	}
	value[0] = '\0';
out:
	free(line);
	pclose(fp);
	return 0;	
}

#if 0
int is_jpeg(char *filename)
{
	FILE *fp;
	u_int16_t maker = 0;
	int ret;
	u_int16_t endian = LITTLE_ENDIAN;

	if (filename == NULL)
		return -EINVAL;
	if ((fp = fopen(filename, "r")) == NULL) {
		perror("fopen");
		return errno;
	}

	fseek(fp, TIFF_HEADER_OFF, SEEK_SET);
	ret = fread((void*) &endian, 1, sizeof(endian), fp);

	fseek(fp, 0, SEEK_SET);
	if (fread((void*) &maker, 1, sizeof(maker), fp) != 2)
		return 0;
	byte_swap((char*) &maker, sizeof(maker));
	if (maker != SOI)
		return 0;
	fseek(fp, -2, SEEK_END);
	if (fread((void*) &maker, 1, sizeof(maker), fp) != 2)
		return 0;
	byte_swap((char*) &maker, sizeof(maker));
	if (maker != EOI)
		return 0;
	fclose(fp);

	return 1;
}
#else
int is_jpeg(char *filename)
{
	char cmd[128];
	FILE *fp;
	size_t len = 0;
	char *line = NULL;
	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "exif %s 2>/dev/null", filename);
	if ((fp = popen(cmd, "r")) == NULL) {
		perror("is_jpeg: popen");
		goto not_jpg;
	}
	getline(&line, &len, fp);
	//printf("%s, line=%s\n", filename, line);
	if ((line == NULL))
		goto not_jpg;
	if (strncmp(line, "EXIF", 4) != 0) {
		free(line);
		goto not_jpg;
	}
	return 1;
not_jpg:
	fclose(fp);
	return 0;
}
#endif


void *theThread(void *parm)
{
	int rc;
	DIR *dir;
	struct dirent *de;
	char tmp[64];
	struct _pthread_data_struct *pds;
	char filename_with_path[PATH_MAX];

	//pthread_detach(pthread_self());

	if (parm == NULL)
		goto out;

	pds = (struct _pthread_data_struct*) parm;

	memset(filename_with_path, 0, sizeof(filename_with_path));
	sprintf(filename_with_path, "%s/%s", pds->path, pds->filename);

	if (is_jpeg(filename_with_path) == 0) {
		goto out;
	}

	struct jpg_file *jfp = (struct jpg_file*) malloc(sizeof(struct jpg_file));

	if (jfp == NULL) {
		perror("malloc");
		goto out;
	}
	memset(jfp, 0, sizeof(struct jpg_file));

#if 0
	strcpy(jfp->filename, "TEST_FILENAME");
	strcpy(jfp->datetime, "TEST_DATE");
#else
	strcpy(jfp->filename, pds->filename);
	if ((jfp->checksum = get_checksum(filename_with_path)) < 0) {
		printf("%s checksum error!\n", filename_with_path);
		goto out;
	}
	memset(tmp, 0, sizeof(tmp));
	get_value_by_ids(TAG_DATETIMEORIGINAL, tmp, filename_with_path);
	if (strlen(tmp) == 0) {
		get_file_time(filename_with_path, tmp);
	}
	strncpy(jfp->datetime, tmp, 20);
	format_time2filename(jfp->datetime, 20);
#endif
	//printf("%s, %s\n", jfp->filename, jfp->datetime);

	rc = pthread_mutex_lock(&mutex);
	list_add(&jfp->list, pds->list);
	rc = pthread_mutex_unlock(&mutex);
out:
	pds->used = 0;
	//pthread_exit(NULL);
	return NULL;
}

struct _pthread_data_struct *pick_thread()
{
	int i;

	for (i = 0; i < MAX_THREADS; i++) {
		if (pthread_data_struct[i].used == 0) {
			pthread_count[i]++;
			return &pthread_data_struct[i];
		}
	}
	return NULL;
}

int travel_folder(char *path, struct list_head *list)
{
	struct _pthread_data_struct *pds;
	int rc;
	static int i;
	DIR *dir;
	struct dirent *de;
	pthread_attr_t attr;

	for (i = 0; i < MAX_THREADS; i++)
		pthread_count[i] = 0;

	dir = opendir(path);
	if (dir == NULL)
		return errno;
	while ((de = readdir(dir))) {
		for (;;) {
			pds = pick_thread();
			if (pds == NULL) {
				pthread_join(pthread_data_struct[0].thread, NULL);
			} else {
				strcpy(pds->filename, de->d_name);
				strcpy(pds->path, path);
				pds->used = 1;
				pds->list = list;
				/*
				pthread_attr_init (&attr);
				pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
				*/
				strcpy(pds->filename, de->d_name);
				rc = pthread_create(&pds->thread, NULL, theThread, (void*) pds);
				if (rc != 0)
					perror("pthread_create");
				break;
			}
		}
	}

	closedir(dir);
	return 0;
}

void main(int argc, char **argv)
{
	struct list_head *entry, *next;
	struct list_head *entry1, *next1;
	struct jpg_file *jf, *jf1;
	int i = 0;
	int found = 0;
	char tmp[PATH_MAX], tmp1[PATH_MAX];

	struct list_head src, dest;

	if (argc != 3)
		return;

	INIT_LIST_HEAD(&src);
	INIT_LIST_HEAD(&dest);

	printf("src=%s, dest=%s\n", argv[1], argv[2]);
	system("date");

	travel_folder(argv[1], &src);
	travel_folder(argv[2], &dest);
	for (i = 0; i < MAX_THREADS; i++) {
		pthread_join(pthread_data_struct[i].thread, NULL);
		printf("[%02d] used count = %d\n", i, pthread_count[i]);
	}

#if 1
	/* check if duplicate file exists */
	list_for_each_safe(entry, next, &src) {
		jf = list_entry(entry, struct jpg_file, list);
		found = 0;
		list_for_each_safe(entry1, next1, &dest) {
			jf1 = list_entry(entry1, struct jpg_file, list);
			if (jf->checksum == jf1->checksum) {
				found = 1;
				break;
			}
		}
		if (!found) {
			FILE *fp;
			int i;

			strcpy(tmp, argv[1]);
			strcat(tmp, "/");
			strcat(tmp, jf->filename);

			/* check if dest file name is occupied,
			 * since checksums are diff, the files are diff */
			for (i = 0; i < 10; i++) {
				if (i > 0) {
					memset(tmp1, 0, sizeof(PATH_MAX));
					sprintf(tmp1, "%s/%s-%d.jpg", argv[2], jf->datetime, i);
				} else {
					strcpy(tmp1, argv[2]);
					strcat(tmp1, "/");
					strcat(tmp1, jf->datetime);
					strcat(tmp1, ".jpg");
				}
				if ((fp = fopen(tmp1, "r")) != NULL) {
					fclose(fp);
					continue;
				} else {
					break;
				}

			}
			printf("copy %s to %s.jpg\n", jf->filename, jf->datetime);
			//copy_file(tmp, tmp1);
		}
	}
#endif

	/* clean up */
	list_for_each_safe(entry, next, &src) {
		jf = list_entry(entry, struct jpg_file, list);
		if (!list_empty(&src)) {
			//printf("[src] %s\t%s\n", jf->filename, jf->datetime);
			list_del(&jf->list);
			free(jf);
		}
	}


	list_for_each_safe(entry, next, &dest) {
		jf = list_entry(entry, struct jpg_file, list);
		if (!list_empty(&dest)) {
			//printf("[dest] %s\n", jf->filename);
			list_del(&jf->list);
			free(jf);
		}
	}
	system("date");
}

