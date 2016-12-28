#ifndef _WATCHDOG_HPP
#define _WATCHDOG_HPP

/////////////////////////////////////////////////////////////
// to manage watchdog device
// 
////////////////////////////////////////////////////////////
class Watchdog {
public:
  Watchdog();
  bool Feed(); //to reset watchdog device and prevent system reboot
  bool Open();
  bool Close();

private:
  static const char * name;
  int fd;

}; 

#endif //_WATCHDOG_HPP
