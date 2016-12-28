#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <getopt.h>
#include <pthread.h>
#include <ctype.h>
#include <ftw.h>
#include <syslog.h>
#include <log.h>

#include "SoftDog.hpp"
#include "program_watch.hpp"
#include "watchdog.hpp"
#include "rtc.hpp"
#include "version.hpp"
#define MAX_FILE_NUM    (100)


#ifdef MEASURE_CALIBRATION
void run_calibrate_program( );
bool IsCalibrateTrue( );
#endif


void InitDaemon( void );
void initenv();

enum PROCESS_STATUS
{
    NO_EXIST,
    RUNNING,
    SLEEPING,
    DISK_SLEEP,
    STOPPED,
    TRACING_STOP,
    ZOMBIE,
    DEAD,
    OTHER,
};
///////////////////////////////////////////////////
// ѭ��ι���Ź�
//////////////////////////////////////////////////////
void * LoopFeedWatchdog( void *  );


void usage_print()
{
  printf( "%s\n (%s)\n", version, MAKEDATE );
  printf( "\nUsage: watchdog [OPTION]...\n" );
  printf( "\nTo init other programs and fresh watchdog devices at intervals.\n" );
  printf( "\nOptions:\n" );
  printf( "\t-h  --help			Output this help string\n" );
  printf( "\t-v  --version		Show version\n" );
  printf( "\t-f  --configfile filename  Set program config file\n" );
}

char * logfile = (char *)"/data1/watchdog.log";

//////////////////////////////////////////////////////////////////////
// ��������ĸ�λʱ��, 
// ע�⣺�����նˣ�ģ�飩������ĳһʱ�̸�λ���������ߣ�����ƶ��������վ�����������˲�����ĳЩʱ��������λ�ķ�ʽʵ�֡�
//       ͨ���ڿ���ʱҪ���ܶ๤��������ܿ�������ֻ�����02:10:00 �� 04:50:00��һ��ʱ���ڸ�λ
///////////////////////////////////////////////////////////////////////
inline int GetRebootSeconds()
{
  float const BEGIN_SECONDS = 2 * 60 * 60 + 10 * 60; // ����λʱ�����ʼʱ�� 2:10:00
  float const END_SECONDS = 4 * 60 * 60 + 50 * 60; // ����λʱ��ν���ʱ�� 4:50:00
			
  return (int)( BEGIN_SECONDS + ( END_SECONDS - BEGIN_SECONDS ) * ( rand() / ( RAND_MAX + 1.0 ) ));
}

///////////////////////////////////////////////////////////
// ���ܣ���������Ľ��̺Ż�ȡ�ý��̶�Ӧ�Ľ���״̬
////////////////////////////////////////////////////////////
enum PROCESS_STATUS GetPidState( int pid )
{
#define BUFLEN 64
#define LINUM 2
    char buffer[BUFLEN], state[2] = { 0xff, 0xff };
    FILE *proc_fs_p;
    int coln_count = 1, tmp = 0;
    if (snprintf(buffer, BUFLEN, "/proc/%d/status", pid) >= BUFLEN)
    {
        printf("buffer overflow detected\n");
        return NO_EXIST;
    }

    proc_fs_p = fopen(buffer , "r");
    if (!proc_fs_p)
    {
        printf("/proc open failed\n");
        return NO_EXIST;
    }

    while ( fgets( buffer, BUFLEN, proc_fs_p ) != NULL )
    {
        tmp++;
        if ( tmp != LINUM )
        {
            continue;
        }

        for ( int i = 0; i < BUFLEN; i++ )
        {
            if ( buffer[i] == ':' )
            {
                coln_count--;
            }
            else if ( !coln_count && !isblank( buffer[i] ) )
            {
                state[0] = buffer[i];
                state[1] = buffer[i + 1];
                break;
            }
        }
        break;
    }
    fclose( proc_fs_p );

    switch ( state[0] )
    {
        case 'R': return RUNNING;
        case 'W': return OTHER;
        case 'S': return SLEEPING;
        case 'T': return TRACING_STOP;
        case 'Z': return ZOMBIE;
        case 'D': return DISK_SLEEP;
        default:
              printf("unable to determine process state\n");
              return OTHER;
    }

    return OTHER; // unreachable
#undef BUFLEN
#undef LINUM
}

//////////////////////////////////////////////////////////////////
// ���ܣ� ��������롰D��״̬��disk sleep����Z��״̬��zombie������������ܱ�ɱ������ϵͳ���ܲ���ɥʧ
//         ��ʱ��Ҫ��λϵͳ�Ա�֤ϵͳ����������
// ע�⣺���̴���"D"��"Z"״̬���ܿ����������ֳ�����������ɵģ���˾�ڲ���EMC���ܲ����ǣ���һ�����ʳ��֣�
//       ����������������Ҫ��λ����
//       �жϳ����ܷ�ɱ��
///////////////////////////////////////////////////////////////////
void SaveSystem( int pid, char *name )
{
#define MAX_TRIES 3
	for ( int i = 0; i < MAX_TRIES; i ++ )
	{
		char cmd[50];
		sprintf( cmd, "/usr/bin/killall -9 %s", name );
		system( cmd );
		syslog( LOG_CONS|LOG_WARNING, "����%s�쳣�����Ź����临λ\n", name );
		sleep( 1 );
		if ( GetPidState( pid ) != DISK_SLEEP && GetPidState( pid ) != ZOMBIE )
		{
			return;
		}
	}

    syslog( LOG_CONS|LOG_WARNING, "����ɱ�����̣�ϵͳ��λ.\n" );
    system( "/sbin/reboot" );
    sleep( 20 );
    system( "/usr/bin/killall -9 watchdog" );
#undef MAX_TRIES
}

int Watch(const char *file, const struct stat *sb, int flag)
{
    struct process_t process = {0, 1, 0, ""};

    if ( flag == FTW_F )
    {
        FILE* fp = fopen(file, "rb");
        if(fp==NULL)
        {
            return 0;
        }
        fread(&process, sizeof(process_t), 1, fp);
        fclose( fp );

        struct sysinfo info ;
        if ( sysinfo( &info ) < 0 )
        {
            return 0;
        }

        if ( process.up_time + process.dead_seconds < info.uptime )
        {
            if( process.pid > 0 )
            {
				if ( GetPidState( process.pid ) == DISK_SLEEP || GetPidState( process.pid ) == ZOMBIE )
                {
                    //syslog( LOG_CONS|LOG_WARNING, "����%s�쳣�����̴���'D'״̬.\n", process.name);
					SaveSystem( process.pid, process.name );
                }
				else
				{
                	kill(process.pid, SIGKILL);
                	syslog( LOG_CONS|LOG_WARNING, "����%s�쳣�����Ź����临λ\n", process.name);
				}
            }
            remove(file);
        }
    }
    return 0;
}

int main( int argc, char *argv[], char *env[] )
{
	
  //  time_t boottime;
  int next_option;
  const char * short_options = "hvf:";

  const struct option long_options[] = {
    { "help",		0, NULL, 'h' },
    { "version",	0, NULL, 'v' },
    { "configfile",	1, NULL, 'f' },
    { NULL,		0, NULL, 0   },
  };

  char * configfile = NULL;

  for ( ;; ) {
    next_option = getopt_long( argc, argv,short_options, long_options, NULL );
    if ( next_option == -1 ) {
      break;
    }

    switch( next_option ) {
    case 'h':
      usage_print();
      exit( 1 );
      break;
    case 'f':
      if ( optarg != NULL ) {
	configfile = optarg;
      }
      else {
	usage_print();
	exit( -1 );
      }
      break;
    case 'v':
      printf( "%s\n Compiled on %s.\n", version, MAKEDATE );
      exit( 1 );
      break;
    default:
      usage_print();
      exit( 1 );
      break;
    }
  } // for ( ;; ) 


  //   /* �жϳ������������Ƿ���ȷ */
  //   if ( argc > 3) {
  //     usage_print();
  //     return -1;
  //   }

  //   if ( argc == 2 ) {
  //     if ( strcmp( "-v", argv[1] ) != 0 ) {
  //       usage_print();
  //       return -1;
  //     }
  //     printf( "%s\n (%s) \n", version, MAKEDATE );
  //     return 0;
  //   }

  //   if ( argc == 3 ) {
  //     if ( strcmp( "-f", argv[1] ) != 0 ) {
  //       usage_print();
  //       return -1;
  //     }
  //   }

//������Ź�����Ŀ¼
  char command [sizeof( SOFT_WATCH_PATH ) + 10];
  sprintf( command, "rm -rf %s", SOFT_WATCH_PATH );
  system( command );

  mkdir(SOFT_WATCH_PATH, S_IRWXU);

  class ProgramWatch progs;
  class RTClock rtc;

  progs.SetConfName( configfile );

  // to mask the SIGTERM signal
  signal( SIGTERM, SIG_IGN );

  progs.Init();

  InitDaemon();
 
  initenv();

  pthread_t thread;
  if ( pthread_create( &thread, NULL, LoopFeedWatchdog, NULL ) != 0 ) 
    {
      printf( "can not create thread\n" );
      exit(-1);
    }

  int i;
  for(  i = 0; i < 6; i ++ ) {
    if ( !rtc.SyncTime() ) {
      sleep(1);
    }
    else {
      break;
    }
  }

  if ( i == 3 ) {
    Log2File( logfile, "can not sync rtc time!\n" );
  }

#ifdef MEASURE_CALIBRATION
  run_calibrate_program( );
#endif

  progs.Run();

  Log2File( logfile, "system boot succeeded!\n" );

  //  boottime = time( NULL );

  int count = 0;
  bool isreboot = false;
  int reboot_seconds = 0;

  while( 1 ) {
    sleep(1);
    progs.Watch(); 

    if ( progs.isreboot ) {
      Log2File( logfile, "program crash, system must be reboot!\n" );
      // to reboot system
      for( i = 0; i < 30; i ++ ) {
	sleep(1);
      }
      system( "/sbin/reboot" );

    }
    
    if ( rebootinterval != 0 ) {
      if ( isreboot ) {
	if ( time( NULL ) % ( 24 * 60 * 60 ) > reboot_seconds )
		{
	  // to reboot system
	  for( i = 0; i < 30; i ++ ) {
	    sleep(1);
		}
	  system( "/sbin/reboot" );
	}
      }
      else {

	FILE *fp = fopen( "/proc/uptime", "r" );
	if ( fp != NULL ) {
	  double boottime;
	  
	  fscanf( fp, "%lf", &boottime );
	  if ( boottime > rebootinterval ) {
	    Log2File( logfile, "reboot time reached, system must be reboot!\n" );
		
		reboot_seconds = GetRebootSeconds();
	    isreboot = true;	    
	  }

	  fclose( fp );

	}
      }
    }

    count ++;
    if (count >= 60) {
      rtc.SyncTime();
      count = 0;
    }
  }

  // at normal state never reach here
  return 0;

}

void InitDaemon (void)
{
  struct sigaction act;
  int max_fd, i, ret;

  /*���е�1��fork,Ϊsetsid��׼�� */
  ret = fork ();

  if (ret < 0) {
    Log2File( logfile, "InitDaemon() fork failed!" );
    exit (1);
  }
  else if (ret != 0) 
    exit (0);

  /*����setsid��ʹ�ý��̾ɻỰ���̷��� */
  //  sleep( 600 );
  ret = setsid ();
  if (ret < 0) {
    Log2File( logfile, "InitDaemon() setsid failed!" );
    exit (1);
  }

  /*�����ź�SIGHUP */
  act.sa_handler = SIG_IGN;
  sigemptyset (&act.sa_mask);
  act.sa_flags = 0;
  sigaction (SIGHUP, &act, 0);

  /*
   *���е�2��fork��ʹ���̲����ǻỰ���̵���ͷ���̣���������ٴ�
   *�ն���Ϊ�Լ��Ŀ����ն�
   */
  ret = fork ();

  if (ret < 0) {
    Log2File( logfile, "InitDaemon() fork failed!" );
    exit (1);
  }
  else if (ret != 0)
    exit (0);

  /*�޸Ľ��̵ĵ�ǰĿ¼ */
  chdir ("/");

  /*������̵��ļ����� */
  umask (0);

  /*ʹ�ý�������ԭ���Ľ����飬�����ܷ���ԭ����������źŵĸ��� */
  setpgrp ();

  /*�رս������е��ļ������� */
  max_fd = sysconf (_SC_OPEN_MAX);
  for (i = 0; i < max_fd; i++)
    close (i);

  /*�򿪿��豸��ʹ�ú����������䲻������� */
  if ((i = open("/dev/null", O_RDWR)) >= 0) {
    while (0 <= i && i <= 2)
      i = dup(i);
    if (i >= 0)
      close(i);
  }

  /*   open ("/dev/null", O_RDWR); */
  /*   dup (1); */
  /*   dup (2); */

  /*����־ */
  //openlog(pathname, LOG_PID, facility);

  return;
}


void initenv()
{
  setenv( "HOME", "/home/et1000", 1 );
  setenv( "LD_LIBRARY_PATH", "/lib:/usr/lib:/home/et1000:/home/et1000/lib", 1 );
  setenv( "locale", g_locale_str, 1 );
  setenv( "cputype", g_cputype_str, 1 );
  setenv( "cssupport", g_cs_str, 1 );
  setenv( "area", g_area_str, 1 );
  setenv( "baudrate", g_baudrate_str, 1 );
  system( "mknod /dev/wfet1000clock c 254 0" );
  chdir( "/home/et1000" );
}



void * LoopFeedWatchdog( void * )
{
  printf( "in LoopFeedWatchdog()\n" );
  class Watchdog watchdog;
  watchdog.Open();
  watchdog.Feed();
  watchdog.Feed();
  watchdog.Feed();
    int count = 0;


  while( true )
    {
      printf( "in while() loop \n" );
      if(count == 4)
      {
        count = 0;
      }
      if(count == 0)
      {
        //����������Ź��ж�
        ftw(SOFT_WATCH_PATH, Watch, MAX_FILE_NUM);
      }
      count++;
      watchdog.Feed();
      usleep( 200000 );
      
    }
  return NULL;
}


#ifdef MEASURE_CALIBRATION
/////////////////////////////////////////////////////////////////////////////////////////
// for calibration requirement
//  if the writeable state of measure chip is true( 1 ),
//  calibration program must be launched, and nother program be launched;
// else, just do as normal
// Ϊ��У�����Ҫ, ������оƬ�����洢����д��־Ϊ"��д"ʱ, оƬ�������ó����������,
// ���Ҳ���������������, 
// ������оƬ��������д, ��������������
/////////////////////////////////////////////////////////////////////////////////////////
void run_calibrate_program( )
{
  if (IsCalibrateTrue( )) {
    int pid = fork();
    if ( pid < 0 ) {
      perror( " fork() failed " );
      exit( 1 );
    }
    else if ( pid == 0 ) {
      execl( "/home/et1000/Calibrations1000F", "Calibrations1000F", (char *)NULL );
      exit( 1 );
    }

    while ( 1 ) {
      sleep ( 1 );
      if (!IsCalibrateTrue( )){
      	system("killall -9 Calibrations1000F");
      	return;
      } 
    }
  }
}

bool IsCalibrateTrue( )
{
    FILE * fp = fopen( "/proc/wfet/tdk6513_state", "r" );
        if ( fp == NULL ) {
        return false;
    }
    char state[ 9 ] = { 0 };
    fread( &state[ 0 ], sizeof( char ), 9, fp );
    fclose( fp );
    int i = atoi( state );
    return ( i == 1 );
}

#endif 
