##Linux poll 机制分析

###参考文档
[Linux poll机制精彩分析 ](http://blog.chinaunix.net/uid-22278460-id-1777659.html)
[Linux poll机制](http://www.xuebuyuan.com/1006606.html)

###poll机制功能

函数原型：`int poll(struct pollfd    *fds ，nfds_t    nfds ，int    timeout);`

fds为指向待查询的设备文件数组；

nfds描述第一个参数fds中有多少个设备；

timeout为查询不到我们期望的结果进程睡眠的时间；

返回值：查询到期望状态的设备文件个数

	struct pollfd {     
		int fd;              /* 文件描述符 */ （待查询的设备）
		short events;   /* 等待的事件 */（待查询的设备的状态）
		short revents;  /* 实际发生了的事件 */
	
	}

功能过程描述：应用程序中调用poll查询文件的状态，首先将fds里面的每个设备文件fd取出，调用它们驱动程序的poll函数，查询是否出现我们期望状态，查询完fds里面所有的设备文件得到满足期望状态的设备文件的数量，如果这个数为0，则poll调用将导致进程就进入睡眠状态，睡眠时间由poll函数设定，如果程序在睡眠状态中fds的某个文件出现我们期望状态，那么poll立即返回，否则一直睡眠到睡眠时间结束为止，返回值为0；如果这个数大于0 ，poll返回满足条件的设备数量。
poll相当于`open("/dev/xxx",O_RDWR)`阻塞打开文件，区别在于当设备文件无数据可读时poll只导致程序休眠固定时间，而open将导致程序一直休眠到有数据为止。

###poll应用举例

	int main(int argc, char **argv)
	{
      int fd;
      unsigned char key_val;
      int ret;
      struct pollfd fds[1];//查询数组的大小，这里我们仅查询一个设备文件
      fd = open("/dev/buttons", O_RDWR);
      if (fd < 0)
          printf("can't open!\n");
      fds[0].fd     = fd;//查询的设备文件描述符为fd，也就是查询的设备是/dev/buttons
      fds[0].events = POLLIN;//查询事件是POLLIN，也就是/dev/buttons是否按下
      while (1)
      {     
           ret = poll(fds, 1, 5000);//查询的设备队列是fds，里面有1个设备，查询不到就睡眠5s，在睡眠中如果有期望状态出现也是可以返回
           if (ret == 0)    
              printf("time out\n"); //没有查询到按键按下，睡眠中也没有按键按下
           else
              {    
                  read(fd, &key_val, 1);           //查询到按键按下，读取这个按键的值
                  printf("key_val = 0x%x\n", key_val);
              }
      }
          return 0;
	}


###poll内核实现过程

对于系统调用poll或select，它们对应的内核函数都是sys_poll。分析sys_poll，即可理解poll机制.sys_poll函数位于fs/select.c文件中，代码如下：
#####函数sys_poll
	asmlinkage long sys_poll(struct pollfd __user *ufds, unsigned int nfds,
			long timeout_msecs)
	{
	s64 timeout_jiffies;

	if (timeout_msecs > 0) {
	#if HZ > 1000
		/* We can only overflow if HZ > 1000 */
		if (timeout_msecs / 1000 > (s64)0x7fffffffffffffffULL / (s64)HZ)
			timeout_jiffies = -1;
		else
	#endif
			timeout_jiffies = msecs_to_jiffies(timeout_msecs);
	} else {
		/* Infinite (< 0) or no (0) timeout */
		timeout_jiffies = timeout_msecs;
	}
	/* 重要的是调用这个函数，前面做一些参数的处理 */
	return do_sys_poll(ufds, nfds, &timeout_jiffies);
	}

#####函数 do\_sys\_poll （fs/select.c文件中）

	int do_sys_poll(struct pollfd __user *ufds, unsigned int nfds, s64 *timeout)
	{
		struct poll_wqueues table;
	 	int fdcount, err;
	 	unsigned int i;
		struct poll_list *head;
	 	struct poll_list *walk;
		/* Allocate small arguments on the stack to save memory and be
		   faster - use long to make sure the buffer is aligned properly
		   on 64 bit archs to avoid unaligned access */
		long stack_pps[POLL_STACK_ALLOC/sizeof(long)];
		struct poll_list *stack_pp = NULL;
	
		/* Do a sanity check on nfds ... */
		if (nfds > current->signal->rlim[RLIMIT_NOFILE].rlim_cur)
			return -EINVAL;
		/*初始化一个poll_wqueues变量table,
		  即table->pt->qproc = __pollwait，__pollwait将在驱动的poll函数里用到。__pollwait，它就是我们的驱动程序执行poll_wait时，真正被调用的函数。*/
		poll_initwait(&table);
	
		head = NULL;
		walk = NULL;
		i = nfds;
		err = -ENOMEM;
		while(i!=0) {
			struct poll_list *pp;
			int num, size;
			if (stack_pp == NULL)
				num = N_STACK_PPS;
			else
				num = POLLFD_PER_PAGE;
			if (num > i)
				num = i;
			size = sizeof(struct poll_list) + sizeof(struct pollfd)*num;
			if (!stack_pp)
				stack_pp = pp = (struct poll_list *)stack_pps;
			else {
				pp = kmalloc(size, GFP_KERNEL);
				if (!pp)
					goto out_fds;
			}
			pp->next=NULL;
			pp->len = num;
			if (head == NULL)
				head = pp;
			else
				walk->next = pp;
	
			walk = pp;
			if (copy_from_user(pp->entries, ufds + nfds-i, 
					sizeof(struct pollfd)*num)) {
				err = -EFAULT;
				goto out_fds;
			}
			i -= pp->len;
		}
	
		fdcount = do_poll(nfds, head, &table, timeout);//调用这个重要的函数
	
		/* OK, now copy the revents fields back to user space. */
		walk = head;
		err = -EFAULT;
		while(walk != NULL) {
			struct pollfd *fds = walk->entries;
			int j;
	
			for (j=0; j < walk->len; j++, ufds++) {
				if(__put_user(fds[j].revents, &ufds->revents))
					goto o ut_fds;
			}
			walk = walk->next;
	  	}
		err = fdcount;
		if (!fdcount && signal_pending(current))
			err = -EINTR;
	out_fds:
		walk = head;
		while(walk!=NULL) {
			struct poll_list *pp = walk->next;
			if (walk != stack_pp)
				kfree(walk);
			walk = pp;
		}
		poll_freewait(&table);
		return err;
	}

#####函数do_poll

	static int do_poll(unsigned int nfds,  struct poll_list *list,
			   struct poll_wqueues *wait, s64 *timeout)
	{
		int count = 0;
		poll_table* pt = &wait->pt;
	
		/* Optimise the no-wait case */
		if (!(*timeout))
			pt = NULL;
	 
		for (;;) {
			struct poll_list *walk;
			long __timeout;
	
			set_current_state(TASK_INTERRUPTIBLE);
			for (walk = list; walk != NULL; walk = walk->next) {
				struct pollfd * pfd, * pfd_end;
	
				pfd = walk->entries;
				pfd_end = pfd + walk->len;
				/* 逐个取出待查询数组中的每个文件描述符来查询*/
				for (; pfd != pfd_end; pfd++) {
					/*
					 * Fish for events. If we found one, record it
					 * and kill the poll_table, so we don't
					 * needlessly register any other waiters after
					 * this. They'll get immediately deregistered
					 * when we break out and return.
					 */
					/*最终会调用驱动程序实现的poll函数，如果驱动程序返回的mask非0，则说明有数据，条件为真，这将会让下面跳出循环*/
					if (do_pollfd(pfd, pt)) {
						count++;
						pt = NULL;
					}
				}
			}
			/*
			 * All waiters have already been registered, so don't provide
			 * a poll_table to them on the next loop iteration.
			 */
			pt = NULL;
			//超时或者有设备文件可读，跳出循环，程序直接返回
			if (count || !*timeout || signal_pending(current))
				break;
			count = wait->error;
			if (count)
				break;
	
			if (*timeout < 0) {
				/* Wait indefinitely */
				__timeout = MAX_SCHEDULE_TIMEOUT;
			} else if (unlikely(*timeout >= (s64)MAX_SCHEDULE_TIMEOUT-1)) {
				/*
				 * Wait for longer than MAX_SCHEDULE_TIMEOUT. Do it in
				 * a loop
				 */
				__timeout = MAX_SCHEDULE_TIMEOUT - 1;
				*timeout -= __timeout;
			} else {
				__timeout = *timeout;
				*timeout = 0;
			}
			//程序睡眠
			__timeout = schedule_timeout(__timeout);
			if (*timeout >= 0)
				*timeout += __timeout;
		}
		__set_current_state(TASK_RUNNING);
		return count;
	}



#####函数do_pollfd

	/*
	 * Fish for pollable events on the pollfd->fd file descriptor. We're only
	 * interested in events matching the pollfd->events mask, and the result
	 * matching that mask is both recorded in pollfd->revents and returned. The
	 * pwait poll_table will be used by the fd-provided poll handler for waiting,
	 * if non-NULL.
	 */
	static inline unsigned int do_pollfd(struct pollfd *pollfd, poll_table *pwait)
	{
		unsigned int mask;
		int fd;
	
		mask = 0;
		fd = pollfd->fd;
		if (fd >= 0) {
			int fput_needed;
			struct file * file;
	
			file = fget_light(fd, &fput_needed);
			mask = POLLNVAL;
			if (file != NULL) {
				mask = DEFAULT_POLLMASK;
				if (file->f_op && file->f_op->poll)
					/*调用驱动程序的poll函数*/
					mask = file->f_op->poll(file, pwait);
				/* Mask out unneeded events. */
				mask &= pollfd->events | POLLERR | POLLHUP;
				fput_light(file, fput_needed);
			}
		}
		pollfd->revents = mask;
	
		return mask;
	}

###一般驱动程序实现的poll方法

	static unsigned int fourth_drv_poll(struct file *file, poll_table *wait)  
	{  
	    unsigned int mask = 0;  
	  
	    /* 该函数，只是将进程挂在button_waitq队列上，而不是立即休眠 */  
	    poll_wait(file, &button_waitq, wait);  
	  
	    /* 当没有按键按下时，即不会进入按键中断处理函数，此时ev_press = 0  
	     * 当按键按下时，就会进入按键中断处理函数，此时ev_press被设置为1 
	     */  
	    if(ev_press)  
	    {  
	        mask |= POLLIN | POLLRDNORM;  /* 表示有数据可读 */  
	    }  
	  
	    /* 如果有按键按下时，mask |= POLLIN | POLLRDNORM,否则mask = 0 */  
	    return mask;    
	} 


###poll机制总结：
1.poll > sys_poll > do_sys_poll > poll_initwait，poll_initwait函数注册一下回调函数__pollwait，它就是我们的驱动程序执行poll_wait时，真正被调用的函数。

2.接下来执行file->f_op->poll，即我们驱动程序里自己实现的poll函数
它会调用poll_wait把自己挂入某个队列，这个队列也是我们的驱动自己定义的；
它还判断一下设备是否就绪。

3.如果设备未就绪，do_sys_poll里会让进程休眠一定时间

4.进程被唤醒的条件有2：一是上面说的“一定时间”到了，二是被驱动程序唤醒。驱动程序发现条件就绪时，就把“某个队列”上挂着的进程唤醒，这个队列，就是前面通过poll_wait把本进程挂过去的队列。

5.如果驱动程序没有去唤醒进程，那么chedule_timeout(__timeou)超时后，会重复2、3动作，直到应用程序的poll调用传入的时间到达