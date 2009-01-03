/*
 *
 */

unsigned int nf_nat_dummy(struct sk_buff *skb, 
	enum ip_conntrack_info ctinfo,
	struct nf_conntrack_expect *exp)
{
	int dir = CTINFO2DIR(ctinfo);
	struct nf_conn *ct = exp->master;

	uint16_t port = 0;
	if (0 != dh[0]) {
		printk(KERN_ALERT "[INFO] not dummy protocol"
			"pages");
		return NF_ACCEPT;
	}
	port = ntohs(dh + 5);
	printk(KERN_ALERT "dummy: " NIPQUAD_FMT ":%d\n", NIPQUAD(dh + 1),
		port);




}

static void __exit nf_nat_dummy_fini(void)
{
	rcu_assign_pointer(nf_nat_ftp_hook, NULL);
	synchronize_rcu();
}

static int __init nf_nat_dummy_init(void)
{
	BUG_ON(nf_nat_ftp_hook != NULL);
	rcu_assign_pointer(nf_nat_ftp_hook, NULL);
	return 0;
}

module_init(nf_nat_dummy_init);
module_exit(nf_nat_dummy_fini);
