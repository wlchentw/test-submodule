#include <sys/prctl.h>
#include <assert.h>
#include <pthread.h>
#include <string.h>

#include "kernel/thread.h"
#include "err.h"


/*
thread_create
thread_resume
*/

struct thread {
	pthread_t thread;
	pthread_mutex_t mutex;

	/* entry point */
	thread_start_routine entry;
	void *arg;

	char name[16];
};

static thread_t threads[NR_THREAD];

static void thread_suspend(thread_t *thread)
{
	while (pthread_mutex_lock(&thread->mutex) != 0);
}

status_t thread_resume(thread_t *thread)
{
	while (pthread_mutex_unlock(&thread->mutex) != 0);
	return NO_ERROR;
}

static void *thread_entry(void *arg)
{
	thread_t *thread = (thread_t *)(arg);

	prctl(PR_SET_NAME, thread->name);

	thread_suspend(thread);
	thread->entry(thread->arg);

	thread->thread = (pthread_t)(0);
	return NULL;
}

thread_t *thread_create(const unsigned int id, const char *name, thread_start_routine entry, void *arg, unsigned int priority)
{
	thread_t *thread = &threads[id];
	int i;

	pthread_mutex_init(&thread->mutex, NULL);
	pthread_mutex_lock(&thread->mutex);

	strncpy(thread->name, name, sizeof(thread->name));
	thread->entry = entry;
	thread->arg = arg;

	i = pthread_create(&thread->thread, NULL, thread_entry, thread);
	assert(i == 0);

	return thread;
}

void thread_yield(void)
{
	sched_yield();
}

/*
enter_critical_section
exit_critical_section
*/

static struct {
	pthread_mutex_t mutex;
	pthread_t thread;
	int selfcount;
} crit = {
	.mutex = PTHREAD_MUTEX_INITIALIZER
};

void enter_critical_section(void)
{
	pthread_t thread = pthread_self();
	if (!pthread_equal(crit.thread, thread)) {
		while (pthread_mutex_lock(&crit.mutex) != 0);
		assert(crit.selfcount == 0);
		crit.thread = thread;
	}
	crit.selfcount++;
}

void exit_critical_section(void)
{
	pthread_t thread = pthread_self();
	assert(pthread_equal(crit.thread, thread));
	crit.selfcount--;
	if (crit.selfcount == 0) {
		crit.thread = (pthread_t)(0);
		while (pthread_mutex_unlock(&crit.mutex) != 0);
	}
}
