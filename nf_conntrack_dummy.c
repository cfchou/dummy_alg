/*
 *
 */

/*
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/netfilter.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/ctype.h>
#include <linux/inet.h>

#include <linux/skbuff.h>
#include <linux/in.h>
*/

#include <linux/module.h>
#include <linux/ctype.h>
#include <linux/udp.h>
#include <linux/netfilter.h>

#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_expect.h>
#include <net/netfilter/nf_conntrack_helper.h>

#include "nf_conntrack_dummy.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chou Chifeng <cfchou@gmail.com>");
MODULE_DESCRIPTION("dummy connection tracking helper");
MODULE_ALIAS("ip_conntrack_dummy");

#define DUMMY_PORT		2008
#define MAX_PORTS		8
#define DUMMY_TIMEOUT		300
#define EXPECTED_TIMEOUT	30

#define DUMMY_HDR_LEN		8

static unsigned short ports[MAX_PORTS];
static unsigned int ports_c;
module_param_array(ports, ushort, &ports_c, 0400);
MODULE_PARM_DESC(ports, "port numbers of dummy servers");


static unsigned int dummy_timeout __read_mostly = DUMMY_TIMEOUT;
module_param(dummy_timeout, uint, 0600);
MODULE_PARM_DESC(dummy_timeout, "timeout for the master dummy session");

static const struct nf_conntrack_expect_policy dummy_exp_policy = {
	.max_expected = 1,
	.timeout = EXPECTED_TIMEOUT
};

unsigned int (*nf_nat_dummy_hook)(struct sk_buff *skb, 
	enum ip_conntrack_info ctinfo,
	struct nf_conntrack_expect *exp);
EXPORT_SYMBOL_GPL(nf_nat_dummy_hook);


static int dummy_help(struct sk_buff *skb,
		    unsigned int protoff,
		    struct nf_conn *ct,
		    enum ip_conntrack_info ctinfo)
{
	int ret = NF_ACCEPT;
	int dir = CTINFO2DIR(ctinfo);
	unsigned int headlen = 0;
	unsigned int dataoff, datalen;
	struct nf_conntrack_expect *exp = NULL;

	uint8_t *dh = NULL;
	typeof(nf_nat_dummy_hook) nf_nat_dummy;

	printk(KERN_ALERT "[INFO] I can help!");
	

	if (IP_CT_IS_REPLY > ctinfo) {
		printk(KERN_ALERT "[INFO] A new conntrack!");
	}

	printk(KERN_ALERT "src: " NIPQUAD_FMT ":%d\n",
		NIPQUAD(ct->tuplehash[dir].tuple.src.u3.all),
		ntohs(ct->tuplehash[dir].tuple.src.u.all));
	printk(KERN_ALERT "dst: " NIPQUAD_FMT ":%d\n",
		NIPQUAD(ct->tuplehash[dir].tuple.dst.u3.all),
		ntohs(ct->tuplehash[dir].tuple.dst.u.all));

	/* we can refresh only after some checks */
	nf_ct_refresh(ct, skb, dummy_timeout * HZ);

	/* no matter skb is linear or not, dummy header(DH) must be in the
	 * head area of skb.
	 */

	// instead of skb->len. using headlen is sufficient for DH.
	headlen = skb_headlen(skb);
	// where DH begins
	dataoff = protoff + DUMMY_HDR_LEN;
	// DH + payload(possibly only part of because of non-linear)
	datalen = headlen - dataoff;
	if (headlen < dataoff + DUMMY_HDR_LEN) {
		printk(KERN_ALERT "[INFO] dummy header can not span multiple"
			"pages\n");
		return ret;
	}

	dh = skb->data + dataoff;
	if (0 != dh[0]) {
		printk(KERN_ALERT "[INFO] not dummy protocol\n");
		return NF_ACCEPT;
	}

	exp = nf_ct_expect_alloc(ct);
	if (NULL == exp) {
		return NF_DROP; 
	}
	nf_ct_expect_init(exp, NF_CT_EXPECT_CLASS_DEFAULT, nf_ct_l3num(ct),
		&ct->tuplehash[!dir].tuple.src.u3,
		(union nf_inet_addr *)dh + 1,
		IPPROTO_UDP, NULL,
		(__be16 *)dh + 5);
	
	nf_nat_dummy = rcu_dereference(nf_nat_dummy_hook);

	if (NULL != nf_nat_dummy && ct->status & IPS_NAT_MASK) {
		ret = nf_nat_dummy(skb, ctinfo, exp);
	} else {
		if (0 != nf_ct_expect_related(exp))
			ret = NF_DROP;
	}

	nf_ct_expect_put(exp);
	return ret;
}

// only support IPv4
struct helper_wrapper_t {
	struct nf_conntrack_helper helper;
	char name[sizeof("dummy-65535")]; // pointed by helper->name
};

struct helper_wrapper_t helpme[MAX_PORTS];

static void nf_conntrack_dummy_fini(void)
{
	int i = 0;
	for (i = 0; i < ports_c; ++i) {
		if (THIS_MODULE != helpme[i].helper.me)
			continue;
		nf_conntrack_helper_unregister(&helpme[i].helper);
		printk(KERN_ALERT "[INFO] nf_conntrack_helper_unregister %u\n",
			ports[i]);
	}
}

static int __init nf_conntrack_dummy_init(void)
{
	int ret, i = 0;
	memset(helpme, 0, sizeof(struct helper_wrapper_t) * MAX_PORTS);
	if (0 == ports_c)
		ports[ports_c++] = DUMMY_PORT;
	for (i = 0; i < ports_c; ++i) {
		if (DUMMY_PORT == ports[i]) {
			snprintf(helpme[i].name, sizeof("dummy-65535"),
				"dummy");
		} else {
			snprintf(helpme[i].name, sizeof("dummy-65535"),
				"dummy-%u", ports[i]);
		}

		helpme[i].helper.name = helpme[i].name;
		helpme[i].helper.me = THIS_MODULE;
		helpme[i].helper.help = dummy_help;
		helpme[i].helper.tuple.src.l3num = AF_INET;
		helpme[i].helper.tuple.src.u.udp.port = htons(ports[i]);
		helpme[i].helper.tuple.dst.protonum = IPPROTO_UDP;

		/* obsoleted in new kernel, use nf_conntrack_expect_policy */
		//helpme[i].helper.max_expected = 0;
		//helpme[i].helper.timeout = EXPECTED_TIMEOUT;

		/* can be multiple policies: expect_policy[expect_class_max] */
		helpme[i].helper.expect_policy = &dummy_exp_policy;
		helpme[i].helper.expect_class_max = NF_CT_EXPECT_CLASS_DEFAULT;


		if ((ret = nf_conntrack_helper_register(&helpme[i].helper))) {
			nf_conntrack_dummy_fini();
			return ret;
		}
		printk(KERN_ALERT "[INFO] nf_conntrack_helper_register %u\n", ports[i]);
	}
	return 0;
}

module_init(nf_conntrack_dummy_init);
module_exit(nf_conntrack_dummy_fini);
