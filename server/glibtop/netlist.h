
#include <stdint.h>

struct glibtop_netlist {
  uint32_t number;
};

char** glibtop_get_netlist(struct glibtop_netlist *buf);

