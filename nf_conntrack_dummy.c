
/*
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/netfilter.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/ctype.h>
#include <linux/inet.h>
*/

#include <linux/module.h>
#include <linux/ctype.h>
//#include <linux/skbuff.h>
//#include <linux/inet.h>
//#include <linux/in.h>
#include <linux/udp.h>
#include <linux/netfilter.h>

#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_expect.h>
#include <net/netfilter/nf_conntrack_helper.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chou Chifeng <cfchou@gmail.com>");
MODULE_DESCRIPTION("dummy connection tracking helper");
MODULE_ALIAS("ip_conntrack_dummy");

#define DUMMY_PORT	2008
#define MAX_PORTS	8
#define DUMMY_TIMEOUT	300
#define EXPECTED_TIMEOUT	30

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


static int dummy_help(struct sk_buff *skb,
		    unsigned int protoff,
		    struct nf_conn *ct,
		    enum ip_conntrack_info ctinfo)
{
	int ret = NF_ACCEPT;
	printk(KERN_ALERT "[INFO] I can help!");
	

	/* we can refresh only after some checks */
	nf_ct_refresh(ct, skb, dummy_timeout * HZ);


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
