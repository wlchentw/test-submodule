#pragma once

#include <pthread.h>
#include <stdint.h>
#include <sys/types.h>
#ifdef __ANDROID__
#include <sys/vpu_types.h>
#endif

typedef struct event {
	/* To guarantee the event variable aligns with sizeof(void *),
	   especially for 64 bit OS, it must be aligned with 8 bytes.
	*/
	void *__padding;
	char __size[104 - sizeof(void *)];
} event_t;

#define EVENT_FLAG_AUTOUNSIGNAL 1

void event_init(event_t *, bool initial, uint flags);
void event_destroy(event_t *);
status_t event_wait(event_t *);
status_t event_wait_timeout(event_t *, time_t); /* wait on the event with a timeout */
status_t event_signal(event_t *, bool reschedule);
status_t event_unsignal(event_t *);
