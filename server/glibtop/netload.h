
#include <stdint.h>

struct glibtop_netload {
  uint32_t has_addr;
  uint32_t is_loopback;
  uint32_t is_linklocal;
  uint64_t bytes_in;
  uint64_t bytes_out;
};

void glibtop_get_netload(struct glibtop_netload *buf, const char *interface);

