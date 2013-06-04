
#include "common.h"
#include "conntrack.h"
#include <stdio.h>

void glibtop_get_conntrack(struct glibtop_conntrack *buf) {
  char buffer[4096];
  buf->tcp_conns = 0;
  buf->udp_conns = 0;
  FILE *f = fopen("/proc/net/ip_conntrack", "r");
  if (f == NULL) {
    return;
  }
  while (fgets(buffer, sizeof buffer, f) != NULL) {
    int tcp = 0;
    int udp = 0;
    char *p = buffer;
    char *q;
    while (*p != ' ' && *p != '\0') p++;
    *p++ = '\0';
    if (strcmp(buffer, "tcp") == 0) {
      tcp = 1;
    } else if (strcmp(buffer, "udp") == 0) {
      udp = 1;
    } else {
      continue;
    }
    p = skip_multiple_token(p, 2);
    q = p;
    while (*p != ' ' && *p != '\0') p++;
    *p++ = '\0';
    if (strcmp(q, "TIME_WAIT") == 0) {
      continue;
    }

    if (tcp) {
      buf->tcp_conns += 1;
    } else if (udp) {
      buf->udp_conns += 1;
    }
  }
  fclose(f);
}

