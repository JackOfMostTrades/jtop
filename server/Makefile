CFLAGS=-Wall -DNETLOAD_IFACE=\"br-wan\" -DIW_IFACE=\"wl0\"
LIBS=-lpthread

all: jtop.c glibtop/libgtop.a
	gcc $(CFLAGS) jtop.c -c
	gcc $(LIBS) jtop.o glibtop/libgtop.a -o jtop
	strip jtop

glibtop/libgtop.a:
	cd glibtop; make

mongoose.o:
	#wget 'https://github.com/valenok/mongoose/raw/master/mongoose.c'
	#wget 'https://github.com/valenok/mongoose/raw/master/mongoose.h'
	gcc $(CFLAGS) -c mongoose.c

clean:
	cd glibtop; make clean
	rm -f jtop *.o

