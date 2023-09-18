#include <err.h>
#include <kernel/mutex.h>
#include <kernel/thread.h>
#include <pthread.h>
#include <time.h>

/**
 * @brief  Initialize a mutex_t
 */
void mutex_init(mutex_t *m)
{
	pthread_mutex_init(&m->mux, NULL);
}

/**
 * @brief  Destroy a mutex_t
 *
 * This function frees any resources that were allocated
 * in mutex_init().  The mutex_t object itself is not freed.
 */
void mutex_destroy(mutex_t *m)
{
	pthread_mutex_destroy(&m->mux);
}

/**
 * @brief  Acquire a mutex; wait if needed.
 *
 * This function waits for a mutex to become available.  It
 * may wait forever if the mutex never becomes free.
 *
 * @return  NO_ERROR on success, other values on error
 */
status_t mutex_acquire(mutex_t *m)
{
	return pthread_mutex_lock(&m->mux);
}

/**
 * @brief  Mutex wait with timeout
 *
 * This function waits up to \a timeout ms for the mutex to become available.
 * Timeout may be zero, in which case this function returns immediately if
 * the mutex is not free.
 *
 * @return  NO_ERROR on success, ERR_TIMED_OUT on timeout,
 * other values on error
 */
status_t mutex_acquire_timeout(mutex_t *m, time_t timeout)
{
	struct timespec t;
	long s;

	if (timeout == INFINITE_TIME)
		return pthread_mutex_lock(&m->mux);

	clock_gettime(CLOCK_REALTIME, &t);
	s = timeout / 1000; // timeout is in ms
	t.tv_sec += s;
	t.tv_nsec += ((timeout - s*1000) * 1000000);
	if (t.tv_nsec >= 1000000000) {
		t.tv_sec++;
		t.tv_nsec -= 1000000000;
	}
	return pthread_mutex_timedlock(&m->mux, &t);
}

/**
 * @brief  Release mutex
 */
status_t mutex_release(mutex_t *m)
{
	return pthread_mutex_unlock(&m->mux);
}

