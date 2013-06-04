
#include "common.h"
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

char* skip_token(char *p) {
  while (*p == ' ' && *p != '\0') p++;
  while (*p != ' ' && *p != '\0') p++;
  while (*p == ' ' && *p != '\0') p++;
  return p;
}
char* skip_multiple_token(char *p, const uint32_t n) {
  uint32_t i;
  for (i=0;i < n; i++) p = skip_token(p);
  return p;
}


void file_to_buffer(char* buffer, const uint32_t bufsize, const char *filename) {
  uint32_t nread = 0;
  uint32_t len = 0;

  int fd = open(filename, O_RDONLY);
  while (len < bufsize-1) {
    nread = read(fd, buffer+len, bufsize-len-1);
    len += nread;
    if (nread == 0) break;
  }
  close(fd);
  buffer[len] = '\0'; 
}

uint64_t get_scaled(const char *buffer, const char *key)
{
  const char    *ptr = buffer;
  char          *next;
  uint64_t value;

        if (key) {
                ptr = strstr(buffer, key);
                if (ptr != NULL)
                        ptr += strlen(key);
                else {
                        return 0;
                }
        }

        value = strtoull(ptr, &next, 0);

        for ( ; *next; ++next) {
                if (*next == 'k') {
                        value *= 1024;
                        break;
                } else if (*next == 'M') {
                        value *= 1024 * 1024;
                        break;
                }
        }

        return value;
}

size_t strlcpy(char *dst, const char *src, size_t size) {
  size_t n;
  for (n = 0; n+1 < size; n++) {
    dst[n] = src[n];
    if (src[n] == '\0') break;
  }
  dst[n] = '\0';
  return n;
}

