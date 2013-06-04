CFLAGS=-Wall -DNO_IPV6

all: libgtop.a

libgtop.a: common.c conntrack.c cpu.c mem.c netlist.c netload.c swap.c wireless.c
	gcc $(CFLAGS) -c common.c conntrack.c cpu.c mem.c netlist.c netload.c swap.c wireless.c
	ar r libgtop.a common.o conntrack.o cpu.o mem.o netlist.o netload.o swap.o wireless.o

clean:
	rm -f libgtop.a *.o

