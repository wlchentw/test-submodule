#ifndef __PLATFORM_UTIL_H
#define __PLATFORM_UTIL_H

void send_thread_info(void);

/**
 * If VPU complete the initialization,
 * it will send ipi message to notify APMCU.
 */
void send_init_fin(void);
void send_uninit_fin(void);
void vpu_fw_version(void);
#endif /* __PLATFORM_UTIL_H */

