#ifndef _SOFT_DOG_HPP_
#define _SOFT_DOG_HPP_
 
///////////////////////////////////////////////////////////////////////////
// 2011-8-17 修订  by laolvu
//           由于原有方案是被监控程序清计数器，监控程序（软狗）加计数器，这种方案可能导致两个进程同时操作一个文件，出现难以
//           预料的问题。因此，改成一个进程写，另外一个进程读的方案。为实现此目的，特用系统运行时间作为原来的计数器。
//           被监控程序喂狗时，只需要将喂狗时的系统运行时间写入到文件中即可。监控程序在监控时，如发现文件中的系统运行时间与
//           “当前”系统运行时间差大于最大允许时间(dead_seconds), 判断被监控程序出问题（没有及时喂狗),监控程序将杀死被监控程序
/////////////////////////////////////////////////////////////////////////////



#include <stdio.h>
#include <unistd.h>
#include <sys/sysinfo.h>

#define SOFT_WATCH_PATH "/var/volatile/watch"


struct process_t
{
    long up_time;
    long dead_seconds;
    int pid;
    char name[40];
};


//喂狗，如果超过dead_seconds秒没有喂狗，则此进程将被重启，为防止频繁操作文件，
//建议增加定时器，使得调用Feed的间隔至少在5秒以上，同时要确保间隔小于最大喂狗时间，
//process_name为线程名，长度不能超过39个字符
inline bool Feed(unsigned short dead_seconds, const char *process_name)
{
	int pid = getpid();
	if(pid<1)
	{
		return false;
	}

	struct sysinfo info;
	if ( sysinfo( &info ) != 0 )
	{
		return false;
	}


	struct process_t process = { info.uptime, dead_seconds, pid, "" };
	memset( process.name, '\0', sizeof( process.name ) );
	strncpy( process.name, process_name, sizeof(process.name) - 1 );

	char filename[sizeof(SOFT_WATCH_PATH) + 10];
	sprintf(filename, "%s/%d", SOFT_WATCH_PATH, pid);
	int fd = open(filename, O_RDWR | O_CREAT);
	if( fd < 0 )
	{
		return false;
	}
	write(fd, &process, sizeof(process_t));
	close( fd );

	return true;
}

#endif // _SOFT_DOG_HPP_
