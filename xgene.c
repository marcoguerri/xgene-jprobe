#include <linux/module.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kprobes.h>
#include <linux/printk.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <asm/page.h>

#include "crc.h"

#include "xgene_enet_main.h"
#include "xgene_enet_hw.h"
#include "xgene_enet_sgmac.h"
#include "xgene_enet_xgmac.h"

static int xgene_enet_rx_frame_probe(struct xgene_enet_desc_ring *rx_ring,
                                     struct xgene_enet_raw_desc *raw_desc) 
{
        uint32_t skb_index, datalen;
        uint8_t status;
        struct sk_buff *skb;
        struct xgene_enet_desc_ring *buf_pool;

	crc_t crc;
	crc_params_t crc_params;

        /* Get the buffer pool from the current ring */
        buf_pool = rx_ring->buf_pool;

        /* Get the index of the buffer (skb) to which the NIC DMAed to. 
         * Then get the skb itself */
        skb_index = GET_VAL(USERINFO, le64_to_cpu(raw_desc->m0));
        skb = buf_pool->rx_skb[skb_index];

        status = (GET_VAL(ELERR, le64_to_cpu(raw_desc->m0)) << LERR_LEN) ||
                  GET_VAL(LERR, le64_to_cpu(raw_desc->m0));

        if (unlikely(status > 2)) {
                printk("raw descriptor reported an error\n");
                goto out;
        }

        /* Get buffer lenght from raw_descriptor */
        datalen = GET_VAL(BUFDATALEN, le64_to_cpu(raw_desc->m1));
        datalen = (datalen & DATALEN_MASK);

        /* Verifying FCS only for non-paged sk_buff and returning the physical address
         * of the buffer */
        if(!skb_is_nonlinear(skb)) 
        {
            crc_params.type = CRC32;
	    crc_params.poly.poly_crc32 = 0x04C11DB7;
	    crc_params.crc_init.crc32 = 0xFFFFFFFF;
            crc_params.flags = CRC_INPUT_REVERSAL |
			       CRC_OUTPUT_REVERSAL |
			       CRC_OUTPUT_INVERSION;
	
            crc = crc_fast(&crc_params, (uint8_t*)skb->data, datalen-4);
            if(crc.crc32 != *((uint32_t*)(skb->data+datalen-4))){
                printk("Calculated CRC is %x,  CRC in frame is %x, phys: %016llx, %016llx\n", 
               	 	crc.crc32, 
	               *((uint32_t*)(skb->data+datalen-4)), 
                       virt_to_phys(skb->data), 
                       __pa(skb->data));
            }
        }

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
