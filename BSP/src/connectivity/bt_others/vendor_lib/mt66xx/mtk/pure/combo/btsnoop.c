/******************************************************************************
 *
 *  Copyright (C) 2014 Google, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#define LOG_TAG "btpure_snoop"

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

// Epoch in microseconds since 01/01/0000.
static const uint64_t BTSNOOP_EPOCH_DELTA = 0x00dcddb30f2f8000ULL;
#define INVALID_FD (-1)
static int logfile_fd = INVALID_FD;
static bool is_logging;

static void pure_btsnoop_write_packet(char type, const uint8_t *packet, bool is_received);

// Interface functions

EXPORT_SYMBOL void pure_btsnoop_enable(bool value, const char *log_path) {
  is_logging = value;
  if (value) {
    mode_t prevmask = umask(0);
    logfile_fd = open(log_path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (logfile_fd == INVALID_FD) {
      printf(LOG_TAG, "%s unable to open '%s': %s", __func__, log_path, strerror(errno));
      is_logging = false;
      umask(prevmask);
      return;
    }
    umask(prevmask);

    write(logfile_fd, "btsnoop\0\0\0\0\1\0\0\x3\xea", 16);
  } else {
    if (logfile_fd != INVALID_FD)
      close(logfile_fd);

    logfile_fd = INVALID_FD;
  }
}

EXPORT_SYMBOL void pure_btsnoop_capture(const char *buffer, bool is_received) {
  const char type = buffer[0];
  const uint8_t *p = &buffer[1];

  if (logfile_fd == INVALID_FD)
    return;

  switch (type) {
    case 0x04:
      pure_btsnoop_write_packet(0x04, p, false);
      break;
    case 0x01:
      pure_btsnoop_write_packet(0x01, p, true);
      break;
  }
}

// Internal functions

static uint64_t pure_btsnoop_timestamp(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);

  // Timestamp is in microseconds.
#if defined(MTK_COMMON) && (MTK_COMMON == TRUE)
  // fix incorrect format issue
  uint64_t timestamp = tv.tv_sec * 1000000ULL;
#else
  uint64_t timestamp = tv.tv_sec * 1000 * 1000LL;
#endif
  timestamp += tv.tv_usec;
  timestamp += BTSNOOP_EPOCH_DELTA;

  return timestamp;
}

static void pure_btsnoop_write(const void *data, size_t length) {
  if (logfile_fd != INVALID_FD)
    write(logfile_fd, data, length);
}

static void pure_btsnoop_write_packet(char type, const uint8_t *packet, bool is_received) {
  int length_he = 0;
  int length;
  int flags;
  int drops = 0;
  switch (type) {
    case 0x01:
      length_he = packet[2] + 4;
      flags = 2;
      break;
    case 0x02:
      length_he = (packet[3] << 8) + packet[2] + 5;
      flags = is_received;
      break;
    case 0x03:
      length_he = packet[2] + 4;
      flags = is_received;
      break;
    case 0x04:
      length_he = packet[1] + 3;
      flags = 3;
      break;
  }

  uint64_t timestamp = pure_btsnoop_timestamp();
  uint32_t time_hi = timestamp >> 32;
  uint32_t time_lo = timestamp & 0xFFFFFFFF;

  length = htonl(length_he);
  flags = htonl(flags);
  drops = htonl(drops);
  time_hi = htonl(time_hi);
  time_lo = htonl(time_lo);

  pure_btsnoop_write(&length, 4);
  pure_btsnoop_write(&length, 4);
  pure_btsnoop_write(&flags, 4);
  pure_btsnoop_write(&drops, 4);
  pure_btsnoop_write(&time_hi, 4);
  pure_btsnoop_write(&time_lo, 4);
  pure_btsnoop_write(&type, 1);
  pure_btsnoop_write(packet, length_he - 1);
}

