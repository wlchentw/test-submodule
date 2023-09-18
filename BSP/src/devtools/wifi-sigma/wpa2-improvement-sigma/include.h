#if __CYGWIN__ // MTK_P2P_SIGMA, MTK_HS20_SIGMA
#include <cygwin/types.h>
#include <cygwin/socket.h>
#include <cygwin/if.h>
#include <sys/select.h>
#include <poll.h>
#else
#include <linux/types.h>
#include <linux/socket.h>
#include <sys/select.h>
#include <poll.h>
#include <linux/if.h>
#endif /* __CYGWIN__ MTK_P2P_SIGMA, MTK_HS20_SIGMA */
