###Linux kernel 定时器

转载于:  [Linux 内核定时器 ](http://blog.csdn.net/shui1025701856/article/details/7580280)

定时器，有时也称为动态定时器或内核定时器，是管理内核时间的基础,内核经常要推迟执行一些代码，如下半部机制就是为了将工作推后执行,时钟中断由系统的定时硬件以周期性的时间间隔产生，这个间隔（即频率）由内核根据HZ来确定,每当时钟中断发生时，全局变量jiffies(unsigned long)就加1，因此jiffies记录了自linux启动后时钟中断发生的次数。内核定时器用于控制某个函数(定时器处理函数)在未来的某个特定时间执行。
核定时器注册的处理函数只执行一次--不是循环执行的。(定时器并不周期运行，它在超时后就自行销毁,动态定时器不断地创建和销毁，而且它的运行次数也不受限制)。定时器的使用只须执行一些初始化工作，设置一个超时时间，指定超时发生后执行的函数，然后激活定时器就可以了

* 定时器结构timer_list
	
	
		include/linux/timer.h
		struct timer_list {
	
		 struct list_head entry;//定时器链表的入口
		 unsigned long expires;//定时器到期时间
		 struct tvec_base *base;
		
		 void (*function)(unsigned long);//定时器处理函数
		 unsigned long data;//传给定时器处理函数的长整形参数
		
		 int slack;
		
		 ...
		
		};

*  定时器的使用

1.定义一个定时器结构

`struct timer_list timer;`

2.初始化定时器

有很多接口函数可以初始化定时器

	init_timer(struct timer_list* timer);
	
	TIMER_INITIALIZER(_function, _expires, _data);
	
	DEFINE_TIMER(_name, _function, _expires, _data);

通常可以这样初始化定时器并赋值func,data

	setup_timer(&timer, gpio_keys_timer, (unsigned long)data);

定时器处理函数

	static void gpio_keys_timer(unsigned long data)
	{
	 schedule_work(&work);
	}

3.增加定时器,并激活定时器

	void add_timer(struct timer_list* timer);

注册内核定时器，并将定时器加入到内核动态定时器链表中

4,.删除定时器

	int del_timer(struct timer_list *timer);
	del_timer_sync(struct timer_list *timer);
	
del\_timer\_sync()是del_timer()的同步版,在删除一个定时器时需等待其被处理完(不能在中断上下文中使用)

5.修改定时器的expire,并启动

	int mod_timer(struct timer_list *timer, unsigned long expires);
	mod_timer(&timer, jiffies + msecs_to_jiffies(50));//未考虑jiffies溢出问题
	msecs_to_jiffies()用于将ms转换成jiffies
####定时器使用的例子


	#include <linux/init.h>
	#include <linux/module.h>
	#include <linux/timer.h>
	#include <linux/fs.h>
	
	#define TIMER_MAJOR 234
	#define DEVICE_NAME "timer_test"
	
	/**1. 定义timer结构*/
	struct timer_list timer;
	
	static void func_timer(unsigned long data)
	{
		/**4. 修改定时器的超时参数并重启*/
		mod_timer(&timer, jiffies + HZ);
		printk("current jiffies is %ld\n", jiffies);
	}
	
	struct file_operations timer_ops = {
		.owner = THIS_MODULE,
	};
	
	static int __init timer_init(void)
	{
		register_chrdev(TIMER_MAJOR, DEVICE_NAME, &timer_ops);
		/**2. 初始化定时器*/
		setup_timer(&timer, func_timer, 0);
		#if 0
		init_timer(&timer);
		timer.data = 0;
		timer.expires = jiffies + HZ;
		timer.function = func_timer;
		#endif
		/**3. 添加激活计时器*/
		add_timer(&timer);
		
		printk("timer_init\n");
		return 0;
	}
	
	static void __exit timer_exit(void)
	{
		/**4. 删除定时器*/
		del_timer(&timer);
		unregister_chrdev(TIMER_MAJOR, DEVICE_NAME);
	}
	
	module_init(timer_init);
	module_exit(timer_exit);
	MODULE_LICENSE("GPL");

