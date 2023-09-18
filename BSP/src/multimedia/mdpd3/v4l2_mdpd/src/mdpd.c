#define NO_FUSE

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#ifndef NO_FUSE
#include <cuse_lowlevel.h>
#include <fuse_opt.h>
#endif
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include "kernel/ipi.h"
#include "platform/mt_reg_base.h"
#include "platform/util.h"
#include "mdpd.h"
#ifdef __ANDROID__
#include <cutils/log.h>
#define LOG_TAG "mdpd"
#define fprintf(level,x...) ALOGE(x)
#endif

extern void apps_init(void);

extern int ipi_id_from_inst_id(int id);
extern void ipi_handler(struct share_obj *obj);
extern void send_obj(int fd, int cmd, int idx);
extern int mdp_support(void);

#ifndef NO_FUSE
static const char *usage =
    "usage: mdpd [options]\n"
    "\n"
    "options:\n"
    "    --help|-h             print this help message\n"
    "    --maj=MAJ|-M MAJ      device major number\n"
    "    --min=MIN|-m MIN      device minor number\n"
    "    --name=NAME|-n NAME   device name (mandatory)\n"
    "\n";
#endif

static struct {
	pthread_cond_t cond;
	pthread_mutex_t mutex;
	struct share_obj obj;
} rcv = {
	.cond = PTHREAD_COND_INITIALIZER,
	.mutex = PTHREAD_MUTEX_INITIALIZER,
};

#ifndef IPI_SYNC_ONLY
static pthread_t thread_s[IPI_INST_CNT];
#endif
static pthread_t thread_r[IPI_INST_CNT];
extern struct ipi_ctx g_ctx_s;
static struct ipi_ctx g_ctx_r;
volatile static int g_fg_exit = 0;

#ifndef IPI_SYNC_ONLY
static void *sender(void *arg)
{
	int idx = (int)(long)arg;

	while (1)
		send_obj(g_ctx_s.fd, g_ctx_s.cmd, idx);

	return 0;
}
#endif

static void *receiver(void *arg)
{
	int i = (int)(long)arg;
	int ret;
	struct share_obj obj;

	i = ipi_id_from_inst_id(i);

	while (1) {
		obj.id = i; /* the kernel will select instance id by judging the ipi id */
		ret = ioctl(g_ctx_r.fd, g_ctx_r.cmd, &obj);
		if (g_fg_exit)
			break;

		if (ret != 0) {
			/* Suspend or reboot case */
			usleep(1000); // 1 ms
			fprintf(stderr, "[mdpd] ipi get return %d\n", ret);
			if (g_fg_exit)
				break;
		}
		else
			ipi_handler(&obj);
	}

	fprintf(stderr, "[mdpd] receiver thread breaked\n");
	return NULL;
}

static void remap_shmem(int fd)
{
	uintptr_t shmem_start, shmem_end;
	size_t shmem_length;
	void *shmem;

	FILE *fp;
	char s[256];
	uintptr_t unused;
	const char *fmt = "%x-%x";

	fp = fopen("/proc/self/maps", "r");
	assert(fp != NULL);
	while (fgets(s, sizeof(s), fp))
		if (strstr(s, "rw-p") && strstr(s, "mdpd.so"))
			break;

	if (sizeof(uintptr_t) == 8)
		fmt = "%llx-%llx";

#ifdef __ANDROID__
	sscanf(s, fmt, &shmem_start, &shmem_end);
#else
	sscanf(s, fmt, &shmem_start, &unused);
	if (!fgets(s, sizeof(s), fp)) {
		fprintf(stderr, "invalid maps file.\n");
		assert(NULL);
	}
	sscanf(s, fmt, &unused, &shmem_end);
#endif

	fclose(fp);

	shmem_length = shmem_end - shmem_start;
	shmem = mmap(NULL, shmem_length, PROT_READ | PROT_WRITE,
	             MAP_SHARED, fd, MAP_SHMEM_ALLOCATE);
	assert(shmem != MAP_FAILED);
	memcpy(shmem, (void *)(shmem_start), shmem_length);
	munmap(shmem, shmem_length);
	shmem = mmap((void *)(shmem_start), shmem_length,
	             PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd,
	             MAP_SHMEM_COMMIT);
	assert(shmem == (void *)(shmem_start));

	fprintf(stderr, "mdpd: shmem_length=%u, shmem=%p\n", (unsigned int)shmem_length, shmem);
}

static int create_rt_thread(pthread_t *pth, void*(*func)(void *), void *arg,
		     int policy, int prio)
{
	int ret = 0, fail = 0;
	struct sched_param schedp;
	pthread_attr_t attr;
	pthread_attr_t *pattr = &attr;

	// The default policy is SCHED_NORMAL (0), priority is 0.
	// sched_getparam(getpid(), &schedp);
	// pthread_getschedparam(pthread_self(), &ret, &schedp);

	pthread_attr_init(&attr);
	/* safe to get existing scheduling param */
	pthread_attr_getschedparam(&attr, &schedp);

#ifndef __ANDROID__
	/*
	 * PTHREAD_EXPLICIT_SCHED: Specifies that the scheduling policy and
	 * associated attributes are to be set to the corresponding values from
	 * this attribute object.
	 */
	ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	if (ret) {
		fprintf(stderr, "[mdp] pthread_attr_setinheritsched %d\n", ret);
		fail = 1;
	}


	ret = pthread_attr_setschedpolicy(&attr, policy);
	if (ret) {
		fprintf(stderr, "[mdp] pthread_attr_setschedpolicy %d\n", ret);
		fail = 1;
	}

	schedp.sched_priority = prio;
	ret = pthread_attr_setschedparam(&attr, &schedp);
	if (ret) {
		fprintf(stderr, "[mdp] pthread_attr_setschedparam %d\n", ret);
		fail = 1;
	}
	#endif

	if (fail == 1) {
		fprintf(stderr, "[mdp] create with default priority.\n");
		pattr = NULL;
	}

	ret = pthread_create(pth, pattr, func, arg);
	if (ret) {
		fprintf(stderr, "[mdp] pthread_create %d\n", ret);
		return -1;
	}

	pthread_attr_destroy(&attr);
	fprintf(stderr, "--[mdp] %s,line:%d\n", __FUNCTION__,__LINE__);
	return 0;
}

#ifdef NO_FUSE
static void mdpd_init(void *userdata, void *conn)
#else
static void mdpd_init(void *userdata, struct fuse_conn_info *conn)
#endif
{
	int i, fd, ret;

	fd = open(PATH_DEV_VCU, O_RDWR);
	if (fd == -1)
		fd = open(PATH_DEV_VPU, O_RDWR);
	assert(fd != -1);

	remap_shmem(fd);

	/* map register buffer range if necessary */

	g_ctx_s.fd = fd;
	g_ctx_s.cmd = VPU_SET_OBJECT;
	vpu_ipi_init();

#ifndef IPI_SYNC_ONLY
	for (i = 0; i < IPI_INST_CNT; i++) {
		pthread_create(&thread_s[i], NULL, sender, (void *)(long)i);
	}
#endif
	apps_init();

	vpu_fw_version();
	send_init_fin();

#ifdef NO_FUSE
	/* Create receiver thread after IPI_VPU_INIT */

	/* The SCHED_FIFO scheduling class is a longstanding, POSIX-specified
	 * realtime feature. Processes in this class are given the CPU for as
	 * long as they want it, subject only to the needs for higher-priority
	 * realtime processes.
	 */
	/*
	 * Priority of a process goes from 0..MAX_PRIO-1, valid RT
	 * priority is 0..MAX_RT_PRIO-1, and SCHED_NORMAL/SCHED_BATCH
	 * tasks are in the range MAX_RT_PRIO..MAX_PRIO-1. Priority
	 * values are inverted: lower p->prio value means higher priority.
	 */
	g_ctx_r.fd = fd;
	g_ctx_r.cmd = VPUD_GET_OBJECT;
	for (i = 0; i < IPI_INST_CNT; i++) {
		ret = create_rt_thread(&thread_r[i], receiver, (void *)(long)i, SCHED_FIFO, 1);
		assert(ret == 0);
	}
#endif
}

#ifndef NO_FUSE
static void mdpd_open(fuse_req_t req, struct fuse_file_info *fi)
{
	fuse_reply_open(req, fi);
}

static void mdpd_ioctl(fuse_req_t req, int cmd, void *arg,
                       struct fuse_file_info *fi, unsigned flags,
                       const void *in_buf, size_t in_bufsz, size_t out_bufsz)
{
	(void)fi;

	if (flags & FUSE_IOCTL_COMPAT) {
		// Skip to let it continue
	}

	switch (cmd) {
	case VPU_SET_OBJECT:
		if (!in_bufsz) {
			struct iovec iov = { arg, sizeof(struct share_obj) };
			fuse_reply_ioctl_retry(req, &iov, 1, NULL, 0);
		} else {
			struct share_obj obj;
			obj = *(struct share_obj *)in_buf;
			ipi_handler(&obj);
			fuse_reply_ioctl(req, 0, NULL, 0);
		}
		break;

	default:
		fprintf(stderr, "error cmd\n");
		fuse_reply_err(req, EINVAL);
	}
}

extern void mdp_dbg_para(void);
extern void mdp_dbg_proc(const char *buf, size_t count);

static void mdpd_read(fuse_req_t req, size_t size,
					  off_t off, struct fuse_file_info *fi)
{
	(void)fi;
	mdp_dbg_para();
	fuse_reply_err(req, 0);
}

static void mdpd_write(fuse_req_t req, const char *buf, size_t size,
					  off_t off, struct fuse_file_info *fi)
{
	(void)fi;
	mdp_dbg_proc(buf, size);
	fuse_reply_write(req, size);
}

struct mdpd_param {
	unsigned		major;
	unsigned		minor;
	char			*dev_name;
	int			is_help;
};

#define MDPD_OPT(t, p) { t, offsetof(struct mdpd_param, p), 1 }

static const struct fuse_opt mdpd_opts[] = {
	MDPD_OPT("-M %u",	major),
	MDPD_OPT("--maj=%u",	major),
	MDPD_OPT("-m %u",	minor),
	MDPD_OPT("--min=%u",	minor),
	MDPD_OPT("-n %s",	dev_name),
	MDPD_OPT("--name=%s",	dev_name),
	FUSE_OPT_KEY("-h",	0),
	FUSE_OPT_KEY("--help",	0),
	FUSE_OPT_END
};

static int mdpd_process_arg(void *data, const char *arg, int key,
                            struct fuse_args *outargs)
{
	struct mdpd_param *param = data;

	(void)outargs;
	(void)arg;

	switch (key) {
	case 0:
		param->is_help = 1;
		fprintf(stderr, "%s", usage);
		return fuse_opt_add_arg(outargs, "-ho");
	default:
		return 1;
	}
}

static const struct cuse_lowlevel_ops mdpd_clop = {
	.init		= mdpd_init,
	.open		= mdpd_open,
	.ioctl		= mdpd_ioctl,
	.read		= mdpd_read,
	.write		= mdpd_write,
};
#endif

#ifdef NO_FUSE

/* We should handle signal SIGTERM for REBOOT */
static struct sigaction g_old_act;
static void daemon_sighandler(int signum)
{
	switch(signum) {
	case SIGTERM:
		fprintf(stderr, "[mdpd] receive SIGTERM\n");
		/* fix reboot issue, if deamon get signal, exit directly
		g_fg_exit = 1;
		send_uninit_fin();
		//fprintf(stderr, "[mdpd] send_uninit_fin\n");
		usleep(50*1000); // 50ms
		//fprintf(stderr, "[mdpd] sleep 50ms\n");
		*/
		exit(0);
		break;
	default:
		fprintf(stderr, "[mdpd] receive signal %d\n", signum);
		break;
	};
}

static void daemon_sighandler_init(void)
{
	struct sigaction new_act;

	new_act.sa_handler = daemon_sighandler;
	sigemptyset(&new_act.sa_mask);
	new_act.sa_flags = 0;

	sigaction(SIGTERM, &new_act, &g_old_act);
}

static void daemon_sighandler_uninit(void)
{
	sigaction(SIGTERM, &g_old_act, NULL);
}

int main(int argc, char **argv)
{
	int i;

	daemon_sighandler_init();

	if (mdp_support() == 0) {
		fprintf(stderr, "mdp unsupport!\n");
		return -1;
	}

	fprintf(stderr, "mdpd without fuse\n");

	mdpd_init(NULL, NULL);

	for (i = 0; i < IPI_INST_CNT; i++) {
		#ifndef IPI_SYNC_ONLY
		pthread_join(thread_s[i], NULL);
		#endif
		pthread_join(thread_r[i], NULL);
	}

	fprintf(stderr, "[mdpd] threads exited\n");

	daemon_sighandler_uninit();
	fprintf(stderr, "[mdpd] exited\n");

	return 0;
}
#else
int main(int argc, char **argv)
{
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	struct mdpd_param param = { 0, 0, NULL, 0 };
	char dev_name[128] = "DEVNAME=";
	const char *dev_info_argv[] = { dev_name };
	struct cuse_info ci;

	if (mdp_support() == 0) {
		fprintf(stderr, "mdp unsupport!\n");
		return -1;
	}

	if (fuse_opt_parse(&args, &param, mdpd_opts, mdpd_process_arg)) {
		fprintf(stderr, "failed to parse option\n");
		return 1;
	}

	if (!param.is_help) {
		strncat(dev_name, param.dev_name ? param.dev_name : DEV_NAME,
		        sizeof(dev_name) - 9);
	}

	memset(&ci, 0, sizeof(ci));
	ci.dev_major = param.major;
	ci.dev_minor = param.minor;
	ci.dev_info_argc = 1;
	ci.dev_info_argv = dev_info_argv;

	return cuse_lowlevel_main(args.argc, args.argv, &ci, &mdpd_clop,
	                          NULL);
}
#endif
