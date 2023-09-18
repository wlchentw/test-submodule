#ifndef _SPM_MTCMOS_
#define _SPM_MTCMOS_

#define STA_POWER_DOWN  0
#define STA_POWER_ON    1

/*
 * 1. for non-CPU MTCMOS: DISP, CM4, AUDAFE, AUDSRC
 * 2. call spm_mtcmos_noncpu_lock/unlock() before/after any operations
 */
extern int spm_mtcmos_ctrl_disp(int state);
extern int spm_mtcmos_ctrl_cm4(int state);
extern int spm_mtcmos_ctrl_audafe(int state);
extern int spm_mtcmos_ctrl_audsrc(int state);

#endif
