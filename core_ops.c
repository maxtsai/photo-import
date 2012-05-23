#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#include "core_ops.h"
#include "os_api.h"

struct list_head format_head;
_Bool core_is_ready = false;

_Bool register_format(struct format *format)
{
	assert(format);
	if (format->name == NULL || format->fops == NULL)
		return false;
	list_add_tail(&format->head, &format_head);
	return true;
}

void prepare_core()
{
	INIT_LIST_HEAD(&format_head);
	core_is_ready = true;
}

_Bool scan_dir(char *path, struct list_head *list)
{
	assert(path);
	assert(list);

	if (core_is_ready == false || list_empty(&format_head)) {
		printf("Not any format supported!\n");
		return false;
	}
	return get_dir_contents(path, list);
}


char *show_cfname(struct list_head *head)
{
	struct file_info *fi;
	fi = list_entry(head, struct file_info);
	return fi->copied_fname;
}

static int compare(struct list_head *left, struct list_head *right)
{
	struct file_info *lfi, *rfi;
	char *l, *r;
	assert(left && right);
	lfi = list_entry(left, struct file_info);
	l = lfi->copied_fname;
	rfi = list_entry(right, struct file_info);
	r = rfi->copied_fname;

	for (int i = 0; i < MAX_NAME; i++) {
//printf("### [%s:%d] i, l[%d], r[%d] = %d, %c, %c\n", __FUNCTION__, __LINE__, i, i, i, l[i], r[i]);
		if (l[i] > r[i])
			return 1;
		else if (l[i] < r[i])
			return -1;
	}
	return 0;
}

void swap(struct list_head *left, struct list_head *right)
{
	assert(left && right);
	struct list_head *lp, *ln, *rp, *rn;
	lp = left->prev;
	ln = left->next;
	rp = right->prev;
	rn = right->next;
	
	if (lp != left)
		lp->next = right;
	if (ln != left)
		ln->prev = right;
	if (rp != right)
		rp->next = left;
	if (rn != right)
		rn->prev = left;

	if (lp != left)
		right->prev = lp;
	if (ln != left) {
		if (ln == right)
			right->next = left;
		else
			right->next = ln;
	}
	if (rp != right) {
		if (rp == left)
			left->prev = right;
		else
			left->prev = rp;
	}
	if (rn != right)
		left->next = rn;
}

static struct list_head *partition(struct list_head *left, struct list_head *right)
{
	struct list_head *up, *down, *pivot, *tmp;
	static int limit = 0;
	int i;
	assert(left && right);
	if ((limit++ > 100) || (left == right) || (left->next == right)) {
		return NULL;  
	}
	pivot = up = left;
	down = right;

	for(;;) {
		i = 0;
		while(up != right) {
			if (compare(pivot, up) == -1)
				break;
			up = up->next;
			i ++;
			if (up == down) {
				goto done;
			}
		}
		while(down != left) {
			if (compare(pivot, down) >= 0)
				break;
			down = down->prev;
			if (up == down)
				goto done;
		}
		swap(up, down);
		tmp = up;
		up = down;
		down = tmp;
	}
done:
	printf("swap '%s' '%s'\n", show_cfname(pivot), show_cfname(down));
	swap(pivot, down);

	printf("\n\n");
	for (tmp = left; tmp->next != right; tmp = tmp->next)
		printf("'%s'\t", show_cfname(tmp));
	printf("\n\n");
	return pivot;
}

void my_qsort_by_cfname(struct list_head *left, struct list_head *right)
{
	struct list_head *pivot = NULL;
	assert(left && right);
	pivot = partition(left, right);
	if (pivot == NULL)
		return;

	printf("left, right, pivot = %s, %s, %s\n",
			show_cfname(left),
			show_cfname(right),
			show_cfname(pivot));

	my_qsort_by_cfname(left, pivot->prev);
	my_qsort_by_cfname(pivot, right);
}

_Bool check_format(char *root, struct list_head *file_head)
{
	struct format *entry, *next;
	char fname[MAX_PATH];

	assert(root);
	assert(file_head);

	list_for_each_entry_safe(entry, next, struct format, &format_head, head) {
		struct file_info *fentry, *fnext;
		list_for_each_entry_safe(fentry, fnext, struct file_info, file_head, head) {
			if (entry->fops->check) {
				strncpy(fname, root, MAX_PATH);
				strcat(fname, "/");
				strncat(fname, fentry->name, MAX_NAME);
				//printf("%s: check '%s'\n", __FUNCTION__, fname);
				if (entry->fops->check(entry, fname)) {
					strcpy(fentry->format, entry->name);
					if (!entry->fops->get_copied_fname(entry, fname, fentry->copied_fname)) {
						get_file_time(fname, fentry->copied_fname);
						if (memcpy(fentry->format, "JPEG", 4) == 0)
							strcat(fentry->copied_fname, ".jpg");
					}
				}
			}
		}
	}
	return true;
}

_Bool save(char *path, struct list_head *file_head)
{
	struct file_info *fentry, *fnext;
	char fname[MAX_PATH];
	FILE *fp;

	assert(path);
	assert(file_head);

	strncpy(fname, path, MAX_PATH);
	strcat(fname, "/");
	strcat(fname, RECORD_FILE);

	fp = fopen(fname, "w");
	if (!fp) {
		printf("unable to open '%s' (%s)", fname, strerror(errno));
		return false;
	}

	list_for_each_entry_safe(fentry, fnext, struct file_info, file_head, head) {
		if (fentry->format[0] != '\0') {
			if (fentry->copied_fname[0] == '\0')
				fprintf(fp, "%s %s %s\n", fentry->format, fentry->name, fentry->name);
			else
				fprintf(fp, "%s %s %s\n", fentry->format, fentry->copied_fname, fentry->name);
		}
	}

	fclose(fp);
	return true;
}

_Bool load(char *path, struct list_head *file_head)
{
	struct file_info *fentry, *fnext;
	char fname[MAX_PATH];
	FILE *fp;

	assert(path);
	assert(file_head);

	strncpy(fname, path, MAX_PATH);
	strcat(fname, "/");
	strcat(fname, RECORD_FILE);

	list_for_each_entry_safe(fentry, fnext, struct file_info, file_head, head) {
		list_del(&fentry->head);
		free(fentry);
	}

	fp = fopen(fname, "r");
	if (!fp) {
		printf("unable to open '%s' (%s)", fname, strerror(errno));
		return false;
	}

	while(!feof(fp)) {
		struct file_info *fi = malloc(sizeof(struct file_info));
		errno = 0;
		fi->copied_fname[0] = '\0';
		fi->name[0] = '\0';
		fi->duplicated = false;
		if ((fscanf(fp, "%s %s %s\n", fi->format, fi->copied_fname, fi->name) == 0) || (errno))
			printf("load error (%s)\n", strerror(errno));
		list_add_tail(&fi->head, file_head);
	}

	fclose(fp);
	return true;
}

