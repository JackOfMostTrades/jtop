
#include <stdint.h>

struct glibtop_conntrack {
  uint64_t tcp_conns;
  uint64_t udp_conns;
};

void glibtop_get_conntrack(struct glibtop_conntrack *buf);

