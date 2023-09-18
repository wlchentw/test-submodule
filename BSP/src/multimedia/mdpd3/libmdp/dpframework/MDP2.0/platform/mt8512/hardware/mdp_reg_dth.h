#ifndef __MDP_REG_DTH_H__
#define __MDP_REG_DTH_H__

#include "mmsys_reg_base.h"

#define MDP_DTH_EN                (ID_ADDR(m_identifier)+ MDP_DTH_BASE + 0x000)
#define MDP_DTH_RESET             (ID_ADDR(m_identifier)+ MDP_DTH_BASE + 0x004)
#define MDP_DTH_INTEN             (ID_ADDR(m_identifier)+ MDP_DTH_BASE + 0x008)
#define MDP_DTH_INTSTA            (ID_ADDR(m_identifier)+ MDP_DTH_BASE + 0x00C)
#define MDP_DTH_STATUS            (ID_ADDR(m_identifier)+ MDP_DTH_BASE + 0x010)
#define MDP_DTH_CFG               (ID_ADDR(m_identifier)+ MDP_DTH_BASE + 0x020)
#define MDP_DTH_INPUT_COUNT       (ID_ADDR(m_identifier)+ MDP_DTH_BASE + 0x024)
#define MDP_DTH_OUTPUT_COUNT      (ID_ADDR(m_identifier)+ MDP_DTH_BASE + 0x028)
#define MDP_DTH_CHKSUM            (ID_ADDR(m_identifier)+ MDP_DTH_BASE + 0x02C)
#define MDP_DTH_SIZE              (ID_ADDR(m_identifier)+ MDP_DTH_BASE + 0x030)
#define MDP_DTH_SHADOW            (ID_ADDR(m_identifier)+ MDP_DTH_BASE + 0x0B0)
#define MDP_DTH_DUMMY_REG         (ID_ADDR(m_identifier)+ MDP_DTH_BASE + 0x0C0)
#define MDP_DTH_ATPG              (ID_ADDR(m_identifier)+ MDP_DTH_BASE + 0x0FC)

#define MDP_DTH_EN_MASK           (0x01)
#define MDP_DTH_RESET_MASK        (0x01)
#define MDP_DTH_INTEN_MASK        (0x03)
#define MDP_DTH_INTSTA_MASK       (0x03)     
#define MDP_DTH_STATUS_MASK       (0xFFFFFFF3)
#define MDP_DTH_CFG_MASK          (0x70001FFF)
#define MDP_DTH_INPUT_COUNT_MASK  (0x1FFF1FFF)
#define MDP_DTH_OUTPUT_COUNT_MASK (0x1FFF1FFF)
#define MDP_DTH_CHKSUM_MASK       (0xFFFFFFFF)
#define MDP_DTH_SIZE_MASK         (0x1FFF1FFF)
#define MDP_DTH_SHADOW_MASK       (0x07)
#define MDP_DTH_DUMMY_REG_MASK    (0xFFFFFFFF)
#define MDP_DTH_ATPG_MASK         (0x03)

#endif  // __MDP_REG_DTH_H__
