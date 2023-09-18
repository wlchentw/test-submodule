/*
 * Copyright (c) 2015 MediaTek Inc.
 * Author: PC Chen <pc.chen@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __MDP_DRV_H_
#define __MDP_DRV_H_

extern "C"
{
#include <kernel/event.h>
}


/* HW instance */
enum mdp_path_id {
	MDP_PATH_0 = 0,
	MDP_PATH_1,
	MDP_PATH_2,
	MDP_PATH_3,
	MDP_PATH_CNT
};

struct mdp_path_param {
	enum mdp_path_id id;
	event_t event;
	void *msg;
};

void mdp_instance_service_init(struct mdp_path_param *path);
void mdp_drv_msg_handler(void *pmsg);


void mdp_process_init(enum mdp_path_id id);
void mdp_process_signal(enum mdp_path_id id);
void mdp_process(enum mdp_path_id id);

extern struct mdp_path_param g_path[MDP_PATH_CNT];

#endif
