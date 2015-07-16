##Linux kernel 同步互斥和阻塞

###原子操作

原子操作指的是在执行过程中不会被别的代码路径所中断的操作。
常用原子操作函数举例：

`atomic_t v = ATOMIC_INIT(0);`     //定义原子变量v并初始化为0

`atomic_read(atomic_t *v);`        //返回原子变量的值

`void atomic_inc(atomic_t *v);`    //原子变量增加1

`void atomic_dec(atomic_t *v);`    //原子变量减少1

`int atomic_dec_and_test(atomic_t *v);` //自减操作后测试其是否为0，为0则返回true，否则返回false。

###互斥锁与信号量

Linux内核中解决并发控制的最常用方法是自旋锁与信号量（绝大多数时候作为互斥锁使用）。自旋锁与信号量不同的地方：自旋锁不会引起调用者睡眠，如果自旋锁已经被别的执行单元保持，调用者就一直循环查看是否该自旋锁的保持者已经释放了锁，"自旋"就是"在原地打转"。而信号量则引起调用者睡眠，它把进程从运行队列上拖出去，除非获得锁。自旋锁与信号量类似的地方：无论是信号量，还是自旋锁，在任何时刻，最多只能有一个保持者，即在任何时刻最多只能有一个执行单元获得锁。

一般而言，自旋锁适合于保持时间非常短的情况，它可以在任何上下文使用,但是，自旋锁不能递归使用，这是因为自旋锁，在设计之初就被设计成在不同进程或者函数之间同步。所以不能用于递归使用。信号量适合于保持时间较长的情况，会只能在进程上下文使用。如果被保护的共享资源只在进程（中断中不可以用）上下文访问，则可以以信号量来保护该共享资源，如果对共享资源的访问时间非常短，自旋锁也是好的选择。但是，如果被保护的共享资源需要在中断上下文访问（包括底半部即中断处理句柄和顶半部即软中断），就必须使用自旋锁。 

信号量：

	struct semaphore sem; //定义信号量
	void sema_init (struct semaphore *sem, int val);//初始化信号量，并把信号量设置为val
	void init_MUTEX (struct semaphore *sem);//该函数用于初始化一个互斥锁，即它把信号量sem的值设置为1，等同于sema_init (struct semaphore *sem, 1)；
	void init_MUTEX_LOCKED (struct semaphore *sem);//该函数也用于初始化一个互斥锁，但它把信号量sem的值设置为0，等同于sema_init (struct semaphore *sem, 0)；
	void down(struct semaphore * sem);//该函数用于获得信号量sem，它会导致睡眠，因此不能在中断上下文使用；
	int down_interruptible(struct semaphore * sem);//该函数功能与down类似，不同之处为，down不能被信号打断，但down_interruptible能被信号打断；
	int down_trylock(struct semaphore * sem);//该函数尝试获得信号量sem，如果能够立刻获得，它就获得该信号量并返回0，否则，返回非0值。它不会导致调用者睡眠，可以在中断上下文使用。
	void up(struct semaphore * sem);//该函数释放信号量sem，唤醒等待者。

自旋锁

	spinlock_t spin;//定义自旋锁
	spin_lock_init(lock)//初始化自旋锁
	spin_lock(lock)//该宏用于获得自旋锁lock，如果能够立即获得锁，它就马上返回，否则，它将自旋在那里，直到该自旋锁的保持者释放；
	spin_trylock(lock)//该宏尝试获得自旋锁lock，如果能立即获得锁，它获得锁并返回真，否则立即返回假，实际上不再"在原地打转"；
	spin_unlock(lock)//该宏释放自旋锁lock，它与spin_trylock或spin_lock配对使用；

###阻塞与非阻塞

######应用程序阻塞与非阻塞的区别

阻塞地都取串口一个字符

	char buf;
	fd = open("/dev/ttys",O_RDWR);
	.. ..
	res = read(fd,&buf,1); //当串口上有输入时才返回
	if(res == 1)
	{
	     printf("%c\n",buf);
	}

非阻塞地都取串口一个字符

	char buf;
	fd = open("/dev/ttys",O_RDWR | O_NONBLOCK);
	.. ..
	while( read(fd,&buf,1) !=1); //当串口上无输入也返回,所
	                                                //以要循环尝试读取串口
	printf("%c\n",buf); 


相关API

	wait_event(queue, conditon);
	
	wait_event_interruptible(queue, condition);//可以被信号打断
	
	wait_event_timeout(queue, condition, timeout);
	
	wait_event_interruptible_timeout(queue, condition, timeout);//不能被信号打断

queue:作为等待队列头的等待队列被唤醒

conditon：必须满足，否则阻塞

timeout和conditon相比，有更高优先级

	void wake_up(wait_queue_head_t *queue);
	
	void wake_up_interruptible(wait_queue_head_t *queue);

上述操作会唤醒以queue作为等待队列头的所有等待队列中所有属于该等待队列头的等待队列对应的进程。

	sleep_on(wait_queue_head_t *q);
	
	interruptible_sleep_on(wait_queue_head_t *q);

sleep_on作用是把目前进程的状态置成TASK_UNINTERRUPTIBLE,并定义一个等待队列，之后把他附属到等待队列头q，直到资源可用，q引导的等待队列被唤醒。interruptible_sleep_on作用是一样的， 只不过它把进程状态置为TASK_INTERRUPTIBLE.

这两个函数的流程是首先，定义并初始化等待队列，把进程的状态置成TASK_UNINTERRUPTIBLE或TASK_INTERRUPTIBLE，并将对待队列添加到等待队列头。

然后通过schedule(放弃CPU,调度其他进程执行。最后，当进程被其他地方唤醒，将等待队列移除等待队列头。

在Linux内核中，使用set_current_state()和__add_wait_queue()函数来实现目前进程状态的改变，直接使用current->state = TASK_UNINTERRUPTIBLE类似的语句也是可以的。

因此我们有时也可能在许多驱动中看到，它并不调用sleep_on或interruptible_sleep_on(),而是亲自进行进程的状态改变和切换。