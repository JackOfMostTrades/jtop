#include "netlist.h"
#include "netload.h"
#include "cpu.h"
#include "mem.h"
#include "swap.h"
#include "conntrack.h"
#include "wireless.h"
#include <stdio.h>

int main(int argc, char **argv) {
  int i;
  struct glibtop_netlist netlist;
  char **ifnames = glibtop_get_netlist(&netlist);
  for (i = 0; i < netlist.number; i++) {
    struct glibtop_netload netload;
    glibtop_get_netload(&netload, ifnames[i]);
    printf("%s\n", ifnames[i]);
    printf(" %lu, %lu\n", netload.bytes_in, netload.bytes_out);
  }

  struct glibtop_cpu cpu;
  glibtop_get_cpu(&cpu);
  printf("%lu, %lu, %lu\n", cpu.user, cpu.sys, cpu.nice);

  struct glibtop_mem mem;
  glibtop_get_mem(&mem);
  printf("%lu, %lu\n", mem.total, mem.used);

  struct glibtop_swap swap;
  glibtop_get_swap(&swap);
  printf("%lu, %lu\n", swap.total, swap.used);

  struct glibtop_conntrack conntrack;
  glibtop_get_conntrack(&conntrack);
  printf("%lu, %lu\n", conntrack.tcp_conns, conntrack.udp_conns);

  struct glibtop_wireless wireless;
  glibtop_get_wireless(&wireless, "wlan0");
  printf("%ld, %ld\n", wireless.signal_dbm, wireless.noise_dbm);

  return 0;
}
