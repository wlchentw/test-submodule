#include <pthread.h>

#include "kernel/event.h"
#include "err.h"


#define COMPILE_ASSERT(x) extern int __dummy[(int)(x) ? 1 : -1]

typedef struct _event {
	pthread_cond_t cond;
	pthread_mutex_t mutex;
	bool signalled;
	uint flags;
} _event_t;

COMPILE_ASSERT(sizeof(struct event) >= sizeof(struct _event));
// TODO:  Need check

void event_init(event_t *e_, bool initial, uint flags)
{
	_event_t *e = (_event_t *)(e_);
	pthread_cond_init(&e->cond, NULL);
	pthread_mutex_init(&e->mutex, NULL);
	e->signalled = initial;
	e->flags = flags;
}

void event_destroy(event_t * e_)
{
	_event_t *e = (_event_t *)(e_);

	pthread_mutex_destroy(&e->mutex);
	pthread_cond_destroy(&e->cond);
}

status_t event_wait(event_t *e_)
{
	_event_t *e = (_event_t *)(e_);
	while (pthread_mutex_lock(&e->mutex) != 0);
	if (!e->signalled)
		while (pthread_cond_wait(&e->cond, &e->mutex) != 0);
	if (e->flags & EVENT_FLAG_AUTOUNSIGNAL)
		e->signalled = false;
	while (pthread_mutex_unlock(&e->mutex) != 0);
	return NO_ERROR;
}

status_t event_signal(event_t *e_, bool reschedule)
{
	_event_t *e = (_event_t *)(e_);
	while (pthread_mutex_lock(&e->mutex) != 0);
	if (!e->signalled) {
		e->signalled = true;
		if (e->flags & EVENT_FLAG_AUTOUNSIGNAL)
			while (pthread_cond_signal(&e->cond) != 0);
		else
			while (pthread_cond_broadcast(&e->cond) != 0);
	}
	while (pthread_mutex_unlock(&e->mutex) != 0);
	return NO_ERROR;
}
