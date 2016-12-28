#ifndef _RTC_HPP
#define _RTC_HPP

////////////////////////////////////////////////////////////
//Real Time Clock Class
//
////////////////////////////////////////////////////////////
class RTClock {
public:
  RTClock();
  // to synchronize system time and RTC time
  bool SyncTime(); 

private:
  bool Open();
  bool Close();
  unsigned char  BCD2Dec( unsigned char bcdval );
  bool isTimeValid( unsigned char * const bcd_time );
  bool isTimeValid( struct tm * const date );

  bool Get9260TrueTime( struct tm * date );
  int fd;
  static const char * name;
};

#endif // _RTC_HPP
