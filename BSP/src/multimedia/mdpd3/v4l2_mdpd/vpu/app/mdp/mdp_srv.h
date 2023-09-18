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

#ifndef __MDP_SRV_H__
#define __MDP_SRV_H__

#include <kernel/event.h>


typedef void (*mdp_msg_handler)(void *msg);


struct mdp_service {
	event_t			event;
	unsigned long		reserved[1]; /* msg should be aligned with 8-byte for 64-bit OS, for memcpy address */
	unsigned char 		msg[48];
	mdp_msg_handler	    handler;
	void		*path;	/* mdp_path_param for multi-instance */
};
#endif
