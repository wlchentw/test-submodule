# Copyright (c) 2015-2016 MICROTRUST Incorporated
# All rights reserved
#
# This file and software is confidential and proprietary to MICROTRUST Inc.
# Unauthorized copying of this file and software is strictly prohibited.
# You MUST NOT disclose this file and software unless you get a license
# agreement from MICROTRUST Incorporated.

MICROTRUST_TEE_VERSION ?= 280

$(info "teeid.mk:MICROTRUST_TEE_VERSION=$(MICROTRUST_TEE_VERSION)")

include services/spd/teeid/$(MICROTRUST_TEE_VERSION)/${SPD}.mk
