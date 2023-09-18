#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
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
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sched.h>
#include <math.h>
#include <errno.h>

