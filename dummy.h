/*
 *
 */
#ifndef _DUMMY_H_CFCHOU_200812
#define _DUMMY_H_CFCHOU_200812

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

#endif /* _DUMMY_H_CFCHOU_200812 */
