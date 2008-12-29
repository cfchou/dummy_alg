/*
 *
 */

#define vvv_perror()							\
do {									\
	char buf[128];							\
	snprintf(buf, sizeof(buf), "%s: %s> ",				\
		cats(cats(cats(__FILE__, "("), stringfy(__LINE__)),	\
			"): "),						\
		__func__);						\
	perror(buf);							\
} while (0)
#define DUMMY_PORT	2008

#define print_usage()								\
do {										\
	fprintf(stderr, "Usage: %s -i ifname -p local_port -r remote_ip\n",	\
		argv[0]);							\
while (0)

static char const * const greeting = "Howdy!"
#define GREETING_MAX	128

int main(int argc , char const *argv[])
{
	int opt = 0;
	int listen_sock = 0;
	int sock = 0;
	int cli_sock = 0;
	struct sockaddr_in local_addr, raddr, raddr_greeting;
	socklen_t len = sizeof(raddr);
	uint8_t buf[GREETING_MAX];

	memset(&local_addr, 0, sizeof(local_addr));
	memset(&radd, 0, sizeof(raddr));
	while (-1 != (opt = getopt(argc, argv, "i:p:r:"))) {
		switch (opt) {
		// local options
		case 'i':
			if (0 != get_if_addr(optarg, &local_addr)) {
				print_usage();
				return -1
			}
			if (AF_INET != local_addr.sin_family) {
				print_usage();
				return -1
			}
			break;
		case 'p':
			local_addr.sin_port = htos(atoi(optarg));
			break;
		// remote option
		case 'r':
			if (1 != inet_pton(AF_INET, optarg,
				(struct sockaddr *)&raddr)) {
				fprintf(stderr, "[ERR] wrong remote_ip: %s\n",
					optarg);
				print_usage();
				return -1;
			}
			raddr.sin_port = htons(DUMMY_PORT);
			break;
		default: /* '?' */
			print_usage();
			return 0;
		}
	}

	if (0 == local_addr.sin_port || AF_INET == local_addr.sin_family ||
		AF_INET != raddr.sin_family) {
		fprintf(stderr, "[ERR] wrong local_port or remote_ip\n");
		print_usage();
		return -1;
	}
	//
	memset(buf, 0, sizeof(buf));
	if (0 >= (buf_sz = prepare_greeting(&local_addr, buf, sizeof(buf)))) {
		return -1;
	}
	//
	if (0 != (listen_sock = listen_local(&listen_addr))) {
		return -1;
	}
	//
	if (-1 == (sock = socket(AF_INET, SOCK_DGRAM, 0))) {
		vvv_perror();
		close(listen_sock);
		return -1;
	}
	if (buf_sz != sendto(sock, buf, buf_sz, 0, &raddr, sizeof(raddr)))
	{
		vvv_perror();
		close(sock);
		close(listen_sock);
		return -1;
	}
	//
	memset(&radd_greeting, 0, sizeof(raddr_greeting));
	if (-1 == (cli_sock = accept(listen_sock, &raddr_greeting, &len))) {
		vvv_perror();
		close(listen_sock);
		return -1;
	}

	
	return 0;
}

int get_if_addr(char const *ifname, struct sockaddr_in *paddr)
{
	int sock = 0;
	struct ifreq ifr;
	assert(ifname && *ifname && paddr);
	if (-1 == (sock = socket(AF_INET, SOCK_DGRAM, 0))) {
		vvv_perror();
		return -1;
	}
	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_addr.sa_family = AF_INET;
	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", ifname);
	if (0 != ioctl(sock, SIOCGIFADDR, &ifr)) {
		vvv_perror();
		close(sock);
		return -1;
	}
	close(sock);
	memcpy(paddr, &ifr.ifr_addr, sizeof(struct sockaddr_in));
	return 0;
}

ssize_t prepare_greeting(struct sockaddr_in *addr, uint8_t *buf, size_t len)
{
	int greeting_len = strlen(greeting);
	if ((uint8_t)(-1) < greeting_len || len < greeting_len + 8)
		return -1;
	buf[0] = 0;
//################
	buf[1-4]
	buf[5-6]
	buf[7] = greeting_len;

}

int listen_local(struct sockaddr_in *addr)
{
	int listen_sock = 0;
	int const reuse = 1;

	if (-1 == (listen_sock = socket(PF_INET, SOCK_DGRAM, 0))) {
		vvv_perror();
		return -1;
	}
	if (0 != setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &reuse,
		sizeof(reuse))) {
		vvv_perror();
		close(listen_sock);
		return -1;
	}
	if (0 != bind(listen_sock, (struct sockaddr *)addr, sizeof(*addr))) {
		vvv_perror();
		close(listen_sock);
		return -1;
	}
	if (0 != listen(listen_sock, SOMAXCONN)) {
		vvv_perror();
		close(listen_sock);
		return -1;
	}
	return listen_sock;
}
