#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <inttypes.h>
#include <dirent.h>
#include <sys/stat.h>
#include <libgen.h>

#include "stack_config.h"
#include "osi/include/osi.h"
//#if defined(MTK_LINUX_STACK_LOG2FILE) && (MTK_LINUX_STACK_LOG2FILE == TRUE)
#include "osi/include/log.h"
//#endif


typedef void (*bg_log_func)(char *log_str);

bg_log_func s_log_cb = NULL;

static int g_log_fd = INVALID_FD;
static bool fg_only2file = true;
static char g_ts_buffer[24];
static bool fg_log_slice = false;  /* if stack.log make into slice file  */
static int stack_logging_pos = 0; /* stack log file position */
static int stack_file_index_in_slice = 0; /* stack log file index, start from 0 */
static int stack_log_max_file_num = 0;
static int stack_log_size = 0;
static char stack_log_file_path[PATH_MAX] = {0};

static int g_log_buf_size = 0;    /* unit: bytes */
static int g_log_buf_timeout = 0; /* unit: s */

static char *g_log_buf = NULL;
static int g_log_buf_len = 0;

static int g_log_last_write_sec = 0;

static pthread_rwlock_t mw_log_wr_lock = PTHREAD_RWLOCK_INITIALIZER;
#define BT_MW_LOG_LOCK() do{                      \
        pthread_rwlock_wrlock(&mw_log_wr_lock);   \
    } while(0)

#define BT_MW_LOG_UNLOCK() do{                    \
        pthread_rwlock_unlock(&mw_log_wr_lock);   \
    } while(0)

#define TIMESTAMP_TYPE_FILE_NAME   1
#define TIMESTAMP_TYPE_LOG_PREFIX  2

static char* log_timestamp(int type)
{
    char buffer[24] = {0};
    struct timeval tv;

    gettimeofday(&tv, NULL);
    struct tm *ltime = localtime(&tv.tv_sec);

    memset(g_ts_buffer, 0, sizeof(g_ts_buffer));
    if (TIMESTAMP_TYPE_FILE_NAME == type)
    {
        strftime(buffer, sizeof(buffer), "%Y%m%d%H%M%S", ltime);
        snprintf(g_ts_buffer, sizeof(g_ts_buffer), "%s%03ld", buffer, tv.tv_usec / 1000);
    }
    else
    {
        int today_sec = tv.tv_sec % 86400;
        int hour = today_sec / 3600;
        int min = today_sec % 3600 / 60;
        int sec = today_sec % 60;
        snprintf(g_ts_buffer, sizeof(g_ts_buffer), "<%02d:%02d:%02d.%06ld>", hour, min, sec, tv.tv_usec);
    }

    return g_ts_buffer;
}

#if defined(MTK_LINUX_MW_STACK_LOG2FILE) && (MTK_LINUX_MW_STACK_LOG2FILE == TRUE)
EXPORT_SYMBOL int g_log_level = LOG_LVL_WARN;
int log_getLevel()
{
    return g_log_level;
}

void log_setLevel(unsigned int lvl)
{
    if (lvl > LOG_LVL_VERBOSE)
    {
        g_log_level = LOG_LVL_VERBOSE;
        return;
    }

    g_log_level = lvl;
    return;
}

static bool validate_stack_log_path(const char *log_path) {
  char log_dir[PATH_MAX];
  char tmp[PATH_MAX];
  int path_len = strlen(log_path);

  printf("%s btstack log_path: %s\n", __func__, log_path);
  if (0 < path_len && path_len < PATH_MAX) {
    int i = 0;
    strncpy(log_dir, log_path, PATH_MAX);
    log_dir[PATH_MAX - 1] = '\0';

    while (log_dir[i]) {
      tmp[i] = log_dir[i];
      if (log_dir[i] == '/' && i) {
        tmp[i] = '\0';
        printf("%s tmp: %s\n", __func__, tmp);
        if (access(tmp, F_OK) != 0) {
          if (mkdir(tmp, 0770) == -1) {
            printf("mkdir error! %s\n", (char*)strerror(errno));
            break;
          }
        }
        tmp[i] = '/';
      }
      i++;
    }
  } else {
    printf("%s btstack log_path is longer then 1024\n", __func__);
    return false;
  }
  return true;
}

static void log_rename_history_file(const char * log_path)
{
    char dir_path[PATH_MAX] = {0};
    char file_path[PATH_MAX] = {0};

    char *dir_name = NULL;
    char *file_name = NULL;
    char full_path[PATH_MAX] = {0};
    char last_path[PATH_MAX] = {0};

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

    printf("%s log_path=%s, dir_name=%s, file_name=%s\n",
            __func__, log_path, dir_name, file_name);

    DIR *p_dir = opendir(dir_name);
    if (p_dir != NULL)
    {
        struct dirent *p_file;
        char *ts_str = log_timestamp(TIMESTAMP_TYPE_FILE_NAME);
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
                snprintf(last_path, PATH_MAX, "%s/%s.%s", dir_name,
                    p_file->d_name, ts_str);
                if (rename(full_path, last_path) && errno != ENOENT) {
                    printf("%s unable to rename '%s' to '%s': %s\n", __func__,
                        full_path, last_path, strerror(errno));
                }

            }
        }
        closedir(p_dir);
    }
    return;
}

static void log_remove_history_file(const char * log_path)
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
    dir_name = dir_path ;
    file_name = basename(file_path);

    if (dir_name == NULL || file_name == NULL)
    {
        return;
    }

    printf("%s log_path=%s, dir_name=%s, file_name=%s\n",
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
                    printf("%s can't remove\n", full_path);
                } else {
                    printf("%s removed\n", full_path);
                }
            }
        }
        closedir(p_dir);
    }
    return;
}


EXPORT_SYMBOL void save_log2file_init(STACK_LOG_SETTING *log_setting)
{
    bool log2file = log_setting->log2file;
    bool save_last = log_setting->save_last;
    char *log_path = log_setting->log_path;
    int log_level = log_setting->log_level;
    bool only2file = log_setting->only2file;
    bool log_slice = log_setting->log_slice;
    int max_file_num = log_setting->max_file_num;
    int log_size = log_setting->log_size;
    char log_path_local[PATH_MAX] = {0};
    char last_log_path[PATH_MAX] = {0};

    log_setLevel(log_level);

    if (false == log2file)
    {
        g_log_fd = INVALID_FD;
        printf("%s no need save log2file\n", __func__);
        if (0 == access(log_path, F_OK))
        {
            unlink(log_path);
            printf("%d@%s remove the existed log file successful\n", __LINE__, __func__);
        }
        return;
    }

    if (NULL == log_path)
    {
        printf("%s null file name\n", __func__);
        return;
    }

    if (true == save_last)
    {
        if (true == log_slice)
        {
            log_rename_history_file(log_path);
        }
        else
        {
            snprintf(last_log_path, PATH_MAX, "%s.%s", log_path, log_timestamp(TIMESTAMP_TYPE_FILE_NAME));
            if (rename(log_path, last_log_path) && errno != ENOENT)
            {
                printf("%s unable to rename '%s' to '%s': %s\n", __func__, log_path, last_log_path, strerror(errno));
            }
        }
    }
    else
    {
        log_remove_history_file(log_path);
    }

    printf("%s log_path=%s \n", __func__, log_path);
    if (false == validate_stack_log_path(log_path))
    {
        printf("validate_stack_log_path fail\n");
        return;
    }

    strncpy(stack_log_file_path, log_path, PATH_MAX);
    stack_log_file_path[PATH_MAX-1] = 0;

    strncpy(log_path_local, log_path, PATH_MAX);
    log_path_local[PATH_MAX-1] = 0;

    fg_log_slice = log_slice;
    stack_log_max_file_num = max_file_num;
    stack_log_size = (log_size == 0) ? 300000000 : log_size;
    printf("%s fg_log_slice:%d, max_file:%d, log_size:%d\n",
        __func__, fg_log_slice, stack_log_max_file_num, stack_log_size);

    if (true == log_slice)
    {
        if (PATH_MAX > strlen(log_path_local) + 1)
            strncat(log_path_local, ".0", PATH_MAX - strlen(log_path_local) -1);
        stack_file_index_in_slice = 0;
        stack_logging_pos = 0;
    }

    g_log_fd = open(log_path_local, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (INVALID_FD == g_log_fd)
    {
        printf("%s unable to open '%s': %s\n", __func__, log_path_local, strerror(errno));
    }
    printf("%s open stack log success fd:%d\n", __func__, g_log_fd);

    fg_only2file = only2file;


    g_log_buf_size = log_setting->buf_size;
    g_log_buf_timeout = log_setting->timeout;
    if (g_log_buf_size > 0)
    {
        g_log_buf = (char*)malloc(g_log_buf_size);
        if (NULL == g_log_buf)
        {
            printf("%s alloc log buf fail\n", __func__);
        }
    }
    else
    {
        g_log_buf = NULL;
    }
    g_log_buf_len = 0;
    g_log_last_write_sec = -1;
    printf("%s buf_size=%d, timeout=%d \n", __func__, g_log_buf_size,
        g_log_buf_timeout);
}

#else
EXPORT_SYMBOL void log_init()
{
    const char *log_path = stack_config_get_interface()->get_btstack_log_path();
    char last_log_path[PATH_MAX] = {0};

    if (false == stack_config_get_interface()->get_btstack_log2file_turned_on())
    {
        g_log_fd = INVALID_FD;
        printf("%d@%s\n", __LINE__, __func__);
        return;
    }

    if (true == stack_config_get_interface()->get_btstack_should_save_last())
    {
        snprintf(last_log_path, PATH_MAX, "%s.%s", log_path, log_timestamp(TIMESTAMP_TYPE_FILE_NAME));
        if (rename(log_path, last_log_path) && errno != ENOENT)
        {
            printf("%s unable to rename '%s' to '%s': %s\n", __func__, log_path, last_log_path, strerror(errno));
        }
    }
    printf("%s %d:%d@%s\n", log_path, g_log_fd, __LINE__, __func__);

    g_log_fd = open(log_path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (INVALID_FD == g_log_fd)
    {
        printf("%s unable to open '%s': %s\n", __func__, log_path, strerror(errno));
    }
    printf("%d@%s %d\n", __LINE__, __func__, g_log_fd);
}
#endif

EXPORT_SYMBOL void log_reg(void (*log_cb)(char *log_str))
{
    s_log_cb = log_cb;
}


static void log_check_for_new_file(int write_bytes)
{
    if (fg_log_slice != TRUE)
    {
        return;
    }

    if (stack_logging_pos + write_bytes >= stack_log_size)
    {
        int log_file_access = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;
        int log_file_mode = O_WRONLY | O_CREAT | O_TRUNC;
        char log_path_local1[PATH_MAX] = {0};
        char log_path_local[PATH_MAX] = {0};

        stack_file_index_in_slice++;
        stack_logging_pos = 0;

        fsync(g_log_fd);
        close(g_log_fd);

        /* if limit the number of the file, remove the old one */
        if (stack_log_max_file_num > 0)
        {
            sprintf(log_path_local1, "%s.%d", stack_log_file_path,
                stack_file_index_in_slice-stack_log_max_file_num);
            if(access(log_path_local1, F_OK) == 0)
            {
                int ret = 0;
                sprintf(log_path_local, "%s.%d", stack_log_file_path,
                    stack_file_index_in_slice);
                if ((ret = rename(log_path_local1, log_path_local)) && errno != ENOENT)
                {
                    printf("%s unable to rename '%s' to '%s': %s, ret=%d\n",
                        __func__, log_path_local1, log_path_local, strerror(errno), ret);
                }
                else
                {
                    printf("%s rename %s to %s success\n", __func__, log_path_local1, log_path_local);
                }
            }
            memset(log_path_local, 0, sizeof(log_path_local));
        }
        sprintf(log_path_local, "%s.%d", stack_log_file_path,
            stack_file_index_in_slice);

        g_log_fd = open(log_path_local, log_file_mode, log_file_access);
        if (g_log_fd == INVALID_FD)
        {
            g_log_fd = -1;
            printf("%s unable to open '%s': %s\n", __func__,
                log_path_local, strerror(errno));
            return;
        }
    }

    stack_logging_pos += write_bytes;

    return;
}


EXPORT_SYMBOL void log_write(const char *format, ...)
{
    va_list args;
    char msg[500] = {0};
    char msg_ts[500] = {0};

    struct timeval tv;
    gettimeofday(&tv, NULL);
    va_start(args, format);
    vsnprintf(msg, sizeof(msg), format, args);
    va_end(args);
    if (g_log_fd != INVALID_FD)
    {
        snprintf(msg_ts, sizeof(msg_ts), "%s %s", log_timestamp(TIMESTAMP_TYPE_LOG_PREFIX), msg);
        if (false == fg_only2file)
        {
            printf("%s", msg_ts);
        }
        BT_MW_LOG_LOCK();

        if (g_log_buf != NULL)
        {
            if ((g_log_buf_len + strlen(msg_ts) > g_log_buf_size)
                || ((tv.tv_sec > g_log_last_write_sec + g_log_buf_timeout) && (g_log_buf_len > 0)))
            {
                write(g_log_fd, g_log_buf, g_log_buf_len);
                log_check_for_new_file(g_log_buf_len);
                g_log_buf_len = 0;
                g_log_last_write_sec = tv.tv_sec;
            }

            memcpy(g_log_buf+g_log_buf_len, msg_ts, strlen(msg_ts));
            g_log_buf_len += strlen(msg_ts);
        }
        else
        {
            write(g_log_fd, msg_ts, strlen(msg_ts));
            log_check_for_new_file(strlen(msg_ts));
        }

        BT_MW_LOG_UNLOCK();
    }
    else
    {
        if (s_log_cb)
        {
            snprintf(msg_ts, sizeof(msg_ts), "%s %s", log_timestamp(TIMESTAMP_TYPE_LOG_PREFIX), msg);
            s_log_cb(msg_ts);
        }
        else
        {
            printf("%s %s", log_timestamp(TIMESTAMP_TYPE_LOG_PREFIX), msg);
        }
    }
}

EXPORT_SYMBOL void log_deinit()
{
    printf("%s %d\n", __func__, g_log_fd);
    if (g_log_fd != INVALID_FD)
    {
        close(g_log_fd);
        g_log_fd = INVALID_FD;
    }

    if (g_log_buf != NULL)
    {
        free(g_log_buf);
        g_log_buf = NULL;
    }
}

