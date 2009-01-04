/* protocol dependent INET nat hook
 *
 */
#include <linux/module.h>

#include <linux/netfilter.h>
#include <net/netfilter/nf_nat.h>
#include <net/netfilter/nf_nat_helper.h>
#include <net/netfilter/nf_nat_rule.h>
#include <net/netfilter/nf_conntrack_expect.h>
#include <net/netfilter/nf_conntrack_helper.h>

#include "nf_conntrack_dummy.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chou Chifeng <cfchou@gmail.com>");
MODULE_DESCRIPTION("dummy NAT helper");
MODULE_ALIAS("ip_nat_dummy");

static int mangle_dummy(struct sk_buff *skb, __be32 ip, u_int16_t port);

/* [NOTE]
 * Comparing ip_nat_ftp.c and ip_nat_sip.c, the former uses 
 * ip_nat_follow_master() but the later uses its own ip_nat_sip_expected(). 
 * That's because what SIP expects, namely RTP, does not necessarily come from
 * any peer of the master conntrack. It does not "follow master".
 * Our dummy protocol is more like ftp. Hence we can just use
 * ip_nat_follow_master().
 */

// We have to setup its NAT on the fly. Mainly copy from the body of
// ip_nat_follow_master() and plus some silly printk. see [NOTE].
static void ip_nat_dummy_expected(struct nf_conn *ct,
	struct nf_conntrack_expect *exp)
{
	struct nf_nat_range range;
	BUG_ON(ct->status & IPS_NAT_DONE_MASK);
	
	printk(KERN_ALERT "[INFO] raw exp ORIGN: " NIPQUAD_FMT ":%d ->"
		NIPQUAD_FMT ":%d\n",
		NIPQUAD(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.all),
		ntohs(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.all),
		NIPQUAD(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u3.all),
		ntohs(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.all));
	printk(KERN_ALERT "[INFO] raw exp REPLY: " NIPQUAD_FMT ":%d ->"
		NIPQUAD_FMT ":%d\n",
		NIPQUAD(ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u3.all),
		ntohs(ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u.all),
		NIPQUAD(ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u3.all),
		ntohs(ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.all));

	// SNAT
	range.flags = IP_NAT_RANGE_MAP_IPS;
	range.min_ip = range.max_ip =
		ct->master->tuplehash[!exp->dir].tuple.dst.u3.ip;
	nf_nat_setup_info(ct, &range, IP_NAT_MANIP_SRC);

	printk(KERN_ALERT "[INFO] SNAT exp ORIGN: " NIPQUAD_FMT ":%d ->"
		NIPQUAD_FMT ":%d\n",
		NIPQUAD(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.all),
		ntohs(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.all),
		NIPQUAD(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u3.all),
		ntohs(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.all));
	printk(KERN_ALERT "[INFO] SNAT exp REPLY: " NIPQUAD_FMT ":%d ->"
		NIPQUAD_FMT ":%d\n",
		NIPQUAD(ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u3.all),
		ntohs(ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u.all),
		NIPQUAD(ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u3.all),
		ntohs(ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.all));

	// DNAT
	range.flags = IP_NAT_RANGE_MAP_IPS | IP_NAT_RANGE_PROTO_SPECIFIED;
	range.min = range.max = exp->saved_proto; // master's port
	range.min_ip = range.max_ip =
		ct->master->tuplehash[!exp->dir].tuple.src.u3.ip;
	nf_nat_setup_info(ct, &range, IP_NAT_MANIP_DST);

	printk(KERN_ALERT "[INFO] DNAT exp ORIGN: " NIPQUAD_FMT ":%d ->"
		NIPQUAD_FMT ":%d\n",
		NIPQUAD(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.all),
		ntohs(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.all),
		NIPQUAD(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u3.all),
		ntohs(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.all));
	printk(KERN_ALERT "[INFO] DNAT exp REPLY: " NIPQUAD_FMT ":%d ->"
		NIPQUAD_FMT ":%d\n",
		NIPQUAD(ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u3.all),
		ntohs(ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u.all),
		NIPQUAD(ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u3.all),
		ntohs(ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.all));
}

unsigned int ip_nat_dummy(struct sk_buff *skb, 
	enum ip_conntrack_info ctinfo,
	struct nf_conntrack_expect *exp)
{
	int dir = CTINFO2DIR(ctinfo);
	struct nf_conn *ct = exp->master;
	__be32 new_ip = ct->tuplehash[!dir].tuple.dst.u3.ip;
	u_int16_t new_port = 0;

	exp->dir = !dir;

	// It will be called once exepcted conntrack being established.
	// We can instead use ip_nat_follow_master. see [NOTE].
	exp->expectfn = ip_nat_dummy_expected;

	// Original expect, if no NAT.
	exp->saved_ip = exp->tuple.dst.u3.ip;
	exp->saved_proto.udp.port = exp->tuple.dst.u.udp.port;

	exp->tuple.dst.u3.ip = new_ip;
	// Find a free port
	for (new_port = ntohs(exp->saved_proto.udp.port); new_port != 0;
		++new_port) {
		exp->tuple.dst.u.udp.port = new_port;
		if (0 == nf_ct_expect_related(exp))
			break;
	}
	
	if (new_port == 0) // Can't find a free port
		return NF_DROP;

	if (new_ip == exp->saved_ip ||
		new_port == exp->saved_proto.udp.port) // If nothing changes
		return NF_ACCEPT;

	if (!mangle_dummy(skb, new_ip, new_port)) {
		nf_ct_unexpect_related(exp);
		return NF_DROP;
	}

	return NF_ACCEPT;
}

static int mangle_dummy(struct sk_buff *skb, __be32 ip, u_int16_t port)
{
	u_int8_t buf[6]; // octet[1-6]
	enum ip_conntrack_info ctinfo;
	struct nf_conn *ct = nf_ct_get(skb, &ctinfo);

	memcpy(buf, &ip, 4);
	memcpy(buf + 4, &port, 2);

	// The range begins from octet[1], span 6 octets. replace it with buf
	return nf_nat_mangle_udp_packet(skb, ct, ctinfo, 1, 6, buf,
		sizeof(buf));
}

static void __exit nf_nat_dummy_fini(void)
{
	rcu_assign_pointer(nf_nat_dummy_hook, NULL);
	synchronize_rcu();
}

static int __init nf_nat_dummy_init(void)
{
	BUG_ON(nf_nat_dummy_hook != NULL);
	rcu_assign_pointer(nf_nat_dummy_hook, ip_nat_dummy);
	return 0;
}

module_init(nf_nat_dummy_init);
module_exit(nf_nat_dummy_fini);
