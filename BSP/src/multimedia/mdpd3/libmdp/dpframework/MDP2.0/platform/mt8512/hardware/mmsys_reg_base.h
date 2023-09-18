#ifndef __MMSYS_REG_BASE_H__
#define __MMSYS_REG_BASE_H__

#define IMGSYS_CONFIG_BASE   (0x15000000)
#define MDP_RDMA0_BASE      (0x15007000)
#define MDP_RSZ0_BASE       (0x15008000)
#define MDP_WROT0_BASE      (0x1500a000)

#define MDP_TDSHP0_BASE     (0x15009000)
#define MDP_GAMMA_BASE      (0x1500b000)
#define MDP_DTH_BASE        (0x1500c000)

#define MMSYS_MUTEX_BASE    (0x15001000)//DpDriver::getInstance()->getMMSysMutexBase()

#define SMI_LARB0_BASE      (0x14017000)
#define SMI_LARB1_BASE      (0x14018000)
#define SMI_COMMON_BASE     (0x14019000)

#define MMSYS_CMDQ_BASE     (0x10280000)

#define MM_REG_READ(cmd, addr, val, ...)                (cmd).read((addr), (val), ##__VA_ARGS__)
#define MM_REG_READ_BEGIN(cmd)                          MM_REG_WAIT(cmd, DpCommand::SYNC_TOKEN_GPR_READ)        /* Must wait token before read */
#define MM_REG_READ_END(cmd)                            MM_REG_SET_EVENT(cmd, DpCommand::SYNC_TOKEN_GPR_READ)   /* Must set token after read */

#define MM_REG_WRITE_MASK(cmd, addr, val, mask, ...)    (cmd).write((addr), (val), (mask), ##__VA_ARGS__)
#define MM_REG_WRITE(cmd, addr, val, mask, ...)         MM_REG_WRITE_MASK(cmd, addr, val, (((mask) & (addr##_MASK)) == (addr##_MASK)) ? (0xFFFFFFFF) : (mask), ##__VA_ARGS__)

#define MM_REG_WAIT(cmd, event)                         (cmd).wait(event)

#define MM_REG_WAIT_NO_CLEAR(cmd, event)                (cmd).waitNoClear(event)

#define MM_REG_CLEAR(cmd, event)                        (cmd).clear(event)

#define MM_REG_SET_EVENT(cmd, event)                    (cmd).setEvent(event)

#define MM_REG_POLL_MASK(cmd, addr, val, mask, ...)     (cmd).poll((addr), (val), (mask), ##__VA_ARGS__)
#define MM_REG_POLL(cmd, addr, val, mask, ...)          MM_REG_POLL_MASK(cmd, addr, val, (((mask) & (addr##_MASK)) == (addr##_MASK)) ? (0xFFFFFFFF) : (mask), ##__VA_ARGS__)

#define MM_REG_WRITE_FROM_MEM(cmd, addr, val, mask)     (cmd).writeFromMem((addr), (val), (mask))
#define MM_REG_WRITE_FROM_MEM_BEGIN(cmd)                MM_REG_WAIT(cmd, DpCommand::SYNC_TOKEN_GPR_WRITE_FROM_MEM)      /* Must wait token before write from mem */
#define MM_REG_WRTIE_FROM_MEM_END(cmd)                  MM_REG_SET_EVENT(cmd, DpCommand::SYNC_TOKEN_GPR_WRITE_FROM_MEM) /* Must set token after write from mem */

#define MM_REG_WRITE_FROM_REG(cmd, addr, val, mask)     (cmd).writeFromReg((addr), (val), (mask))
#define MM_REG_WRITE_FROM_REG_BEGIN(cmd)                MM_REG_WAIT(cmd, DpCommand::SYNC_TOKEN_GPR_WRITE_FROM_REG)      /* Must wait token before write from reg */
#define MM_REG_WRITE_FROM_REG_END(cmd)                  MM_REG_SET_EVENT(cmd, DpCommand::SYNC_TOKEN_GPR_WRITE_FROM_REG) /* Must set token after write from reg */

#define ID_ADDR(id)     (id << 12)

#endif  // __MM_REG_BASE_H__
