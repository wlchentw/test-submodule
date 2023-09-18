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

extern "C"
{
#include <app.h>
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <kernel/thread.h>
#include <kernel/ipi.h>
#include <kernel/event.h>
}

#include "mdp_debug.h"
#include "mdp_drv.h"
#include "mdp_ipi.h"


static void mdp_process_service_init(const struct app_descriptor *app);
static void mdp_process_service_entry(const struct app_descriptor *app, void *args);


APP_START(mdp_process_service)
	.name = "mdpps",
	.init = mdp_process_service_init,
	.entry = mdp_process_service_entry,
	.priv = (void *)MDP_PATH_0,
APP_END

APP_START(mdp1_process_service)
	.name = "mdp1ps",
	.init = mdp_process_service_init,
	.entry = mdp_process_service_entry,
	.priv = (void *)MDP_PATH_1,
APP_END

APP_START(mdp2_process_service)
	.name = "mdp2ps",
	.init = mdp_process_service_init,
	.entry = mdp_process_service_entry,
	.priv = (void *)MDP_PATH_2,
APP_END

APP_START(mdp3_process_service)
	.name = "mdp3ps",
	.init = mdp_process_service_init,
	.entry = mdp_process_service_entry,
	.priv = (void *)MDP_PATH_3,
APP_END

static void mdp_process_service_init(const struct app_descriptor *app)
{
	enum mdp_path_id id = (enum mdp_path_id)(unsigned long)app->priv;
	unsigned int tid[] = {
		THREAD_MDP_PROCESS,
		THREAD_MDP_1_PROCESS,
		THREAD_MDP_2_PROCESS,
		THREAD_MDP_3_PROCESS
	};

	mdp_process_init(id);

	thread_resume(thread_create(tid[id], app->name,
				    &app_thread_entry, (void *)app, HIGH_PRIORITY));
}

static void mdp_process_service_entry(const struct app_descriptor *app, void *args)
{
	enum mdp_path_id id = (enum mdp_path_id)(unsigned long)app->priv;

	while(1)
	{
		mdp_process(id);
	}
}

