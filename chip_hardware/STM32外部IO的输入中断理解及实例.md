#STM32外部IO的输入中断理解及实例

###异常与中断

Cortex-M3 在内核水平上搭载了一个异常响应系统，支持为数众多的系统异常和外部中断。其中，编号为 1－15 的对应系统异常(其中包括复位，SysTic等)，大于等于 16 的则全是外部中断。NVIC 的寄存器以存储器映射的方式来访问，除了包含控制寄存器和中断处理的控制逻辑之外， NVIC 还包含了MPU、 SysTick 定时器以及调试控制相关的寄存器。STM32F103 的中断控制器支持 19 个外部中断/事件请求。每个中断设有状态位，每个中断/事件都有独立的触发和屏蔽设置。STM32F103 的19 个外部中断为：

1. 线 0~15：对应外部 IO 口的输入中断。
2. 线 16：连接到 PVD 输出。
3. 线 17：连接到 RTC 闹钟事件。
4. 线 18：连接到 USB 唤醒事件。

线0~15对应的是STM32的GPIO管脚，STM32GPIO 的管脚GPIOx.0~GPIOx.15(x=A,B,C,D,E，F,G)分别对应中断线 0~15。这样每个中断线对应了最多 7 个 IO 口，中断线每次只能连接到 1 个 IO 口上，这样就需要通过配置来决定对应的中断线配置到哪个 GPIO 上了。
EXTI线16连接到PVD输出
EXTI线17连接到RTC闹钟事件
EXTI线18连接到USB唤醒事件
EXTI线19连接到以太网唤醒事件(只适用于互联型产品)

###GPIO口的外部中断

STM32 的每个 IO 都可以作为外部中断的中断输入口，这样就可以配置GPIO口来设置中断，比如可以把按键的输入引脚设置为外部中断的输入引脚，这样在按键按下的时候就可以触发中断，以下用正点原子的mini板子来说明GPIO的外部中断：

配置外部中断：

按键KEY0和KEY1用的STM32F103RBT6的是GPIOA的13脚和15脚，所以这里要把GPIOA.13和GPIOA.15初始化：

	void KEY_Init(void) //GPIOA.13和GPIOA.15初始化
	{ 
	 	GPIO_InitTypeDef GPIO_InitStructure;
		//初始化KEY0-->GPIOA.13,KEY1-->GPIOA.15  上拉输入
	 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
		GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_13|GPIO_Pin_15;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	 	GPIO_Init(GPIOA, &GPIO_InitStructure);
	}


GPIOA.13 中断线以及中断初始化配置

	//把外部中断与中断线链接
	GPIO_EXTILineConfi(GPIO_PortSourceGPIOA,GPIO_PinSource13);
	EXTI_InitStructure.EXTI_Line=EXTI_Line13;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);	 	//根据EXTI_InitStruct中指定的参数初始化外设EXTI寄存器

GPIOA.15 中断线以及中断初始化配置

	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA,GPIO_PinSource15);
	EXTI_InitStructure.EXTI_Line=EXTI_Line15;//设置中断线的标号
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;//中断模式，可选值为中断 EXTI_Mode_Interrupt 和事件 EXTI_Mode_Event。
	
	//触发方式，可以是下降沿触发 EXTI_Trigger_Falling，上升沿触发 EXTI_Trigger_Rising，或者任意电平（上升沿和下降沿）触发EXTI_Trigger_Rising_Falling，
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);	  	//根据EXTI_InitStruct中指定的参数初始化外设EXTI寄存器

设置 NVIC 中断优先级：中断优先级有抢占优先级和子优先级，NVIC 中有一个寄存器是“应用程序中断及复位控制寄存器” （内容见表 7.5） ，它里面有一个位段名为“优先级组” 。该位段的值对每一个优先级可配置的异常都有影响——把其优先级分为 2 个位段：MSB 所在的位段（左边的）对应抢占优先级，而 LSB 所在的位段（右边的）对应子优先级，如果优先级完全相同的多个异常同时悬起，则先响应异常编号最小的那一个。
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;//使能按键所在的外部中断通道
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x02;//抢占优先级2， 
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x01;//子优先级1
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;//使能外部中断通道
	NVIC_Init(&NVIC_InitStructure);


中断服务函数：中断服务函数是ST已经定义好的，也就是说函数名已经声明，只需要定义相应的函数，在中断被触发时，中断函数将被调用进行处理。在这里，10到15的中断线公用同一个中断函数

	void EXTI15_10_IRQHandler(void)
	{
	  	delay_ms(10);    //消抖			 
	 
	    if(EXTI_GetITStatus(EXTI_Line13) != RESET)
		{
		 	LED0=!LED0;
		}
	 
	     else if (EXTI_GetITStatus(EXTI_Line15) != RESET)
		{
			LED1=!LED1;
		}
		 EXTI_ClearITPendingBit(EXTI_Line13);  //清除EXTI13线路挂起位
		 EXTI_ClearITPendingBit(EXTI_Line15);  //清除EXTI15线路挂起位
	} 