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

#define MAX_THREADS 100
#define MAX_RUNS 1000000

struct _pthread_data_struct {
	pthread_t thread;
	int used;
	int count;
	pthread_mutex_t mutex;
} pthread_data_struct[MAX_THREADS];

void *theThread(void *parm)
{
	struct _pthread_data_struct *pds;
	static int i = 0;
	if (parm == NULL)
		return NULL;
	pds = (struct _pthread_data_struct *) parm;

	srand(time(NULL));

	printf("%s:%d, i = %d\n", __FUNCTION__, __LINE__, i++);
	usleep(rand() % 10000);
	//pthread_exit(NULL);
	//pthread_mutex_lock(&pds->mutex);
	pds->used = 0;

	pthread_detach (pthread_self());
	//pthread_mutex_unlock(&pds->mutex);

}

struct _pthread_data_struct *pick_thread()
{
	int i;

	for (i = 0; i < MAX_THREADS; i++) {
		//pthread_mutex_lock(&pthread_data_struct[i].mutex);
		if (pthread_data_struct[i].used == 0) {
			pthread_data_struct[i].count++;
			//pthread_mutex_unlock(&pthread_data_struct[i].mutex);
			return &pthread_data_struct[i];
		}
		//pthread_mutex_unlock(&pthread_data_struct[i].mutex);
	}
	return NULL;
}


void main(int argc, char **argv)
{
	struct _pthread_data_struct *pds;
	pthread_attr_t attr;
	int i, rc;

	printf("%s:%d enter\n", __FUNCTION__, __LINE__);

	memset(pthread_data_struct, 0, sizeof(pthread_data_struct));

	for (i = 0; i < MAX_THREADS; i++) {
		pthread_data_struct[i].used = 0;
		pthread_data_struct[i].count = 0;
		pthread_mutex_init(&pthread_data_struct[i].mutex, NULL);
	}

	for (;;) {
		pds = pick_thread();
		if (pds == NULL) {
			//pthread_join(pthread_data_struct[0].thread, NULL);
			usleep(1000);
		} else {
			if (i++ > MAX_RUNS)
				break;
			//pthread_mutex_lock(&pds->mutex);
			pds->used = 1;
			//pthread_mutex_unlock(&pds->mutex);
			/*
			pthread_attr_init(&attr);
			pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
			rc = pthread_create(&pds->thread, &attr, theThread, (void*) pds);
			*/
			rc = pthread_create(&pds->thread, NULL, theThread, (void*) pds);
			//pthread_attr_destroy(&attr);
			if (rc != 0)
				perror("pthread_create");
			continue;
		}
	}
	/*
	for (i = 0; i < MAX_THREADS; i++) {
		pthread_join(pthread_data_struct[i].thread, NULL);
		printf("[thread%02d] count = %03d\n", i, pthread_data_struct[i].count);
	}
	*/

	for (i = 0; i < MAX_THREADS; i++) {
		pthread_mutex_destroy(&pthread_data_struct[i].mutex);
	}

	printf("%s:%d exit\n", __FUNCTION__, __LINE__);
}
