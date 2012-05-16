#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <dirent.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "list.h"

#define NAME_MAX 255
#define PATH_MAX 1024

#include "format.h"
#include "os_api.h"

_Bool get_file_time(char *filename, char *time)
{
	struct stat sb;
	struct tm *ft;

	assert(filename && time);

	if (stat(filename, &sb) == -1) {
		perror("stat");
		return false;
	}
	ft = localtime(&sb.st_mtime);
	memset(time, 0, 64);
	strftime(time, 63, "%Y-%m-%d-%H-%M-%S", ft);
	//printf("%s\n", time);
	return true;
}
_Bool get_dir_contents(char *path, struct list_head *contents_head)
{
	struct dirent *de;
	DIR *dir;

	assert(path);
	assert(contents_head);

	if ((dir = opendir(path)) == NULL) {
		perror("opendir");
		return false;
	}

	errno = 0;
	while ((de = readdir(dir))) {
		struct file_info *ff = malloc(sizeof(struct file_info));
		ff->copied_fname[0] = '\0';
		ff->format[0] = '\0';
		ff->duplicated = false;
		strncpy(ff->name, de->d_name, MAX_NAME-1);
		list_add_tail(&ff->head, contents_head);
	}
	if (errno) {
		struct file_info *entry, *next;
		perror("readdir");
		list_for_each_entry_safe(entry, next, struct file_info, contents_head, head)
			list_del(&entry->head);
		closedir(dir);
		return false;
	}

	closedir(dir);
	return true;
}

