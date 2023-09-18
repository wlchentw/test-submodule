#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <unistd.h>
#include <signal.h>

#include <sys/time.h>
#include <time.h>


/*
* printf time stamp like android log
*/
void printf_timestamp(void)
{
	char timebuf[32];
	char timebuf_final[128];
	struct timeval timeval;
	struct tm *tm;

	memset(timebuf, 0, 32);
	memset(timebuf_final, 0, 128);

	gettimeofday(&timeval, NULL);

	tm = localtime(&timeval.tv_sec);
	strftime(timebuf, sizeof(timebuf), "%m-%d %H:%M:%S", tm);
	snprintf(timebuf_final, 128, "%s.%03ld", timebuf, timeval.tv_usec / 1000);
	printf("%s ", timebuf_final);
}

void mtk_printf(const char *fmt, ...)
{
	char msg[4096];
	va_list params;
	va_start(params, fmt);
	vsnprintf(msg, sizeof(msg), fmt, params);
	printf_timestamp();
	printf("%s", msg);
	va_end(params);
}
void mtk_fprintf(FILE *fp, const char *fmt, ...)
{
	char msg[4096];
	va_list params;
	va_start(params, fmt);
	vsnprintf(msg, sizeof(msg), fmt, params);
	printf_timestamp();
	fprintf(fp, "%s", msg);
	va_end(params);
}

int mtk_run_shell_cmd(char *cmd,
			char *reply,
			size_t buflen,
			size_t *reply_len,
			void *ret,
			void (*cb)(char *msg, size_t len, void *ret))
{
	FILE *pp;
	mtk_printf("%s running cmd: %s\n", __func__, cmd);
	pp = popen(cmd, "r"); /* read is enough */
	if (!pp) {
		mtk_printf("%s cmd: %s failed\n", __func__, cmd);
		pclose(pp);
		return -1;
	}
	if (reply && reply_len) {
		memset(reply, 0, buflen);
		*reply_len = fread(reply, sizeof(char), buflen, pp);
	}
	if (cb)
		cb(reply, *reply_len, ret);

	pclose(pp);
	return 0;
}
static int tokenize_line(char *cmd, char *argv[], int len)
{
	char *pos;
	char *start;
	int argc = 0;

	start = pos = cmd;
	for (;;) {
		argv[argc] = pos;
		argc++;
		while (*pos != '\n') {
			pos++;
			if (pos - start >= len)
				break;
		}
		if (*pos == '\n') {
			*pos++ = '\0';
			if (pos - start >= len)
				break;
		}
	}

	return argc;
}
void kill_thread(char *threadname)
{
	FILE *pp;
	char buf[512 * 2];
	char cmd[128];
	char pingPid[64];
	char *argv[3];
	int argc;
	char *pos = pingPid;
	char *pingShell = NULL;
	char *pid = NULL;
	size_t readlen = 0;
	int i;

	pid_t ppid = 0;
	memset(buf, 0, 512 * 2);
	memset(cmd, 0, 128);
	memset(pingPid, 0, 64);
	mtk_printf("%s Killing thread %s...\n", __func__,
			threadname);
	snprintf(cmd, 128, "ps %s", threadname);
	mtk_run_shell_cmd(cmd, buf, 512 * 2, &readlen, NULL, NULL);
	if (readlen) {
		argc = tokenize_line(buf, argv, readlen);
		for (i = 0; i < argc; i++) {
			mtk_printf("%s argv[%d]: %s\n", __func__, i, argv[i]);
			if (strstr(argv[i], threadname)) {
				pid = argv[i];
				/* move pid until encount space */
				while (*pid != ' ')
					pid++;

				/* skip space */
				while (*pid == ' ' || *pid == '\t')
					pid++;
				/*
				*copy pid string to pingPid
				*USER     PID   PPID  VSIZE  RSS     WCHAN    PC        NAME
				*root      23504 13498 1004   304   00000000 b6eff848 R ping
				*/
				while (*pid != ' ')
					*pos++ = *pid++;
				ppid = atoi(pingPid);
				mtk_printf("ppid %d\n", ppid);
				kill(ppid, SIGINT);
			}
		}
	}
}
static void usage()
{
	char *usage = "killthread thread1 thread2 ...\n";
	mtk_printf("%s", usage);
}
int main(int argc, char *argv[])
{
	int i;
	/*
	* argv[0] == killthread
	* argv[1] == threadname being killed
	*/
	if (argc < 2) {
		usage();
		return -1;
	}
	/*
	*Starting from argv[1], the first being-killed-thread name
	*/
	for (i = 1; i < argc; i++) {
		kill_thread(argv[i]);
	}
	return 0;
}
