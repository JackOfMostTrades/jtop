
#include <stdint.h>

struct glibtop_mem {
  uint64_t total;
  uint64_t used;
  uint64_t free;
};

void glibtop_get_mem (struct glibtop_mem *buf);

