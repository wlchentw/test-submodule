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
#include "mdp_srv.h"
#include "mdp_drv.h"
#include "mdp_ipi.h"

static struct mdp_service s[MDP_PATH_CNT];

static void mdp_service_init(const struct app_descriptor *app);
static void mdp_service_entry(const struct app_descriptor *app, void *args);

APP_START(mdp_service)
	.name = "mdps",
	.init = mdp_service_init,
	.entry = mdp_service_entry,
	.priv = (void *)&s[MDP_PATH_0],
APP_END

APP_START(mdp1_service)
	.name = "mdp1s",
	.init = mdp_service_init,
	.entry = mdp_service_entry,
	.priv = (void *)&s[MDP_PATH_1],
APP_END

APP_START(mdp2_service)
	.name = "mdp2s",
	.init = mdp_service_init,
	.entry = mdp_service_entry,
	.priv = (void *)&s[MDP_PATH_2],
APP_END

APP_START(mdp3_service)
	.name = "mdp3s",
	.init = mdp_service_init,
	.entry = mdp_service_entry,
	.priv = (void *)&s[MDP_PATH_3],
APP_END


static void mdp_ipi_handler(int id, void *data, unsigned int len)
{
	struct mdp_service *ms;

	if (id == IPI_MDP) {
		ms = &s[MDP_PATH_0];
	}
	else if (id == IPI_MDP_1) {
		ms = &s[MDP_PATH_1];
	}
	else if (id == IPI_MDP_2) {
		ms = &s[MDP_PATH_2];
	}
	else if (id == IPI_MDP_3) {
		ms = &s[MDP_PATH_3];
	}
	else {
		MDP_ERR("unknown ipi id: %d", id);
		return;
	}

	enter_critical_section();
	memcpy(ms->msg, data, MIN(len, sizeof(ms->msg)));
	exit_critical_section();

	MDP_Printf("mdp-vpu: get id=%d, msg=0x%x, len=%d", id, *(int*)data, len);
	event_signal(&ms->event, false);
}

static void mdp_service_init(const struct app_descriptor *app)
{
	enum ipi_id id;
	const char *tn;
	unsigned int tid;
	struct mdp_service *ms;

	ms = (struct mdp_service *)app->priv;

	/* register IPI handler for MDP*/
	if (ms == &s[MDP_PATH_0]) {
		id = IPI_MDP;
		tn = "mdp";
		tid = THREAD_MDP;

		g_path[MDP_PATH_0].id = MDP_PATH_0;
		g_path[MDP_PATH_0].msg = ms->msg;
		ms->path = (void *)&g_path[MDP_PATH_0];
	}
	else if (ms == &s[MDP_PATH_1]) {
		id = IPI_MDP_1;
		tn = "mdp1";
		tid = THREAD_MDP_1;

		g_path[MDP_PATH_1].id = MDP_PATH_1;
		g_path[MDP_PATH_1].msg = ms->msg;
		ms->path = (void *)&g_path[MDP_PATH_1];
	}
	else if (ms == &s[MDP_PATH_2]) {
		id = IPI_MDP_2;
		tn = "mdp2";
		tid = THREAD_MDP_2;

		g_path[MDP_PATH_2].id = MDP_PATH_2;
		g_path[MDP_PATH_2].msg = ms->msg;
		ms->path = (void *)&g_path[MDP_PATH_2];
	}
	else if (ms == &s[MDP_PATH_3]) {
		id = IPI_MDP_3;
		tn = "mdp3";
		tid = THREAD_MDP_3;

		g_path[MDP_PATH_3].id = MDP_PATH_3;
		g_path[MDP_PATH_3].msg = ms->msg;
		ms->path = (void *)&g_path[MDP_PATH_3];
	}
	else {
		MDP_ERR("unknown mdp_service");
		ASSERT(0);
	}

	vpu_ipi_registration(id, mdp_ipi_handler, tn);

	ms->handler = mdp_drv_msg_handler;
	mdp_instance_service_init((struct mdp_path_param *)ms->path);

	event_init(&ms->event, false, EVENT_FLAG_AUTOUNSIGNAL);
	thread_resume(thread_create(tid, app->name,
				    &app_thread_entry, (void *)app, HIGH_PRIORITY));
}

static void mdp_service_entry(const struct app_descriptor *app, void *args)
{
	struct mdp_service *ms;
	ms = (struct mdp_service *)app->priv;

	while(1)
	{
		event_wait(&ms->event);

		MDP_Printf("[%s] event_wait", app->name);

		if (NULL != ms->handler)
			ms->handler(ms->path);
	}
}

