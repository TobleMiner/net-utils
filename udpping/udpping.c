#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>

#define PING_INTERVAL 200
#define PORT 31337
#define SIZE 16

bool do_exit = false;

void show_usage(char* bin_name)
{
	fprintf(stderr, "USAGE: %s [-c IP-ADDRESS] [-p PORT] [-i TIMEOUT] [-s PACKET_SIZE]\n", bin_name);
	fprintf(stderr, "-c: Client mode, IP-ADDRESS is address of device running %s in server mode\n", bin_name);
}

void doshutdown(int signal)
{
	do_exit = true;
}

int main(int argc, char** argv)
{
	struct sockaddr_in server_addr, client_addr, nat_addr;
	int sock, err = 0, interval = PING_INTERVAL;
	short port = PORT;
	struct timeval timeout;
	size_t packetsize = SIZE;
	unsigned char* buffer;
	unsigned long seq = 0, loss_cnt = 0, seq_net, seq_recv;
	bool client = false;
	ssize_t len;
	char opt;
	socklen_t slen;

	while((opt = getopt(argc, argv, "c:p:i:s:h")) != -1)
	{
		switch(opt)
		{
			case 'c':
				client = true;
				if(inet_pton(AF_INET, optarg, &(client_addr.sin_addr.s_addr)) != 1)
				{
					fprintf(stderr, "Invalid ip address\n");
					show_usage(argv[0]);
					err = -EINVAL;
					goto exit_err;
				}
				break;
			case 'p':
				port = atoi(optarg);
				break;
			case 'i':
				interval = atoi(optarg);
				break;
			case 's':
				packetsize = atoi(optarg);
				break;
			case 'h':
				show_usage(argv[0]);
				goto exit_err;
			default:
				fprintf(stderr, "Invalid parameter %c\n", opt);
				show_usage(argv[0]);
				err = -EINVAL;
				goto exit_err;
		}
	}

	// Init packet buffer
	buffer = malloc(packetsize);
	if(!buffer)
	{
		fprintf(stderr, "Failed to allocate packet buffer\n");
		err = -ENOMEM;
		goto exit_err;
	}
	memset(buffer, 0x55, packetsize);

	// Setup server socket
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(port);
	if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		fprintf(stderr, "Failed to set up server socket, %s(%d)\n", strerror(errno), errno);
		err = -errno;
		goto exit_buffer;
	}
	if(bind(sock, (struct sockaddr*) &server_addr, sizeof(server_addr)))
	{
		fprintf(stderr, "Failed to bind server socket, %s(%d)\n", strerror(errno), errno);
		err = -errno;
		goto exit_sock;
	}

	timeout.tv_sec = 0;
	timeout.tv_usec = interval * 1000;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

	// Setup client address
	client_addr.sin_port = htons(port);
	client_addr.sin_family = AF_INET;

	// Set signal handler
	if(signal(SIGINT, doshutdown))
	{
		fprintf(stderr, "Failed to bind SIGINT\n");
		err = errno;
		goto exit_sock;
	}

	while(!do_exit)
	{
		if(client)
		{
			seq_net = htonl(seq);
			if(packetsize < sizeof(seq_net))
			{
				fprintf(stderr, "Packet size %zd < %zd, packet too short\n", len, sizeof(seq_net));
				err = -EINVAL;
				goto exit_sock;
			}
			memcpy(buffer, &seq_net, sizeof(seq_net));
			printf("Sending to %s:%u, buffer size=%zd\n", inet_ntoa((struct in_addr)client_addr.sin_addr), ntohs(client_addr.sin_port), packetsize);
			if((len = sendto(sock, buffer, packetsize, 0, (struct sockaddr*) &client_addr, sizeof(client_addr))) < packetsize)
			{
				if(len < 0)
				{
					fprintf(stderr, "Failed to send data, %s(%d)\n", strerror(errno), errno);
					err = -errno;
					goto exit_sock;
				}
				printf("Only %zd of %zd bytes sent\n", len, packetsize);
			}
recv_again:
			if((len = recvfrom(sock, buffer, packetsize, 0, NULL, NULL)) < 0)
			{
				if(errno == EAGAIN)
				{
					printf("Packet %lu lost\n", seq);
					loss_cnt++;
				}
				printf("Failed to recv data, %s(%d)\n", strerror(errno), errno);
			}
			else
			{
				if(len < sizeof(seq_net))
				{
					printf("Packet length %zd < %zd, packet too short\n", len, sizeof(seq_net));
					goto recv_again;
				}
				memcpy(&seq_net, buffer, sizeof(seq_net));
				seq_recv = ntohl(seq_net);
				if(seq_recv != seq)
				{
					printf("Received data for unexpected seq num %lu\n", seq);
					goto recv_again;
				}
				printf("Received data for seq=%lu\n", seq_recv);
			}
			seq++;
		}
		else
		{
			if((len = recvfrom(sock, buffer, packetsize, 0, (struct sockaddr*) &nat_addr, &slen)) < 0)
			{
				if(errno != EAGAIN)
					printf("Failed to recv data, %s(%d)\n", strerror(errno), errno);
			}
			else
			{
				memcpy(&seq_net, buffer, sizeof(seq_net));
				seq_recv = ntohl(seq_net);
				printf("Received data for seq=%lu, port=%u\n", seq_recv, ntohs(nat_addr.sin_port));
				if((len = sendto(sock, buffer, packetsize, 0, (struct sockaddr*) &nat_addr, slen)) < packetsize)
				{
					if(len < 0)
					{
						printf("Failed to send data, %s(%d)\n", strerror(errno), errno);
						err = -errno;
						goto exit_sock;
					}
					printf("Only %zd of %zd bytes sent\n", len, packetsize);
				}
			}
		}
	}

	if(client)
	{
		// Print statistics
		printf("Packets sent: %lu\n", seq);
		printf("Packets lost: %lu\n", loss_cnt);
		printf("Loss percentage: %.5f%%\n", (double)loss_cnt / seq * 100);
	}

exit_sock:
	close(sock);
exit_buffer:
	free(buffer);
exit_err:
	return err;
}
