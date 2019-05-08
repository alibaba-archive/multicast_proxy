#include <linux/ip.h>
#include <net/ip.h>
#include <net/tcp.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <linux/module.h>

#include "tmcc_nl.h"
#include "grp.h"
#include "nf.h"
#include "nl.h"

static int multi_init(void)
{
    int ret;
    
    ret = multi_node_init();
    if(ret < 0)
        goto FAIL;

    ret = multi_nl_init();
    if(ret < 0)
        goto MULTI_NODE_FAIL;

    ret = multi_nf_init();
    if(ret < 0)
        goto MULTI_NL_FAIL;

    printk(KERN_INFO"multi client init success!\n");
    return 0;
    
MULTI_NL_FAIL:
    multi_nl_fini();
    
MULTI_NODE_FAIL:
    multi_node_fini();

FAIL:
    return ret;

}

static void multi_fini(void)
{
    multi_nl_fini();
    multi_node_fini();
    multi_nf_fini();
    printk(KERN_INFO"multi server fini!\n");
}

module_init(multi_init);
module_exit(multi_fini);

MODULE_LICENSE("GPL");
