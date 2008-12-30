/*
 *
 */
#include <netinet/in.h>
#include <arpa/inet.h>
#ifndef _DUMMY_H_CFCHOU_200812
#define _DUMMY_H_CFCHOU_200812

#define real_stringfy(x)	#x
#define stringfy(x)	real_stringfy(x)
// catenate strings
#define cats(a, b)		a b

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
#define GREETING_MAX	128

extern int get_if_addr(char const *ifname, struct sockaddr_in *paddr);
extern int bind_local(struct sockaddr_in *addr);
#endif /* _DUMMY_H_CFCHOU_200812 */
