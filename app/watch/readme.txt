看门狗模块

创建者：	Fanlv
日期：		2004-11-19	ver： 1.3

功能：

启动应用软件的其它软件程序，定时将RTC(硬件时钟)刷新到系统时钟，并且定
时对系统进行复位。 定时复位看门狗硬件(喂狗)，如果当看门狗模块不再喂狗时，
系统会复位，这样可以防止程序死机。

看门狗模块在运行时会定时检测被其启动的程序的状态，如果程序启动模式是退
出后重新运行，则看门狗会重新启动该程序，如果程序启动模式是退出后复位系
统，看门狗模块会向所有被其启动的程序发送SIGTERM信号，约30秒后复位系统。

为了系统的稳定运行，看门狗模块可以定时复位系统，复位系统的时间单位是天。
如果复位系统时间为零，表示不复位系统。

应用程序的启动的模式，应用程序的启动参数，定时复位系统的时间是可配置的。
看门狗模块在启动是通过读入配置文件(默认文件：
/home/et1000/watchdog.conf)中的参数.确定启动程序。
具体配置方式参见watchdog.conf 

应用软件的启动分成三种模式，只运行一次，退出后重新运行，退出后复位系统。

Bug revise
Date 2005-2-5		ver: 1.4
bug description: when the RTC time is invalid, the SyncTime() failed close the
RTC file descriptor before return it's caller, and so that other programs
could not reopen the RTC device.
Revision:
	to close the file descriptor when return to the caller
Author: Fanlvu
 
