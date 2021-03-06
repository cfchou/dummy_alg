/* sdummy.c
 * send & wait for a greeting
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

#define print_usage()								\
do {										\
	fprintf(stderr, "Usage: %s -i ifname -p local_port -r remote_ip\n",	\
		argv[0]);							\
} while (0)

static char const * const greeting = "Howdy!";
static ssize_t compose_greeting(struct sockaddr_in *addr, uint8_t *buf,
	size_t len);

int main(int argc , char * const argv[])
{
	int opt = 0;
	int bind_sock = 0;
	int sock = 0;
	struct sockaddr_in local_addr, raddr;
	uint8_t buf[GREETING_MAX + 1];
	ssize_t buf_sz = 0;

	memset(&local_addr, 0, sizeof(local_addr));
	memset(&raddr, 0, sizeof(raddr));
	while (-1 != (opt = getopt(argc, argv, "i:p:r:"))) {
		switch (opt) {
		// local options
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
		case 'p':
			local_addr.sin_port = htons(atoi(optarg));
			break;
		// remote option
		case 'r':
			if (1 != inet_pton(AF_INET, optarg, &raddr.sin_addr)) {
				fprintf(stderr, "[ERR] wrong remote_ip: %s\n",
					optarg);
				print_usage();
				return -1;
			}
			raddr.sin_port = htons(DUMMY_PORT);
			raddr.sin_family = AF_INET;
			break;
		default: /* '?' */
			print_usage();
			return 0;
		}
	}

	if (0 == local_addr.sin_port || AF_INET != local_addr.sin_family ||
		AF_INET != raddr.sin_family) {
		fprintf(stderr, "[ERR] wrong local_port or remote_ip\n");
		print_usage();
		return -1;
	}
	// compose a sincere greeting
	memset(buf, 0, sizeof(buf));
	if (0 >= (buf_sz = compose_greeting(&local_addr, buf, sizeof(buf)))) {
		return -1;
	}

	// according to our "dummy protocol", wait for a greeting back, which
	// is gonna be "expected" by alg
	if (-1 == (bind_sock = bind_local(&local_addr))) {
		return -1;
	}

	// send our greeting
	if (-1 == (sock = socket(AF_INET, SOCK_DGRAM, 0))) {
		vvv_perror();
		close(bind_sock);
		return -1;
	}
	if (buf_sz != sendto(sock, buf, buf_sz, 0, (struct sockaddr *)&raddr,
		sizeof(raddr))) {
		vvv_perror();
		close(sock);
		close(bind_sock);
		return -1;
	}
	close(sock);

	// receive greeting back from remote
	memset(buf, 0, sizeof(buf));
	if (-1 == (buf_sz = recvfrom(bind_sock, buf, sizeof(buf) - 1, 0,
		NULL, 0))) {
		vvv_perror();
		close(bind_sock);
		return -1;
	}
	
	fprintf(stdout, "[S] Greeting from remote(%u bytes): \"%s\"\n", buf_sz, buf);
	close(bind_sock);
	return 0;
}

static ssize_t compose_greeting(struct sockaddr_in *addr, uint8_t *buf,
	size_t buf_sz)
{
	int greeting_len = strlen(greeting);
	if ((uint8_t)(-1) < greeting_len || buf_sz < greeting_len + 8) {
		return -1;
	}

	buf[0] = 0;
	// network-endian anyway
	memcpy(buf + 1, &addr->sin_addr.s_addr, 4);
	memcpy(buf + 5, &addr->sin_port, 2);
	buf[7] = greeting_len;
	memcpy(buf + 8, greeting, greeting_len); 
	return greeting_len + 8;
}

