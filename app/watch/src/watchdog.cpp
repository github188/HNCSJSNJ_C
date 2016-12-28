#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#include "watchdog.hpp"

const char * Watchdog::name = "/dev/wfet1000wdog";

Watchdog::Watchdog()
{


}


// to open Watchdog device and to active watchdog device 
bool Watchdog::Open()
{
  int retval;
  char wBuf[2];

  fd = open( name, O_RDWR ); 
  
  if ( fd < 0 ) {
    fd = 0;
    return false;
  }

  wBuf[ 0 ] = 0x00;
  wBuf[ 1 ] = 0x80;
  retval = write( fd, wBuf, 2 );
  if( retval != 2 ) {
//     log_err( "watchdog device write() error! " );
//     exit(1);
    return false;
  }

  return true;
}


bool Watchdog::Close()
{

  if ( fd > 0 ) {
    close( fd );
  }

  return true;
}


// to "feed watchdog" in case of system reboot
bool Watchdog::Feed()
{
	unsigned char wBuf[ 2 ];
	int retval;
	
	wBuf[ 0 ] = 0x00;
	wBuf[ 1 ] = 0x80;
	retval=write( fd, wBuf, 2 );
	if(retval!=2) {
 	  //log_err( "resetWatchdog(): write failed! " );
	 // exit(1);
	  return false;
	}

	return true;
}



