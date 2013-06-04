
#include <stdint.h>

struct glibtop_cpu {
  uint64_t user;
  uint64_t nice;
  uint64_t sys;
  uint64_t idle;
  uint64_t total;
};

void glibtop_get_cpu(struct glibtop_cpu *buf);

