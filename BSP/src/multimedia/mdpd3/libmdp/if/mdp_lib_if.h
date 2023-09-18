#ifndef MDP_IF_H__
#define MDP_IF_H__

#define MDP_LIB
#include "DpDataType.h"
#include "mdp_ipi.h"
#include "mdp_debug.h"

#define CONFIG_FOR_TUNING 1

#define MTK_MDP_MAX_CTX         30
#define MDP_CMDQ_SIZE           (32*1024)
#if CONFIG_FOR_TUNING
#define MDP_TUNING_SIZE         (200*1024)
#endif

typedef int (*MDP_NOTIFY_EXECUTE_COMMAND)(void *priv, void *cmdq_param);

struct MDP_CMDQ_FLUSH_CB_PRIV_DATA {
        unsigned int cmdq_buf_pa;
        void *priv_data;
};

struct mdp_param {
	struct mdp_process_vsi  vsi;
	uint8_t                 cmdq_buffer[MDP_CMDQ_SIZE];
	unsigned int            h_drv;
	#if CONFIG_FOR_TUNING
	struct mdp_tuning       *shmem_mdp_tuning;
	#endif
};

#if CONFIG_FOR_TUNING
struct mdp_tuning {
	uint8_t                 tuning_buf[MDP_TUNING_SIZE];
};
#endif

struct mtk_mdp_lib_if {
	DP_STATUS_ENUM (*init)(struct mdp_param *param);
	DP_STATUS_ENUM (*deinit)(struct mdp_param *param);
	DP_STATUS_ENUM (*process)(struct mdp_param *param);
	DP_STATUS_ENUM (*process_thread)(struct mdp_param *param, MDP_NOTIFY_EXECUTE_COMMAND pfn, void *priv);
	DP_STATUS_ENUM (*process_continue)(struct mdp_param *param);
	DP_STATUS_ENUM (*prepare_cmdq_param)(struct mdp_param *param, void *cmdq_param);
	DP_STATUS_ENUM (*wait_cmdq_done)(struct mdp_param *param, void *cmdq_param);
	DP_STATUS_ENUM (*cmdq_flushing_status)(struct mdp_param *param);
	void (*dbg_para)(void);
	void (*dbg_proc)(const char *buf, size_t count);
};

extern struct mtk_mdp_lib_if mtk_mdp_if;

#endif
