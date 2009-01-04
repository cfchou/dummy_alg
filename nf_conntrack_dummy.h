/*
 *
 */
#ifndef __NF_CONNTRACK_DUMMY_H__
#define __NF_CONNTRACK_DUMMY_H__
#include <linux/skbuff.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_expect.h>

extern unsigned int (*nf_nat_dummy_hook)(struct sk_buff *skb, 
	enum ip_conntrack_info ctinfo,
	struct nf_conntrack_expect *exp);

#endif /* __NF_CONNTRACK_DUMMY_H__ */
