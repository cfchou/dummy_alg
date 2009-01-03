/* rdummy.c
 * receive & send a greeting
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "dummy.h"

#define print_usage()					\
do {							\
	fprintf(stderr, "Usage: %s -i ifname\n",	\
		argv[0]);				\
} while (0)

static ssize_t compose_greeting_back(uint8_t const *recv_buf,
	size_t recv_buf_sz, struct sockaddr_in *addr, uint8_t *buf,
	size_t buf_sz);

int main(int argc , char * const argv[])
{
	int opt = 0;
	int bind_sock = 0;
	int sock = 0;
	struct sockaddr_in local_addr, raddr;
	uint8_t buf[GREETING_MAX + 1];
	ssize_t buf_sz = 0;
	uint8_t buf_back[GREETING_MAX + 1];
	ssize_t buf_back_sz = 0;

	memset(&local_addr, 0, sizeof(local_addr));
	memset(&raddr, 0, sizeof(raddr));
	while (-1 != (opt = getopt(argc, argv, "i:"))) {
		switch (opt) {
		case 'i':
			if (0 != get_if_addr(optarg, &local_addr)) {
				print_usage();
				return -1;
			}
			if (AF_INET != local_addr.sin_family) {
				print_usage();
				return -1;
			}
			break;
		default: /* '?' */
			print_usage();
			return 0;
		}
	}

	if (AF_INET != local_addr.sin_family) {
		print_usage();
		return -1;
	}
	local_addr.sin_port = htons(DUMMY_PORT);
	if (-1 == (bind_sock = bind_local(&local_addr))) {
		return -1;
	}
	memset(buf, 0, sizeof(buf));
	if (-1 == (buf_sz = recvfrom(bind_sock, buf, sizeof(buf) - 1, 0,
		NULL, 0))) {
		vvv_perror();
		close(bind_sock);
		return -1;
	}
	fprintf(stdout, "[R] Greeting from remote: %u bytes\n", buf_sz);
	close(bind_sock);


	if (-1 == (buf_back_sz = compose_greeting_back(buf, buf_sz, &raddr,
		buf_back, sizeof(buf_back)))) {
		return -1;
	}

	// send our greeting back
	if (-1 == (sock = socket(AF_INET, SOCK_DGRAM, 0))) {
		vvv_perror();
		return -1;
	}
	if (buf_back_sz != sendto(sock, buf_back, buf_back_sz, 0,
		(struct sockaddr *)&raddr, sizeof(raddr))) {
		vvv_perror();
		close(sock);
		return -1;
	}
	close(sock);
	return 0;
}

static ssize_t compose_greeting_back(uint8_t const *recv_buf,
	size_t recv_buf_sz, struct sockaddr_in *addr, uint8_t *buf,
	size_t buf_sz)
{
	if (recv_buf_sz < DUMMY_HDR_LEN || recv_buf_sz < recv_buf[7] + DUMMY_HDR_LEN ||
		buf_sz < recv_buf[7] + DUMMY_HDR_LEN) {
		return -1;
	}

	addr->sin_family = AF_INET;
	// network-endian
	memcpy(&addr->sin_addr.s_addr, recv_buf + 1, 4);
	memcpy(&addr->sin_port, recv_buf + 5, 2);
	memcpy(buf, recv_buf + DUMMY_HDR_LEN, recv_buf[7]);
	return recv_buf[7];
}

