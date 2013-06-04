
#include <stdint.h>

struct glibtop_swap {
  uint64_t total;
  uint64_t used;
  uint64_t free;
};

void glibtop_get_swap (struct glibtop_swap *buf);

