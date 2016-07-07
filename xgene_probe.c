#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kprobes.h>
#include <linux/printk.h>
#include <linux/errno.h>

#include "xgene_enet_main.h"
#include "xgene_enet_hw.h"
#include "xgene_enet_sgmac.h"
#include "xgene_enet_xgmac.h"

static int done = 0;
static int xgene_enet_rx_frame_probe(struct xgene_enet_desc_ring *rx_ring,
                                     struct xgene_enet_raw_desc *raw_desc) 
{
        u32 skb_index, datalen;
        u8 status;
        struct sk_buff *skb;
        struct xgene_enet_desc_ring *buf_pool;

        printk("Calling probe function\n");
        if(done == 1)
            jprobe_return();
       
        /* Get the buffer pool from the current ring */
        buf_pool = rx_ring->buf_pool;

        /* Get the index of the buffer (skb) to which the NIC DMAed to. Then get the skb itself */
        skb_index = GET_VAL(USERINFO, le64_to_cpu(raw_desc->m0));
        skb = buf_pool->rx_skb[skb_index];

        status = (GET_VAL(ELERR, le64_to_cpu(raw_desc->m0)) << LERR_LEN) ||
                  GET_VAL(LERR, le64_to_cpu(raw_desc->m0));

        if (unlikely(status > 2)) {
                printk("raw descriptor reported and error\n");
                goto out;
        }

        /* Get buffer lenght from raw_descriptor */
        datalen = GET_VAL(BUFDATALEN, le64_to_cpu(raw_desc->m1));
        datalen = (datalen & DATALEN_MASK);

        /* Trying to read datalen from skb->data? This will work only if skb is not paged? */
        printk("CRC is %x\n", *((u32*)(skb->data+datalen-4)));

        done = 1;
out:
        jprobe_return();
        return 0;
}


static struct jprobe xgene_probe;
 
int xgene_probe_init(void)
{
    int ret = 0;
    printk("Installing xgene probe\n");
    xgene_probe.kp.symbol_name  = "xgene_enet_rx_frame";
    xgene_probe.entry =  xgene_enet_rx_frame_probe;
    if((ret = register_jprobe(&xgene_probe)) < 0)
    {
        printk("jprobe registration failed\n");
        return ret;
    }
    return 0;
}
 
void xgene_probe_exit(void)
{
    printk("Unregistering xgene_probe\n");
    unregister_jprobe(&xgene_probe);
}
 
module_init(xgene_probe_init);
module_exit(xgene_probe_exit);
 
MODULE_AUTHOR("Marco Guerri");
MODULE_DESCRIPTION("jprobe to debug data corruption issue");
MODULE_LICENSE("GPL");
