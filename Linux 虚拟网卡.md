###Linux 虚拟网卡

看韦东山的视频实现了虚拟网卡，于是在自己开发板上实现虚拟网卡，开发板上的Linux系统用的是2.6.35.7版本的，但是韦东山视频用的是2.6.22.6的Linux系统版本，在写的过程中发现还是有很多不一样的地方，同时，也遇到了一些坑，以此记录：

	
	#include <linux/module.h>
	#include <linux/errno.h>
	#include <linux/netdevice.h>
	#include <linux/etherdevice.h>
	#include <linux/kernel.h>
	#include <linux/types.h>
	#include <linux/fcntl.h>
	#include <linux/interrupt.h>
	#include <linux/ioport.h>
	#include <linux/in.h>
	#include <linux/skbuff.h>
	#include <linux/spinlock.h>
	#include <linux/string.h>
	#include <linux/init.h>
	#include <linux/bitops.h>
	#include <linux/delay.h>
	#include <linux/gfp.h>
	
	
	#include <linux/ip.h>
	#include <linux/if_ether.h>
	
	#include <asm/system.h>
	#include <asm/io.h>
	#include <asm/irq.h>
	
	
	
	
	static struct net_device *virnet_dev;
	
	static void emulator_rx_packet(struct sk_buff *skb,
				 struct net_device *dev){
				 
		/* 参考LDD3 */
		unsigned char *type;
		struct iphdr *ih;
		__be32 *saddr, *daddr, tmp;
		unsigned char	tmp_dev_addr[ETH_ALEN];
		struct ethhdr *ethhdr;
		
		struct sk_buff *rx_skb;
				
		// 从硬件读出/保存数据
		/* 对调"源/目的"的mac地址 */
		ethhdr = (struct ethhdr *)skb->data;
		memcpy(tmp_dev_addr, ethhdr->h_dest, ETH_ALEN);
		memcpy(ethhdr->h_dest, ethhdr->h_source, ETH_ALEN);
		memcpy(ethhdr->h_source, tmp_dev_addr, ETH_ALEN);
	
		/* 对调"源/目的"的ip地址 */    
		ih = (struct iphdr *)(skb->data + sizeof(struct ethhdr));
		saddr = &(ih->saddr);
		daddr = &(ih->daddr);
	
		tmp = *saddr;
		*saddr = *daddr;
		*daddr = tmp;
		
		//((u8 *)saddr)[2] ^= 1; /* change the third octet (class C) */
		//((u8 *)daddr)[2] ^= 1;
		type = skb->data + sizeof(struct ethhdr) + sizeof(struct iphdr);
		//printk("tx package type = %02x\n", *type);
		// 修改类型, 原来0x8表示ping
		*type = 0; /* 0表示reply */
	
		/* 重新校验*/
		ih->check = 0;		   /* and rebuild the checksum (ip needs it) */
		ih->check = ip_fast_csum((unsigned char *)ih,ih->ihl);
		
		// 构造一个sk_buff
		rx_skb = dev_alloc_skb(skb->len + 2);
		skb_reserve(rx_skb, 2); /* align IP on 16B boundary */	
		memcpy(skb_put(rx_skb, skb->len), skb->data, skb->len);
	
		/* Write metadata, and then pass to the receive level */
		rx_skb->dev = dev;
		rx_skb->protocol = eth_type_trans(rx_skb, dev);
		rx_skb->ip_summed = CHECKSUM_UNNECESSARY; /* don't check it */
		dev->stats.rx_packets++;
		dev->stats.rx_bytes += skb->len;
	
		// 提交sk_buff
		netif_rx(rx_skb);
	}
	
	
	static netdev_tx_t virt_net_send_packet(struct sk_buff *skb,
					struct net_device *dev){
	
		static int count = 0;
		printk("virt_net_send_packet count = [%d]\n ", ++count);
		/* 对于真实的网卡, 把skb里的数据通过网卡发送出去 */
		netif_stop_queue(dev);/* 停止该网卡的队列 */
	
		/* ...... */           /* 把skb的数据写入网卡 */
		/* 构造一个假的sk_buff,上报 */
		emulator_rx_packet(skb, dev);
		 
		dev_kfree_skb (skb);/* 释放skb */
		netif_wake_queue(dev);/* 数据全部发送出去后,唤醒网卡的队列 */
		/* 更新统计信息 */
		dev->stats.tx_packets++;
		dev->stats.tx_bytes +=skb->len;
	
		
		return NETDEV_TX_OK;
	} 
	/*这个地方一开始是直接引用，但是一直报错，最后看
	loopback.c实现方式，才想到了这个办法，目的也是赋值*/
	static const struct net_device_ops virnet_ops = {
	    .ndo_start_xmit= virt_net_send_packet,
	};
	
	static int virt_net_init(void){
		virnet_dev = alloc_netdev(0, "virnet%d", ether_setup); /* alloc_etherdev*/
		if(NULL == virnet_dev){
			printk("virnet_dev is null \n");
			goto out1;
		}
		printk("virnet_dev is ok!\n");
		/*两个版本不一样的地方，这个地方给发送函数赋值
		在这个地方卡了一段时间 ， 如果不给发送函数赋值，
		在下面register_netdev这个函数调用的时候就会说空指针*/
		virnet_dev->netdev_ops = &virnet_ops;
	
		virnet_dev->dev_addr[0] = 0x08;
		virnet_dev->dev_addr[0] = 0x89;
		virnet_dev->dev_addr[0] = 0x89;
		virnet_dev->dev_addr[0] = 0x89;
		virnet_dev->dev_addr[0] = 0x89;
		virnet_dev->dev_addr[0] = 0x89;
	
		virnet_dev->flags  |= IFF_NOARP;
		virnet_dev->features |= NETIF_F_NO_CSUM;
		
		if(register_netdev(virnet_dev)){
			printk("register_netdev is error\n ");
			goto out2;
		}
		return 0;
	
	out2:
		unregister_netdev(virnet_dev);
	
	out1:
		free_netdev(virnet_dev);
		return -1;
	
	
	
	}
	
	static void virt_net_exit(void){
	
		unregister_netdev(virnet_dev);
		free_netdev(virnet_dev);
	
	}
	
	module_init(virt_net_init);
	module_exit(virt_net_exit);
	
	
	MODULE_AUTHOR("fhw");
	MODULE_LICENSE("GPL");



#####总结

1. 因为看视频的时候第一个版本没有给发送函数赋值，所以一开始也没有给发送版本赋值，但是一直报空指针的错误，一直怀疑是空间分配失败，但不是，最后发现是没有给空指针赋值。
2. 赋值的时候因为内核版本不一样，所以 net_device结构体的成员也不一样。
3. 因为没有引用以下两个头文件报错：

以下是头文件和报错
	
	#include <linux/ip.h>
	#include <linux/if_ether.h>


	make[1]: 正在进入目录 `/home/tyxm/src/chip/fri_linux_2.6.35.7'
	  CC [M]  /home/tyxm/src/chip/device/net_device/virt_net_fir.o
	/home/tyxm/src/chip/device/net_device/virt_net_fir.c: In function 'emulator_rx_packet':
	/home/tyxm/src/chip/device/net_device/virt_net_fir.c:45:15: error: dereferencing pointer to incomplete type
	/home/tyxm/src/chip/device/net_device/virt_net_fir.c:46:15: error: dereferencing pointer to incomplete type
	/home/tyxm/src/chip/device/net_device/virt_net_fir.c:54:53: error: invalid application of 'sizeof' to incomplete type 'struct iphdr' 
	/home/tyxm/src/chip/device/net_device/virt_net_fir.c:59:5: error: dereferencing pointer to incomplete type
	/home/tyxm/src/chip/device/net_device/virt_net_fir.c:60:5: error: dereferencing pointer to incomplete type
	/home/tyxm/src/chip/device/net_device/virt_net_fir.c:60:50: error: dereferencing pointer to incomplete type
	make[2]: *** [/home/tyxm/src/chip/device/net_device/virt_net_fir.o] 错误 1
	make[1]: *** [_module_/home/tyxm/src/chip/device/net_device] 错误 2
	make[1]:正在离开目录 `/home/tyxm/src/chip/fri_linux_2.6.35.7'
