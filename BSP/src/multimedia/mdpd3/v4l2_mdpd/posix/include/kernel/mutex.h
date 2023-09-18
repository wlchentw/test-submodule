#ifndef __KERNEL_MUTEX_H
#define __KERNEL_MUTEX_H

#include <kernel/thread.h>
#include <pthread.h>

typedef struct _mutex_t {
	pthread_mutex_t mux;
} mutex_t;

/* Rules for Mutexes:
 * - Mutexes are only safe to use from thread context.
 * - Mutexes are non-recursive.
*/

void mutex_init(mutex_t *);
void mutex_destroy(mutex_t *);
status_t mutex_acquire(mutex_t *);
status_t mutex_acquire_timeout(mutex_t *, time_t); /* try to acquire the mutex with a timeout value */
status_t mutex_release(mutex_t *);

#endif

