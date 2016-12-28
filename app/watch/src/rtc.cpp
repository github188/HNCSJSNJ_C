#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>

#include "rtc.h"
#include "rtc.hpp"

const char * RTClock::name = "/dev/wfet1000clock";

RTClock::RTClock()
{

}

bool RTClock::Open()
{
  fd = open( name, O_RDWR );
  if ( fd < 0 ) {
    fd = 0;
    return false;
  }

  return true;

}

bool RTClock::Close()
{

  if ( fd > 0 ) {
    close( fd );
  }

  return true;

}

bool RTClock::SyncTime()
{
  int i;
  unsigned char r_buf[8];

  struct tm i2cTime;

  struct timeval tv;
  struct timezone tmz;
  time_t temp = 0;

  char * cputype = getenv( "cputype" );
  if ( !cputype )
    cputype = (char *)"2410";

  if ( !Open() ) {
    return false;
  }

  if ( 0 == strcmp( cputype, "9260" ) ) 
    {
      if ( !Get9260TrueTime( &i2cTime ) )
	{
	  Close();
	  return false;
	}
    }
  else
    {
      if ( ( i = read( fd, r_buf, 8 ) ) != 8 ) {
	Close();
	return false;
      }

      r_buf[5] &= 0x7f;		// 有时rtc返回的小时第7的bit位会置1, 这时候为正常情况
      if ( !isTimeValid( r_buf + 1 ) ) {
	Close();
	return false;
      }
      i2cTime.tm_year = BCD2Dec( r_buf[ 1 ] ) + 100;
      i2cTime.tm_mon  = BCD2Dec( r_buf[ 2 ] ) - 1;
      i2cTime.tm_mday = BCD2Dec( r_buf[ 3 ] );
      i2cTime.tm_wday = BCD2Dec( r_buf[ 4 ] ); 
      i2cTime.tm_hour = BCD2Dec( r_buf[ 5 ] );
      i2cTime.tm_min  = BCD2Dec( r_buf[ 6 ] );
      i2cTime.tm_sec  = BCD2Dec( r_buf[ 7 ] );
    }
  gettimeofday( &tv, &tmz );

  temp = mktime( &i2cTime );
  tv.tv_sec = ( long )temp;
		
  settimeofday( &tv, &tmz );

  Close();
  return true;
}

unsigned char  RTClock::BCD2Dec( unsigned char bcdval )
{
  return ( ( ( bcdval & 0xf0 ) >> 4 ) * 10 ) + ( bcdval & 0x0f );
}

bool RTClock::isTimeValid( unsigned char * const bcd_time )
{
  if ( bcd_time == NULL ) {
    return false;
  }


  int year = BCD2Dec( bcd_time[0] );
  int mon  = BCD2Dec( bcd_time[1] );
  int day  = BCD2Dec( bcd_time[2] );
  int hour = BCD2Dec( bcd_time[4] );
  int min  = BCD2Dec( bcd_time[5] );
  int sec  = BCD2Dec( bcd_time[6] );

  if ( year > 40 ) {
    return false;
  }


  if ( ( year > 99 ) 
       || (  mon == 0 || mon > 12 )  
       || ( day  > 31 || day == 0 ) 
       || ( hour > 23  || min > 59 || sec > 59 ) ) {
    return false;
  }


  // to judge more complicate cases
  time_t now = time( NULL );
  struct tm date;
  localtime_r( &now, &date );
  date.tm_year = year + 100 ;
  date.tm_mon = mon - 1;
  date.tm_mday = day;
  now = mktime( &date );	
  if ( mon != date.tm_mon + 1 || day != date.tm_mday ) {
    return false;
  }

 
  return true;
}



//////////////////////////////////////////////////////
// 从内核中读取时钟，考虑到硬件时钟信号质量不好，有可能造成时钟跳变，因此需要进行错误处理
// 方式是： 连续读取内核时间若干（默认5次），如果连续几次次的时间相差在允许范围秒内，
// 认为是硬件没有受到干扰，读取到的时钟是可信的。
// 另外，如果某次读取的时钟是非法的，也返回错误
/////////////////////////////////////////////////////
bool RTClock::Get9260TrueTime( struct tm * date )
{
  const int READ_TIMES = 5;

  time_t tmp = 0;

  int min = INT_MAX; 
  int max = INT_MIN;
  for ( int i = 0; i < READ_TIMES; i ++ )
    {
      if( ioctl( fd, RTC_RD_TIME, date ) < 0 )
	{
	  perror("ioctl");
	  return false;
	}

      if ( ( date->tm_mon < 0  || date->tm_mon > 11 )  
	   || ( date->tm_mday > 31 || date->tm_mday == 0 ) 
	   || ( date->tm_hour > 23 || date->tm_min > 59 || date->tm_sec > 59 ) ) 
	{
	  // date is not valid 
	  return false;
	}

      tmp = mktime( date );
      if ( min > tmp )
	min = tmp;

      if ( max < tmp )
	max = tmp;
    }
  
  return ( ( max - min ) < READ_TIMES );   
} 
