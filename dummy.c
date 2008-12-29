/* dummy.c
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

#include "dummy.h"

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
	return listen_sock;
}
