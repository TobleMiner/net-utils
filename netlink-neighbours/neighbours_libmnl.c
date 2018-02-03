#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>

#include <libmnl/libmnl.h>
#include <linux/if.h>
#include <linux/if_link.h>
#include <linux/rtnetlink.h>

static int data_ipv6_attr_cb(const struct nlattr *attr, void *data) {
	int type = mnl_attr_get_type(attr);
	if(mnl_attr_type_valid(attr, NDA_MAX) < 0) {
		printf("Unsupported attribute type %d\n", type);
		return MNL_CB_OK;
	}

//	printf("Attr type %d\n", type);

	if(type == NDA_DST) {
		struct in6_addr* addr = mnl_attr_get_payload(attr);
		static char buf[INET6_ADDRSTRLEN];
		printf("%s ", inet_ntop(AF_INET6, &addr->s6_addr, buf, sizeof(buf)));
	}

	if(type == NDA_LLADDR) {
		unsigned char* mac = mnl_attr_get_payload(attr);
		printf("lladdr %02x:%02x:%02x:%02x:%02x:%02x ", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	}

	if(type == NDA_CACHEINFO) {
		struct nda_cacheinfo* info = mnl_attr_get_payload(attr);
//		printf("info: %x\n", info->ndm_confirmed);
	}

/*	if(type == RTA_SRC) {
		struct in6_addr* addr = mnl_attr_get_payload(attr);
		static char buf[INET6_ADDRSTRLEN];
		printf("src address: %s\n", inet_ntop(AF_INET6, &addr->s6_addr, buf, sizeof(buf)));
	}
*/
	return MNL_CB_OK;
}

static int data_cb(const struct nlmsghdr *nlh, void *data)
{
	struct ndmsg* ndmsg = mnl_nlmsg_get_payload(nlh);
	if(ndmsg->ndm_state != NUD_REACHABLE) {
		return MNL_CB_OK;
	}
	printf("Interface %d, state %x\n", ndmsg->ndm_ifindex, ndmsg->ndm_state);
	mnl_attr_parse(nlh, sizeof(*ndmsg), data_ipv6_attr_cb, NULL);
	printf("\n");
	return MNL_CB_OK;
}

int main(int argc, char *argv[])
{
	struct mnl_socket *nl;
	char buf[MNL_SOCKET_BUFFER_SIZE];
	struct nlmsghdr *nlh;
	struct ndmsg *ndm;
	int ret;
	unsigned int seq, portid;

	nlh = mnl_nlmsg_put_header(buf);
	nlh->nlmsg_type = RTM_GETNEIGH;
	nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
	nlh->nlmsg_seq = seq = time(NULL);
	ndm = mnl_nlmsg_put_extra_header(nlh, sizeof(struct ndmsg));

	ndm->ndm_family = AF_INET6;

	nl = mnl_socket_open(NETLINK_ROUTE);
	if (nl == NULL) {
		perror("mnl_socket_open");
		exit(EXIT_FAILURE);
	}

	if (mnl_socket_bind(nl, 0, MNL_SOCKET_AUTOPID) < 0) {
		perror("mnl_socket_bind");
		exit(EXIT_FAILURE);
	}
	portid = mnl_socket_get_portid(nl);

	if (mnl_socket_sendto(nl, nlh, nlh->nlmsg_len) < 0) {
		perror("mnl_socket_sendto");
		exit(EXIT_FAILURE);
	}

	ret = mnl_socket_recvfrom(nl, buf, sizeof(buf));
	while (ret > 0) {
		ret = mnl_cb_run(buf, ret, seq, portid, data_cb, NULL);
		if (ret <= MNL_CB_STOP)
			break;
		ret = mnl_socket_recvfrom(nl, buf, sizeof(buf));
	}
	if (ret == -1) {
		perror("error");
		exit(EXIT_FAILURE);
	}

	mnl_socket_close(nl);

	return 0;
}

