#pragma once

#include <sys/types.h>
#ifdef __ANDROID__
#include <sys/vpu_types.h>
#endif

enum thread_id {
	THREAD_ANC = 0,
	THREAD_VDEC,
	THREAD_VENC,
	THREAD_MDP,
	THREAD_MDP_PROCESS,
	THREAD_MDP_1,
	THREAD_MDP_1_PROCESS,
	THREAD_MDP_2,
	THREAD_MDP_2_PROCESS,
	THREAD_MDP_3,
	THREAD_MDP_3_PROCESS,
        //THREAD_DEMO1,
        //THREAD_DEMO2,
        //THREAD_DEMO3,
        //THREAD_DEMO4,
	THREAD_APP,
	/*...*/
	NR_THREAD,
};

typedef int (*thread_start_routine)(void *arg);

typedef struct thread thread_t;

/* thread priority */
#define NUM_PRIORITIES 32
#define LOWEST_PRIORITY 0
#define HIGHEST_PRIORITY (NUM_PRIORITIES - 1)
#define DPC_PRIORITY (NUM_PRIORITIES - 2)
#define IDLE_PRIORITY (LOWEST_PRIORITY + 1)
#define NEVER_RUN_PRIORITY LOWEST_PRIORITY
#define LOW_PRIORITY (NUM_PRIORITIES / 4)
#define DEFAULT_PRIORITY (NUM_PRIORITIES / 2)
#define HIGH_PRIORITY ((NUM_PRIORITIES / 4) * 3)

thread_t *thread_create(const unsigned int id, const char *name, thread_start_routine entry, void *arg, unsigned int priority);
status_t thread_resume(thread_t *);

void thread_yield(void); /* give up the cpu voluntarily */

void enter_critical_section(void);
void exit_critical_section(void);
