#ifndef __MDP_REG_GAMMA_H__
#define __MDP_REG_GAMMA_H__

#include "mmsys_reg_base.h"

#define MDP_GAMMA_EN                (ID_ADDR(m_identifier)+ MDP_GAMMA_BASE + 0x000)
#define MDP_GAMMA_RESET             (ID_ADDR(m_identifier)+ MDP_GAMMA_BASE + 0x004)
#define MDP_GAMMA_INTEN             (ID_ADDR(m_identifier)+ MDP_GAMMA_BASE + 0x008)
#define MDP_GAMMA_INTSTA            (ID_ADDR(m_identifier)+ MDP_GAMMA_BASE + 0x00C)
#define MDP_GAMMA_STATUS            (ID_ADDR(m_identifier)+ MDP_GAMMA_BASE + 0x010)
#define MDP_GAMMA_CFG               (ID_ADDR(m_identifier)+ MDP_GAMMA_BASE + 0x020)
#define MDP_GAMMA_INPUT_COUNT       (ID_ADDR(m_identifier)+ MDP_GAMMA_BASE + 0x024)
#define MDP_GAMMA_OUTPUT_COUNT      (ID_ADDR(m_identifier)+ MDP_GAMMA_BASE + 0x028)
#define MDP_GAMMA_CHKSUM            (ID_ADDR(m_identifier)+ MDP_GAMMA_BASE + 0x02C)
#define MDP_GAMMA_SIZE              (ID_ADDR(m_identifier)+ MDP_GAMMA_BASE + 0x030)
#define MDP_GAMMA_LUT               (ID_ADDR(m_identifier)+ MDP_GAMMA_BASE + 0x700)

#define MDP_GAMMA_EN_MASK           (0x01)
#define MDP_GAMMA_RESET_MASK        (0x01)
#define MDP_GAMMA_INTEN_MASK        (0x03)
#define MDP_GAMMA_INTSTA_MASK       (0x03)     
#define MDP_GAMMA_STATUS_MASK       (0xFFFF3)
#define MDP_GAMMA_CFG_MASK          (0x7000011F)
#define MDP_GAMMA_INPUT_COUNT_MASK  (0x1FFF1FFF)
#define MDP_GAMMA_OUTPUT_COUNT_MASK (0x1FFF1FFF)
#define MDP_GAMMA_CHKSUM_MASK       (0xFFFFFFFF)
#define MDP_GAMMA_SIZE_MASK         (0x1FFF1FFF)
#define MDP_GAMMA_LUT_MASK          (0xFF)

#endif  // __MDP_REG_GAMMA_H__
