
#include <stdint.h>

struct glibtop_wireless {
  int64_t signal_dbm;
  int64_t noise_dbm;
};

void glibtop_get_wireless(struct glibtop_wireless *buf, const char *ifname);

