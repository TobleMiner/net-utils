CFLAGS_LIBNL=`pkg-config --cflags --libs libnl-3.0 libnl-route-3.0`
CFLAGS_LIBMNL=`pkg-config --cflags --libs libmnl`
CFLAGS=-Wall -ggdb -O0

all: clean neighbours_libmnl

neighbours:
	$(CC) ${CFLAGS} ${CFLAGS_LIBNL} neighbours.c -o showneighbours

neighbours_rtnl:
	$(CC) ${CFLAGS} neighbours_rtnl.c -o showneighbours

neighbours_libmnl:
	$(CC) ${CFLAGS} ${CFLAGS_LIBMNL} neighbours_libmnl.c -o showneighbours

clean:
	$(RM) showneighbours
