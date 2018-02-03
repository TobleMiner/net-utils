#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <linux/rtnetlink.h>

int main(int argc, char** argv) {
	struct {
		struct nlmsghdr n;
		struct ndmsg r;
	} req;

	struct rtattr *rta;
	int status;
	char buf[16384];
	struct nlmsghdr *nlmp;
	struct ndmsg *rtmp;
	struct rtattr *rtatp;
	int rtattrlen;
	struct in6_addr *inp;

	char lladdr[6];
	char ipv6string[INET6_ADDRSTRLEN];

	int fd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);

	memset(&req, 0, sizeof(req));
	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ndmsg));
	req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
	req.n.nlmsg_type = RTM_GETNEIGH;


	req.r.ndm_family = AF_INET6;
//	req.r.ndm_state = NUD_REACHABLE | NUD_PERMANENT | NUD_NONE;
//	req.r.ndm_flags = NTF_ROUTER | NTF_USE;
/*	req.r.ndm_ifindex = 1;
*/
//	rta = (struct rtattr *)(((char *)&req) + NLMSG_ALIGN(req.n.nlmsg_len));
//	rta->rta_len = RTA_LENGTH(req.n.nlmsg_len);

	status = send(fd, &req, req.n.nlmsg_len, 0);

	if (status < 0) {
		//error sending
	}

	status = recv(fd, buf, sizeof(buf), 0);

	if (status < 0) {
		//error receiving
	}

	for(nlmp = (struct nlmsghdr *)buf; status > sizeof(*nlmp);){

		int len = nlmp->nlmsg_len;
		int req_len = len - sizeof(*nlmp);

		if (req_len<0 || len>status || !NLMSG_OK(nlmp, status)) {
			printf("error");
		}

		rtmp = (struct ndmsg *)NLMSG_DATA(nlmp);
		rtatp = (struct rtattr *)IFA_RTA(rtmp);

		rtattrlen = IFA_PAYLOAD(nlmp);

		for (; RTA_OK(rtatp, rtattrlen); rtatp = RTA_NEXT(rtatp, rtattrlen)) {
			if(rtatp->rta_type == NDA_DST) {
				inp = (struct in6_addr *)RTA_DATA(rtatp);
				inet_ntop(AF_INET6, inp, ipv6string, INET6_ADDRSTRLEN);
				printf("addr: %s\n",ipv6string);
			}
		}

		status -= NLMSG_ALIGN(len);
		nlmp = (struct nlmsghdr*)((char*)nlmp + NLMSG_ALIGN(len));
	}

	return 0;
}
