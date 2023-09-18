#define LOG_TAG "TWRITE"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/prctl.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <inttypes.h>
#include <dirent.h>
#include <libgen.h>

#include "osi/include/osi.h"
#include "osi/include/log.h"
#include "osi/include/properties.h"
#include "stack_config.h"

#include "mdroid_buildcfg.h"
#include "twrite.h"

#define MAX_NODE_SIZE 2048
#define MAX_LOG_FILE_SIZE   300000000 // max log file size in log alone mode
#define TRIMMED_PACKET_SIZE   50
#define DUMMY_PACKET_SIZE   31

#define PROP_SNOOP_LOG_POS "persist.service.bdroid.loglen"

#define BTSNOOP_HEADER "btsnoop\0\0\0\0\1\0\0\x3\xea"

#define BTSNOOP_HEADER_SIZE 16

#define RW(pointer, member) ((*pointer)->member)

enum {
    B_NORMAL,
    B_END,      // buffer didn't user, pop after all data write into fd
    B_SEEK,     // buffer should be seek before pop
};

typedef struct _BUFFER_NODE {
    struct _BUFFER_NODE *next;
    unsigned char buffer[MAX_NODE_SIZE];
    volatile unsigned char *start, *end;
    volatile int state;
    volatile long seek_offset;
    volatile int seek_origin;
    volatile unsigned char *end_pos; /* every line end position */
} BUFFER_NODE;

typedef struct _BUFFER_Q {
    volatile BUFFER_NODE *bufferQHead;
    volatile BUFFER_NODE *bufferQTail;
} BUFFER_Q;

typedef struct _TWRITE_FD {
    int fd;
    int writeThreadRun;
    int error;
    BUFFER_Q bufferQ;

    pthread_t writeThread;
    pthread_cond_t writeCond;
    pthread_mutex_t mtx;
} TWRITE_FD;

static void initNode(volatile BUFFER_NODE *node);
static void initQ(BUFFER_Q *bufferQ);
static void deinitQ(BUFFER_Q *bufferQ);
// incress buffer size, should only use when buffer fill
static void pushQ(BUFFER_Q *bufferQ);
// free empyt buffer node
static void popQ(BUFFER_Q *bufferQ);
static void *writeThread();

static int q_cnt = 0;

static TWRITE_FD *twrite_fd = NULL;

BTSNOOP_LOG_MODE m_log_mode = BTSNOOP_LOG_MODE_DEFAULT;
static int twrite_file_index_in_multi_mode = 0;
static char twrite_log_file_path[PATH_MAX] = {0};
static int twrite_max_file_num = 0;

int btsnoop_logging_pos = 0;
int btsnoop_log_size = 0;
int btsnoop_trimmed_size = 0;

static bool validateLogPath(const char *log_path) {
  char log_dir[MTK_STACK_CONFIG_FPATH_LEN];
  char tmp[MTK_STACK_CONFIG_FPATH_LEN];
  int path_len = strlen(log_path);

  LOG_INFO(LOG_TAG, "%s M_BTCONF log_path: %s", __func__, log_path);
  if (0 < path_len && path_len < MTK_STACK_CONFIG_FPATH_LEN) {
    int i = 0;
    strncpy(log_dir, log_path, MTK_STACK_CONFIG_FPATH_LEN);
    log_dir[MTK_STACK_CONFIG_FPATH_LEN - 1] = '\0';
    while (log_dir[i]) {
      tmp[i] = log_dir[i];
      if (log_dir[i] == '/' && i) {
        tmp[i] = '\0';
        LOG_INFO(LOG_TAG, "%s tmp: %s", __func__, tmp);
        if (access(tmp, F_OK) != 0) {
          if (mkdir(tmp, 0770) == -1) {
            LOG_ERROR(LOG_TAG, "mkdir error! %s", (char*)strerror(errno));
            break;
          }
        }
        tmp[i] = '/';
      }
      i++;
    }
  } else {
    LOG_ERROR(LOG_TAG, "%s M_BTCONF log_path is longer then 1024", __func__);
    return false;
  }
  return true;
}

static uint64_t readability_timestamp(void)
{
    const int buf_size = 24;
    char buffer[buf_size];
    char buffer2[buf_size];
    struct timeval tv;
    time_t curtime = time(NULL);
    struct tm * ltime = localtime(&curtime);
    strftime(buffer, sizeof(buffer), "%Y%m%d%H%M%S", ltime);

    gettimeofday(&tv, NULL);
    snprintf(buffer2, sizeof(buffer2), "%s%ld", buffer, tv.tv_usec / 1000);

    return strtoull(buffer2, NULL, 10);
}

int twrite_lseek(long offset, int origin)
{
    volatile BUFFER_NODE **bQTail = &twrite_fd->bufferQ.bufferQTail;

    LOG_INFO(LOG_TAG, "%s", __func__);
    RW(bQTail, state) = B_SEEK;
    RW(bQTail, seek_offset) = offset;
    RW(bQTail, seek_origin) = origin;
    pushQ(&twrite_fd->bufferQ);

    return 0;
}

static void twrite_remove_history_file(const char * log_path)
{
    char dir_path[PATH_MAX] = {0};
    char file_path[PATH_MAX] = {0};

    char *dir_name = NULL;
    char *file_name = NULL;
    char full_path[PATH_MAX] = {0};

    strncpy(dir_path, log_path, PATH_MAX);
    dir_path[PATH_MAX - 1] = '\0';
    strncpy(file_path, log_path, PATH_MAX);
    file_path[PATH_MAX - 1] = '\0';

    dir_name = dirname(dir_path);
    dir_name = dir_path;
    file_name = basename(file_path);

    if (dir_name == NULL || file_name == NULL)
    {
        return;
    }

    LOG_INFO(LOG_TAG, "%s log_path=%s, dir_name=%s, file_name=%s",
            __func__, log_path, dir_name, file_name);

    DIR *p_dir = opendir(dir_name);
    if (p_dir != NULL)
    {
        struct dirent *p_file;
        while ((p_file = readdir(p_dir)) != NULL)
        {
            if (strncmp(p_file->d_name, "..", 2) == 0
                || strncmp(p_file->d_name, ".", 1) == 0)
            {
                continue;
            }
            if (strstr(p_file->d_name, file_name) != NULL) {
                memset(full_path, 0, sizeof(full_path));
                snprintf(full_path, sizeof(full_path), "%s/%s", dir_name, p_file->d_name);
                if (remove(full_path)) {
                    LOG_ERROR(LOG_TAG, "%s can't remove", full_path);
                } else {
                    LOG_INFO(LOG_TAG, "%s removed", full_path);
                }
            }
        }
        closedir(p_dir);
    }
    return;
}

static void twrite_rename_history_file(const char * log_path)
{
    char dir_path[PATH_MAX] = {0};
    char file_path[PATH_MAX] = {0};

    char *dir_name = NULL;
    char *file_name = NULL;
    char full_path[PATH_MAX] = {0};
    char last_path[PATH_MAX] = {0};
    uint64_t read_ts = readability_timestamp();

    strncpy(dir_path, log_path, PATH_MAX);
    dir_path[PATH_MAX - 1] = '\0';
    strncpy(file_path, log_path, PATH_MAX);
    file_path[PATH_MAX - 1] = '\0';

    dir_name = dirname(dir_path);
    dir_name = dir_path;
    file_name = basename(file_path);

    if (dir_name == NULL || file_name == NULL)
    {
        return;
    }

    LOG_INFO(LOG_TAG, "%s dir_name=%s, file_nanme=%s",
            __func__, dir_name, file_name);

    DIR *p_dir = opendir(dir_name);
    if (p_dir != NULL)
    {
        struct dirent *p_file;
        while ((p_file = readdir(p_dir)) != NULL)
        {
            if (strncmp(p_file->d_name, "..", 2) == 0
                || strncmp(p_file->d_name, ".", 1) == 0)
            {
                continue;
            }
            if (strstr(p_file->d_name, file_name) != NULL) {
                memset(full_path, 0, sizeof(full_path));
                snprintf(full_path, sizeof(full_path), "%s/%s", dir_name, p_file->d_name);
                snprintf(last_path, PATH_MAX, "%s/%s.%" PRIu64, dir_name, p_file->d_name, read_ts);
                if (rename(full_path, last_path) && errno != ENOENT) {
                    LOG_ERROR(LOG_TAG, "%s unable to rename '%s' to '%s': %s", __func__,
                        full_path, last_path, strerror(errno));
                }

            }
        }
        closedir(p_dir);
    }
    return;
}

static void twrite_rename_old_file(const char * log_path, int old_idx, int new_idx)
{
    char dir_path[PATH_MAX] = {0};
    char file_path[PATH_MAX] = {0};

    char *dir_name = NULL;
    char *file_name = NULL;
    char new_path[PATH_MAX] = {0};
    char last_path[PATH_MAX] = {0};
    char temp_filename[PATH_MAX] = {0};
    char timestamp_buffer[24];

    int log_file_access = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;
    int log_file_mode = O_WRONLY | O_CREAT | O_TRUNC;

    struct timeval tv;

    strncpy(dir_path, log_path, PATH_MAX);
    dir_path[PATH_MAX - 1] = '\0';
    strncpy(file_path, log_path, PATH_MAX);
    file_path[PATH_MAX - 1] = '\0';

    dir_name = dirname(dir_path);
    dir_name = dir_path;
    file_name = basename(file_path);

    //LOG_INFO(LOG_TAG, "%s log_path=%s, dir_name=%s, file_name=%s",
    //        __func__, log_path, dir_name, file_name);

    gettimeofday(&tv, NULL);
    struct tm *ltime = localtime(&tv.tv_sec);
    strftime(timestamp_buffer, sizeof(timestamp_buffer), "%Y%m%d%H%M%S", ltime);
    snprintf(new_path, sizeof(new_path), "%s.%d.%s", log_path, new_idx, timestamp_buffer);

    if (old_idx >= 0)
    {
        DIR *p_dir = opendir(dir_name);
        if (p_dir != NULL)
        {
            struct dirent *p_file;
            while ((p_file = readdir(p_dir)) != NULL)
            {
                if (strncmp(p_file->d_name, "..", 2) == 0
                    || strncmp(p_file->d_name, ".", 1) == 0)
                {
                    continue;
                }

                snprintf(temp_filename, sizeof(temp_filename), "%s.%d", file_name, old_idx);
                if (strstr(p_file->d_name, temp_filename) != NULL)
                {
                    memset(last_path, 0, sizeof(new_path));
                    snprintf(last_path, sizeof(last_path), "%s/%s", dir_name, p_file->d_name);

                    if (rename(last_path, new_path) && errno != ENOENT)
                    {
                        LOG_ERROR(LOG_TAG, "%s unable to rename '%s' to '%s': %s",
                            __func__, last_path, new_path, strerror(errno));
                    }
                    else
                    {
                        LOG_INFO(LOG_TAG, "rename '%s' to '%s'",
                            last_path, new_path);
                    }
                }
            }
            closedir(p_dir);
        }
    }

    twrite_fd->fd = open(new_path, log_file_mode, log_file_access);
    if (twrite_fd->fd == INVALID_FD)
    {
        twrite_fd->fd = -1;
        LOG_ERROR(LOG_TAG, "%s unable to open '%s': %s", __func__,
            new_path, strerror(errno));
    }

    return;
}

static void twrite_check_for_new_file(int write_bytes, int need_check)
{
    if (m_log_mode != BTSNOOP_LOG_MODE_MULTI)
    {
        return;
    }

    //LOG_ERROR(LOG_TAG, "twrite_check_for_new_file btsnoop_logging_pos=%d, write_bytes=%d, btsnoop_log_size=%d, need_check=%d",
    //        btsnoop_logging_pos, write_bytes, btsnoop_log_size, need_check);

    if ((btsnoop_logging_pos + write_bytes >= btsnoop_log_size) && need_check)
    {
        btsnoop_logging_pos = 0;

        fsync(twrite_fd->fd);
        close(twrite_fd->fd);

        /* if limit the number of the file, remove the old one */
        if (twrite_max_file_num > 0)
        {
            twrite_rename_old_file(twrite_log_file_path,
                twrite_file_index_in_multi_mode-twrite_max_file_num,
                twrite_file_index_in_multi_mode);
            twrite_file_index_in_multi_mode++;
        }
    }

    btsnoop_logging_pos += write_bytes;

    return;
}

bool twrite_init(const char * log_path, BTSNOOP_LOG_MODE log_mode, int * fd, int max_file_num)
{
    int logfile_fd = INVALID_FD;
    char log_path_local[PATH_MAX] = {0};
    int log_file_mode = O_WRONLY | O_CREAT | O_TRUNC;
    int log_file_access = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;

#if defined(MTK_LINUX) && defined(MTK_COMMON) && (MTK_COMMON == TRUE)
    pthread_condattr_t condattr;
    pthread_condattr_init(&condattr);
    pthread_condattr_setclock(&condattr, CLOCK_MONOTONIC);
#endif

    *fd = INVALID_FD;
    if (!validateLogPath(log_path)) {
        return false;
    }
    strncpy(twrite_log_file_path, log_path, PATH_MAX);
    twrite_log_file_path[PATH_MAX - 1] = '\0';
    if (stack_config_get_interface()->get_btsnoop_should_save_last()) {
        if (log_mode == BTSNOOP_LOG_MODE_MULTI)
        {
            twrite_rename_history_file(log_path);
        }
        else
        {
            char last_log_path[PATH_MAX];
            snprintf(last_log_path, PATH_MAX, "%s.%" PRIu64, log_path, readability_timestamp());
            if (rename(log_path, last_log_path) && errno != ENOENT) {
                LOG_ERROR(LOG_TAG, "%s unable to rename '%s' to '%s': %s", __func__,
                    log_path, last_log_path, strerror(errno));
            }
        }
    }
    else
    {
        twrite_remove_history_file(log_path);
    }

    twrite_fd = (TWRITE_FD *)malloc(sizeof(TWRITE_FD));
    if (NULL == twrite_fd) {
        LOG_ERROR(LOG_TAG, "%s cannot malloc twrite_fd errno = %d (%s)",
                __func__, errno, strerror(errno));
        goto ERROR;
    }

    m_log_mode = log_mode;
    twrite_max_file_num = max_file_num;

#if defined(MTK_LINUX_SNOOP_CONFIG) && (MTK_LINUX_SNOOP_CONFIG == TRUE)
    btsnoop_log_size = stack_config_get_interface()->get_btsnoop_max_log_file_size();
    btsnoop_trimmed_size = stack_config_get_interface()->get_btsnoop_trimmed_packet_size();
    if (BTSNOOP_LOG_MODE_ALONE == m_log_mode) {
        if (0 == btsnoop_log_size)
            btsnoop_log_size = MAX_LOG_FILE_SIZE;
        if (0 == btsnoop_trimmed_size)
            btsnoop_trimmed_size = TRIMMED_PACKET_SIZE;
    }
    else if (BTSNOOP_LOG_MODE_MULTI == m_log_mode) {
        if (0 == btsnoop_log_size)
            btsnoop_log_size = MAX_LOG_FILE_SIZE;
    }
    LOG_INFO(LOG_TAG, "[BT] m_log_mode %d, btsnoop_log_size %d, btsnoop_trimmed_size %d, file_number=%d",
        m_log_mode, btsnoop_log_size, btsnoop_trimmed_size, twrite_max_file_num);
#endif

    strncpy(log_path_local, log_path, PATH_MAX);
    log_path_local[PATH_MAX - 1] = '\0';
    if (m_log_mode == BTSNOOP_LOG_MODE_ALONE) {
        char logging_pos_prop[PROPERTY_VALUE_MAX] = {0};
        // Modify Log Path and file open flag
        if (sizeof(log_path_local) > strlen(log_path_local) + 1)
            strncat(log_path_local, "_MTK", sizeof(log_path_local) - strlen(log_path_local) - 1);
        log_file_mode = O_WRONLY | O_CREAT;
        if(access(log_path_local, F_OK) != 0) {
            btsnoop_logging_pos = 0;
            sprintf(logging_pos_prop, "%d", btsnoop_logging_pos);
            osi_property_set(PROP_SNOOP_LOG_POS, logging_pos_prop);
        } else {
            osi_property_get(PROP_SNOOP_LOG_POS, logging_pos_prop, "0");
            btsnoop_logging_pos = atoi(logging_pos_prop);
        }
        LOG_INFO(LOG_TAG, "[BT] M_BTCONF log fetch file len: %d", btsnoop_logging_pos);
    }
    else if (m_log_mode == BTSNOOP_LOG_MODE_MULTI)
    {
        struct timeval tv;
        char timestamp_buffer[24];

        gettimeofday(&tv, NULL);
        struct tm *ltime = localtime(&tv.tv_sec);
        strftime(timestamp_buffer, sizeof(timestamp_buffer), "%Y%m%d%H%M%S", ltime);
        sprintf(log_path_local, "%s.%d.%s", twrite_log_file_path,
            twrite_file_index_in_multi_mode, timestamp_buffer);
        twrite_file_index_in_multi_mode++;
        btsnoop_logging_pos = 0;
    }
    logfile_fd = open(log_path_local, log_file_mode, log_file_access);
    if (logfile_fd == INVALID_FD) {
      LOG_ERROR(LOG_TAG, "%s unable to open '%s': %s", __func__, log_path_local, strerror(errno));
      goto ERROR;
    }

    initQ(&twrite_fd->bufferQ);
    twrite_fd->fd = logfile_fd;
    twrite_fd->writeThreadRun = 1;
    twrite_fd->error = 0;
    pthread_mutex_init(&twrite_fd->mtx, NULL);
#if defined(MTK_LINUX) && defined(MTK_COMMON) && (MTK_COMMON == TRUE)
    pthread_cond_init(&twrite_fd->writeCond, &condattr);
#else
    pthread_cond_init(&twrite_fd->writeCond, NULL);
#endif
    pthread_mutex_lock(&twrite_fd->mtx);

    if (0 != pthread_create(&twrite_fd->writeThread, NULL, writeThread,
                (void*)twrite_fd)) {
        LOG_ERROR(LOG_TAG, "create write thread failed");
        goto ERROR;
    }

    //pthread_mutex_lock(&twrite_fd->mtx);
    pthread_mutex_unlock(&twrite_fd->mtx);

    // M: Write header
    /* if power off/on, then we don't write the head again, because its head will
     * at the first xxx.log.0.
     */
    if (m_log_mode != BTSNOOP_LOG_MODE_MULTI || twrite_file_index_in_multi_mode == 1)
    {
        int nWritten = write(logfile_fd, BTSNOOP_HEADER, BTSNOOP_HEADER_SIZE);
        if (nWritten != BTSNOOP_HEADER_SIZE) {
          LOG_INFO(LOG_TAG, "write btsnoop header failed : nWritten=%d", nWritten);
          if (nWritten < 0)
            LOG_ERROR(LOG_TAG, "write btsnoop header failed: err=%s, errno=%d", strerror(errno), errno);
          twrite_deinit();
          goto ERROR;
        }
    }

    if (m_log_mode == BTSNOOP_LOG_MODE_ALONE) {
      if (btsnoop_logging_pos == 0)
        btsnoop_logging_pos += BTSNOOP_HEADER_SIZE;
      else
        twrite_lseek(btsnoop_logging_pos, SEEK_SET);
    }
    *fd = logfile_fd;
    return true;
ERROR:
    if (twrite_fd != NULL) {
      free(twrite_fd);
      twrite_fd = NULL;
    }
    if (logfile_fd != INVALID_FD) {
      close(logfile_fd);
      logfile_fd = INVALID_FD;
    }
    LOG_ERROR(LOG_TAG, "There is no hci log in this run!");
    return false;
}

int twrite_deinit()
{
    int log_fd = -1;
    if (NULL == twrite_fd) {
        LOG_ERROR(LOG_TAG, "deinit with NULL fd");
        return -1;
    }
    if (0 == twrite_fd->writeThreadRun) { // if deinit twice ignore
        LOG_ERROR(LOG_TAG, "sniffer thread already end");
        return -2;
    }
    twrite_fd->writeThreadRun = 0;
    if (pthread_cond_signal(&twrite_fd->writeCond)) {
        LOG_ERROR(LOG_TAG, "deinit pthread_cond_signal failed");
    }
    pthread_join(twrite_fd->writeThread, NULL);
    deinitQ(&twrite_fd->bufferQ);

    log_fd = twrite_fd->fd;

    free(twrite_fd);
    twrite_fd = NULL;

    if (m_log_mode == BTSNOOP_LOG_MODE_ALONE) {
        char logging_pos_prop[PROPERTY_VALUE_MAX] = {0};
        sprintf(logging_pos_prop, "%d", btsnoop_logging_pos);
        osi_property_set(PROP_SNOOP_LOG_POS, logging_pos_prop);
        LOG_INFO(LOG_TAG, "%s logging pos is: %s", __func__, logging_pos_prop);
    }

    LOG_DEBUG(LOG_TAG, "%s: done", __func__);
    return log_fd;
}

int twrite_write(const unsigned char *data, int len, int last_frag)
{
    const unsigned char *p, *e;
    volatile unsigned char *bufferEnd;
    volatile unsigned char *QTailEnd;
    volatile BUFFER_NODE **bQTail = &twrite_fd->bufferQ.bufferQTail;
    //LOG_DEBUG(LOG_TAG, "data len = %d", len);
    if (twrite_fd == NULL || twrite_fd->fd < 0) {
      return -1;
    }
    if (twrite_fd->error) {
        LOG_ERROR(LOG_TAG, "write thread have error, ignore err=%s, errno=%d",
                strerror(twrite_fd->error), twrite_fd->error);
        return -1;
    }
    p = data;
    e = data + len;
    if (NULL == *bQTail) {
        pushQ(&twrite_fd->bufferQ);
    }

    // log longer than left buffer size
    while (RW(bQTail, buffer) + MAX_NODE_SIZE - RW(bQTail, end) < (e - p)) {
        bufferEnd = RW(bQTail, buffer) + MAX_NODE_SIZE;
        QTailEnd = RW(bQTail, end);
        while (QTailEnd != bufferEnd) { // memcpy
            *(QTailEnd++) = *(p++);
        }
        RW(bQTail, end) = QTailEnd;
        pushQ(&twrite_fd->bufferQ);
    }
    QTailEnd = RW(bQTail, end);
    while (e != p) { // memcpy
        *(QTailEnd++) = *(p++);
    }
    RW(bQTail, end) = QTailEnd;

    if (last_frag)
    {
        RW(bQTail, end_pos) = QTailEnd;
    }

    if (pthread_cond_signal(&twrite_fd->writeCond)) {
        LOG_ERROR(LOG_TAG, "%s: pthread_cond_signal failed", __func__);
        return -1;
    }
    return len;
}

void twrite_write_packet(SNOOP_WRITE_IN_THREAD btsnoop_write, int type,
    const uint8_t *packet, int length_he_in, int length_in, int flags_in,
    int drops_in, uint32_t time_hi_in, uint32_t time_lo_in) {
  //refer to definition in btsnoop_net.c
  const int kAclPacket = 2;
  const int kEventPacket = 4;
  uint8_t acc_header[32] = {'\0'};
  uint8_t dummy_data[90] = {'\0'};
  uint32_t drops_MTK = 0xFEEDF00D ;
  int length_he;
  int length;
  int flags;
  int drops;
  uint32_t time_hi;
  uint32_t time_lo;
  if (m_log_mode == BTSNOOP_LOG_MODE_ALONE)
    drops = htonl(drops_MTK);
  else
    drops = htonl(drops_in);

  time_hi = htonl(time_hi_in);
  time_lo = htonl(time_lo_in);

  length_he = htonl(length_he_in);
  if(m_log_mode == BTSNOOP_LOG_MODE_ALONE) {
    if(length_he_in > btsnoop_trimmed_size && type == kAclPacket)
          length_in = btsnoop_trimmed_size;

    // Write a dummy packet to full fill the last content of file.
    if(btsnoop_logging_pos + length_in + 24 > btsnoop_log_size - DUMMY_PACKET_SIZE) {
        uint8_t acc_header[32] = {'\0'};
        int flags_dummy, type_dummy, length_dummy;
        flags_dummy = htonl(3);
        type_dummy = kEventPacket;
        length_dummy = btsnoop_log_size - btsnoop_logging_pos;
        length_dummy = htonl(length_dummy);
        dummy_data[0] = 0xff;
        dummy_data[1] = 0xf1;
        memcpy(acc_header, &length_dummy, 4);
        memcpy(acc_header+4, &length_dummy, 4);
        memcpy(acc_header+8, &flags_dummy, 4);
        memcpy(acc_header+12, &drops, 4);
        memcpy(acc_header+16, &time_hi, 4);
        memcpy(acc_header+20, &time_lo, 4);
        memcpy(acc_header+24, &type_dummy, 1);
        length_dummy = ntohl(length_dummy);
        btsnoop_write(acc_header, 25, 0);
        btsnoop_write(dummy_data, length_dummy - 1, 1);
        twrite_lseek(BTSNOOP_HEADER_SIZE, SEEK_SET);
        LOG_INFO(LOG_TAG, "[BT] M_BTCONF should_log_MTK twrite_lseek: %d", btsnoop_logging_pos);
        btsnoop_logging_pos = BTSNOOP_HEADER_SIZE;
    }
    btsnoop_logging_pos += (length_in + 24);
  }
  length = htonl(length_in);
  flags = htonl(flags_in);
  memcpy(acc_header, &length_he, 4);
  memcpy(acc_header+4, &length, 4);
  memcpy(acc_header+8, &flags, 4);
  memcpy(acc_header+12, &drops, 4);
  memcpy(acc_header+16, &time_hi, 4);
  memcpy(acc_header+20, &time_lo, 4);
  memcpy(acc_header+24, &type, 1);
  btsnoop_write(acc_header, 25, 0);
  btsnoop_write(packet, length_in - 1, 1);
}

static void initQ(BUFFER_Q *bufferQ)
{
    bufferQ->bufferQHead = malloc(sizeof(BUFFER_NODE));
    if (NULL == bufferQ->bufferQHead)
    {
        return;
    }
    bufferQ->bufferQTail = bufferQ->bufferQHead;
    initNode(bufferQ->bufferQHead);
    return;
}

static void deinitQ(BUFFER_Q* bufferQ)
{
    while(bufferQ->bufferQHead->next) {
        popQ(bufferQ);
    }
    free((void*)bufferQ->bufferQHead);
    return;
}

static void initNode(volatile BUFFER_NODE *node)
{
    node->next = NULL;
    node->start = node->buffer;
    node->end = node->buffer;
    node->state = B_NORMAL;
    node->seek_offset = 0;
    node->seek_origin = SEEK_CUR;
    node->end_pos = node->buffer;
}

static void popQ(BUFFER_Q *bufferQ)
{
    volatile BUFFER_NODE *tmp;
    volatile BUFFER_NODE **bQHead = &bufferQ->bufferQHead;
    if (NULL == *bQHead || NULL == RW(bQHead, next) ||
            RW(bQHead, end) > RW(bQHead, start) ||
            RW(bQHead, state) == B_SEEK) {
        // keep one node in queue, and check if all buffer was wrote
        return;
    } else {
        tmp = *bQHead;
    }

    *bQHead = RW(bQHead, next);

    free((void*)tmp);
    q_cnt--;
    return;
}

static void pushQ(BUFFER_Q *bufferQ)
{
    volatile BUFFER_NODE **bQHead = &bufferQ->bufferQHead;
    volatile BUFFER_NODE **bQTail = &bufferQ->bufferQTail;
    BUFFER_NODE *node;
    node = malloc(sizeof(BUFFER_NODE));
    if (NULL == node)
    {
        return;
    }
    initNode(node);
    q_cnt++;

    if (NULL == *bQHead && NULL == *bQTail) {
        *bQHead = node;
        *bQTail = node;
    } else {
        RW(bQTail, next) = node;
        *bQTail = node;
    }
    return;
}

static void *writeThread(void *t_fd)
{
    TWRITE_FD *twrite_fd = (TWRITE_FD *)t_fd;
    volatile BUFFER_NODE **bQHead = &twrite_fd->bufferQ.bufferQHead;
    BUFFER_Q *bufferQ = &twrite_fd->bufferQ;
    int nWritten;
    int bytesToWrite;
    int need_check = 0;
    int bytesToWrite2Flie;

    if (prctl(PR_SET_NAME, (unsigned long)"twrite_logout", 0, 0, 0) == -1) {
      LOG_DEBUG(LOG_TAG, "%s unable to set thread name: %s", __func__, strerror(errno));
    }

    LOG_DEBUG(LOG_TAG, "write Thread start");
#ifdef TWRITE_TEST
    srand(time(NULL));
#endif
    do {
        pthread_cond_wait(&twrite_fd->writeCond, &twrite_fd->mtx);
        //LOG_DEBUG(LOG_TAG, "g Cond: q_cnt(%d)", q_cnt);
        popQ(bufferQ); // try to remove empty Node
        while (RW(bQHead, start) != RW(bQHead, end) ||
                RW(bQHead, state) == B_SEEK) {
#ifdef TWRITE_TEST
            usleep(rand() % 1000000);
#endif
            nWritten = 0;
            if (RW(bQHead, end_pos) - RW(bQHead, start) > 0)
            {
                bytesToWrite2Flie = bytesToWrite = RW(bQHead, end_pos) - RW(bQHead, start);
                need_check = 1;
            }
            else
            {
                bytesToWrite2Flie = bytesToWrite = RW(bQHead, end) - RW(bQHead, start);
                need_check = 0;
            }

            while (bytesToWrite > 0) {
                nWritten = write(twrite_fd->fd, (void*)RW(bQHead, start),
                        bytesToWrite);
                if (nWritten < 0) {
                    twrite_fd->error = errno;
                    LOG_ERROR(LOG_TAG, "write failed : nWritten=%d, err=%s, errno=%d",
                            nWritten, strerror(errno), errno);
                    return NULL;
                }
                //LOG_DEBUG(LOG_TAG, "%db wr fd=%d", nWritten, twrite_fd->fd);
                bytesToWrite -= nWritten;
                RW(bQHead, start) += nWritten;
            }

            twrite_check_for_new_file(bytesToWrite2Flie, need_check);

            if(RW(bQHead, state) == B_SEEK &&
                    RW(bQHead, start) == RW(bQHead, end)) {// process seek buffer
                off_t ret;
                RW(bQHead, state) = B_END;
                LOG_INFO(LOG_TAG, "do lseek");
                ret = lseek(twrite_fd->fd, RW(bQHead, seek_offset), RW(bQHead, seek_origin));
                if (ret < 0) {
                  LOG_ERROR(LOG_TAG, "lseek error: %s", strerror(errno));
                }
                popQ(bufferQ);
            }
            else if (RW(bQHead, start) == RW(bQHead, buffer) + MAX_NODE_SIZE) {
                popQ(bufferQ);
            }
        }
    } while (twrite_fd->writeThreadRun);

    LOG_DEBUG(LOG_TAG, "exit loop of writeThread");
    return NULL;
}

/**
 * Check system property to decide whether override mode is used.
 * TODO: use stack config file to control log mode uniformly.
 */
bool enable_mtk_btsnoop() {
  bool ret = false;
#if defined(MTK_BTSNOOPLOG_MODE_SUPPORT) && (MTK_BTSNOOPLOG_MODE_SUPPORT == TRUE)
  char should_log_MTK_prop[PROPERTY_VALUE_MAX] = {0};
  osi_property_get("bt.hcilogalone", should_log_MTK_prop, "0" );
  LOG_INFO(LOG_TAG, "[BT] M_BTCONF should_log : %s", should_log_MTK_prop);
  ret = atoi(should_log_MTK_prop) ? true : false;
#endif
  return ret;
}
