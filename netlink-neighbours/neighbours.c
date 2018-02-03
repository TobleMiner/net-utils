#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/socket.h>

#include <netlink/socket.h>
#include <netlink/route/link.h>
#include <netlink/route/neighbour.h>
#include <netlink/route/neightbl.h>
#include <netlink/cache.h>
#include <netlink/list.h>
#include <netlink/object.h>

int main(int argc, char** argv) {
	int err = 0;

	struct nl_sock* sock = nl_socket_alloc();
	if(!sock) {
		errno = -ENOMEM;
		fprintf(stderr, "Failed to create socket\n");
		goto fail;
	}

	printf("Connecting to netlink...\n");
	if((err = nl_connect(sock, NETLINK_ROUTE))) {
		fprintf(stderr, "Failed to connecto to netlink: %s (%d)\n", strerror(-err), err);
		goto fail_socket;
	}
	printf("Connection established\n");

	struct nl_cache* link_cache;
	if((err = rtnl_link_alloc_cache(sock, AF_UNSPEC, &link_cache)) < 0) {
		fprintf(stderr, "Failed to allocate link cache: %s (%d)\n", strerror(-err), err);
		goto fail_socket;
	}

	const char* iface = "wlp4s0";
	int ifindex;
	if((ifindex = rtnl_link_name2i(link_cache, iface)) < 0) {
		err = ifindex;
		fprintf(stderr, "Failed to get index of interface '%s': %s (%d)\n", iface, strerror(-err), err);
		goto fail_link_cache;
	}

	if(!ifindex) {
		err = -ENOENT;
		fprintf(stderr, "Interface '%s' has no ifindex assigned\n", iface);
		goto fail_link_cache;
	}

	struct nl_cache* cache;

	if(rtnl_neightbl_alloc_cache(sock, &cache) != 0) {
		fprintf(stderr, "Failed to allocate neighbour table cache\n");
		goto fail_link_cache;
	}

	struct rtnl_neightbl *tbl = rtnl_neightbl_get(cache, "ndisc_cache", ifindex);
	if(!tbl) {
		fprintf(stderr, "Failed to get neighbour table\n");
		goto fail_cache;
	}

	printf("Got neighbour table\n");

	char attrs[4096];

	nl_object_attr_list((struct nl_object*)tbl, attrs, sizeof(attrs));

	printf("nbtbl attrs: %s\n", attrs);

	rtnl_neightbl_put(tbl);


fail_cache:
	nl_cache_free(cache);
fail_link_cache:
	nl_cache_free(link_cache);
fail_socket:
	nl_socket_free(sock);
fail:
	return err;
}
