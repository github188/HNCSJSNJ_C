/***************************************************************
*Serial communication functions
*Copyright (C) 2003 by WILLFAR.
*            【　　　　
*　　　◎■■■■】〓〓〓〓〓〓〓〓〓〓〓〓〓〓〉
*            【　
*Written by lilian,    2003.5.19
*Modified by lilian,   2003.5.28
***************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "logexport.h"
#include "serial.h"

CSerialCom *CSerialCom::m_pInstance = NULL;

//serial port device name in linux.
const char *CSerialCom::Serial_Port_Filename[MAX_COMS] = {
	"/dev/ttyS0",
	"/dev/ttyS1",
	"/dev/ttyS2",
	"/dev/ttyS3",
	"/dev/ttyS4",
	"/dev/ttyS5",
	"/dev/ttyS6",
	"/dev/ttyS7",
	"/dev/ttyS8",
	"/dev/ttyS9",
	"/dev/ttyS10",
	"/dev/ttyS11",
};

/*创建或者销毁串口 */
SC_HANDLER SC_SerialCreate(S_COMM_PARA scParam)
{
	CSerialCom* scHandler = new CSerialCom(scParam);
	return (SC_HANDLER)scHandler;
}

bool SC_SerialDestroy(SC_HANDLER sc_handler)
{
	if (sc_handler)
	{
		((CSerialCom*)sc_handler)->serial_close();
		delete (CSerialCom*)sc_handler;
		sc_handler = NULL;
	}

	return true;
}


/*打开或者关闭串口 */
bool SC_SerialOpen(SC_HANDLER sc_handler)
{
	if(!sc_handler)
	{
		LogE("sc_handler is NULL");
		return false;
	}

	return ((CSerialCom*)sc_handler)->serial_open();
}

bool SC_SerialClose(SC_HANDLER sc_handler)
{
	if(!sc_handler)
	{
		LogE("sc_handler is NULL");
		return false;
	}

	((CSerialCom*)sc_handler)->serial_close();
	return true;
}

/*串口读写*/
int SC_SerialRead(SC_HANDLER sc_handler, unsigned char *rbuff, int len)
{
	if (NULL == sc_handler) {
		LogE("CSerialCom::m_pInstance is NULL");
		return FAILURE;
	}

	int rlen = 0;
	/*等待数据*/
	if(((CSerialCom*)sc_handler)->wait_for_response(3000)) {
		rlen = ((CSerialCom*)sc_handler)->serial_read(rbuff, len);	/*读取数据*/
	} else {
		LogD("Reading data from serial failed!");
		return FAILURE;
	}

	return rlen;
}

int SC_SerialWrite(SC_HANDLER sc_handler, const unsigned char *wbuff, int len)
{
	if (NULL == sc_handler) {
		LogE("CSerialCom::m_pInstance is NULL");
		return FAILURE;
	}

	if (NULL == wbuff || len <= 0) {
		LogE("write data is invalid");
		return FAILURE;
	}

	int wlen = 0;
	wlen = ((CSerialCom*)sc_handler)->serial_write(wbuff, len);	/*写数据*/

	if (len != wlen) {
		LogD("Writing data into serial failed!");
		return FAILURE;
	}

	return wlen;
}

void SC_Flush(SC_HANDLER sc_handler)
{
	if (NULL == sc_handler) {
		LogE("sc_handler is NULL");
		return;
	}

	return ((CSerialCom*)sc_handler)->serial_flush();
}

/*设置串口参数*/
bool SC_ChangeBaudrate(SC_HANDLER sc_handler, int speed)
{
	if (NULL == sc_handler) {
		LogE("sc_handler is NULL");
		return FAILURE;
	}

	return ((CSerialCom*)sc_handler)->serial_baudrate(speed);
}

bool SC_ChangePort(SC_HANDLER sc_handler, unsigned char com)
{
	if (NULL == sc_handler) {
		LogE("sc_handler is NULL");
		return false;
	}

	((CSerialCom*)sc_handler)->change_port(com);
	return true;
}

/***************************************************
  默认串口1,初始化参数为1200N81,无校验，非阻塞模式
  关闭硬件流控制
 ***************************************************/
CSerialCom::CSerialCom(void)
{
	m_serial_fd = -1;	//serial port file descriptor
	m_para.serial_num = COM3;
	m_para.baudrate = 9600;
	m_para.parity = PARITY_EVEN;
	m_para.sync_mode = COM_BLOCK;
	m_para.rtscts = COM_RTSCTS_DISABLE;
	m_para.bytesize = COM_BYTESIZE8;
	m_para.stopbits = COM_STOPBITS1;
}

CSerialCom::CSerialCom(S_COMM_PARA m_p)
{
	m_para = m_p;
	m_para.sync_mode = COM_BLOCK;	//COM_NBLOCK
	m_para.rtscts = COM_RTSCTS_DISABLE;
	m_serial_fd = -1;
}

/************************************************
  { COM1=0, COM2=1, COM3=2, ... }
  初始化参数为1200N81,无校验，非阻塞模式
  关闭硬件流控制
 ************************************************/
CSerialCom::CSerialCom(unsigned char com)
{
	m_serial_fd = -1;	//serial port file descriptor
	m_para.serial_num = com;
	m_para.baudrate = 9600;
	m_para.parity = PARITY_EVEN;
	m_para.sync_mode = COM_BLOCK;
	m_para.rtscts = COM_RTSCTS_DISABLE;
	m_para.bytesize = COM_BYTESIZE8;	//8个数据位
	m_para.stopbits = COM_STOPBITS1;	//一个停止位
}

/************************************************
  关闭当前串口
 ************************************************/
CSerialCom::~CSerialCom()
{
	serial_close();
}

/**************************************************
  关闭串口
 ***************************************************/
void CSerialCom::serial_close(void)
{
	if (m_serial_fd != -1)	//serial_fd有效
	{
		close(m_serial_fd);	//cur_port_status==PORT_READY时serial_fd才有效
		m_serial_fd = -1;
	}
}

/*****************************************************************
  打开串口时按已有参数初始化速率、校验位、同步方式、数据位、流控制
  失败返回false, 成功返回true并填写串口文件描述符.
 *****************************************************************/
bool CSerialCom::serial_open(void)
{
	int fd = m_serial_fd;
	struct termios term;

	if (-1 != fd) {
		return true;
	}

	fd = open(CSerialCom::Serial_Port_Filename[int (m_para.serial_num)],
		  O_RDWR | O_NOCTTY);
	if (fd == -1)		//open failed
	{
		LogD("Openning serial port failed!");
		return false;
	}

	bzero(&term, sizeof(term));
	if (m_para.baudrate == 0) {
		m_para.baudrate = 1200;
	}

	int bps = tran_baudrate(m_para.baudrate);
	term.c_cflag |= CBAUD | CLOCAL | CREAD;	//control mode flags
	cfsetispeed(&term, bps);	//设置termios中存储的线路速度
	cfsetospeed(&term, bps);

	term.c_cflag &= ~CSIZE;
	switch (m_para.bytesize) {
	case COM_BYTESIZE5:
		term.c_cflag |= CS5;	//5 bits
		break;
	case COM_BYTESIZE6:
		term.c_cflag |= CS6;	//6 bits
		break;
	case COM_BYTESIZE7:
		term.c_cflag |= CS7;	//7 bits
		break;
	case COM_BYTESIZE8:
		term.c_cflag |= CS8;	//8 bits
		break;
	}
	term.c_cflag &= ~CSTOPB;	// 1 stop bit
	term.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);	//原始模式,不回显,忽略SIGINT,SIGQUIT,SIGSTP信号
	term.c_oflag &= ~OPOST;	//choosing raw output
	switch (m_para.rtscts) {
	case COM_RTSCTS_DISABLE:	//取消硬件流控制
		term.c_cflag &= ~CRTSCTS;
		break;
	case COM_RTSCTS_ENABLE:	//设定硬件流控制
		term.c_cflag |= CRTSCTS;
		break;
	default:
		term.c_cflag &= ~CRTSCTS;
		break;
	}
	term.c_iflag &= ~(IXON | IXOFF | IXANY);
	if (m_para.baudrate < 1200)
		term.c_cc[VTIME] = 6;
	else
		term.c_cc[VTIME] = 4;
	term.c_cc[VMIN] = 255;

	switch (m_para.parity) {
	case PARITY_ODD:
		term.c_cflag |= PARENB;	//enable checknum
		term.c_cflag |= PARODD;
		break;
	case PARITY_EVEN:
		term.c_cflag |= PARENB;	//enable checknum
		term.c_cflag &= ~PARODD;
		break;
	case PARITY_NONE:
		term.c_cflag &= ~PARENB;	//no checknum
		break;
	}
	tcflush(fd, TCIFLUSH);
	if (tcsetattr(fd, TCSANOW, &term) < 0) {
		LogD("Initializing serial failed!");
		close(fd);	//cur_port_status==PORT_READY时serial_fd才有效
		m_serial_fd = -1;
		return false;
	} else {
		m_serial_fd = fd;
		serial_sync_mode(m_para.sync_mode);
		serial_flush();
		return true;
	}
}

/***************************************************
  控制串口通讯校验位,
  操作失败返回false, 成功返回true.
  条件：已打开端口
 ***************************************************/
bool CSerialCom::serial_verify(unsigned char parity)
{
	struct termios term;

	if (m_serial_fd == -1)	//serial_fd无效
		return false;
	bzero(&term, sizeof(term));
	if (tcgetattr(m_serial_fd, &term) == FAILURE)	//获取终端参数
		return false;

	switch (parity) {
	case PARITY_ODD:
		term.c_cflag |= PARENB;	//enable checknum
		term.c_cflag |= PARODD;
		break;
	case PARITY_EVEN:
		term.c_cflag |= PARENB;	//enable checknum
		term.c_cflag &= ~PARODD;

		break;
	case PARITY_NONE:
		term.c_cflag &= ~PARENB;	//no checknum
		break;
	default:
		return false;
	}

	tcflush(m_serial_fd, TCIFLUSH);
	if (tcsetattr(m_serial_fd, TCSANOW, &term) < 0)
		return false;
	else {
		m_para.parity = parity;
		return true;
	}
}

/*********************************************
  更改指定串口的通讯速率,失败返回false
  条件：已打开端口
 **********************************************/
bool CSerialCom::serial_baudrate(int speed)
{
	struct termios term;

	if (m_serial_fd == -1)	//serial_fd无效
		return false;
	int bps = tran_baudrate(speed);
	if ((tcgetattr(m_serial_fd, &term) < 0) ||	//获取终端参数
	    (cfsetispeed(&term, bps) < 0) ||	//设置结构termios中存储的线路速度值
	    (cfsetospeed(&term, bps) < 0) ||
	    (tcsetattr(m_serial_fd, TCSANOW, &term) < 0)) {
		return false;
	}
	return true;
}

/*************************************************************
  说明：从指定串口读取数据
  返回：失败返回-1, 成功返回实际接收到的数据长度
  非阻塞方式无数据时立即返回-1, 并将errno置为EWOULDBLOCK
  条件：已打开端口
 **************************************************************/
int CSerialCom::serial_read(unsigned char *buf, int len)
{
	int read_len, trys = 1;

	if (m_serial_fd != -1)	//否则serial_fd无效
	{
	    retry_read:
		read_len = (int)(read(m_serial_fd, buf, (size_t) len));
		if (read_len < 0)	//read() error
		{
			if ((errno == EINTR) && (trys < 2)) {
				trys++;
				goto retry_read;
			} else
				return FAILURE;
		} else
			return read_len;
	} else
		return FAILURE;
}

/***************************************************************
  说明：向指定串口发送数据
  返回：失败返回-1, 成功返回实际发送出去的数据长度
  条件：已打开端口
 ****************************************************************/
int CSerialCom::serial_write(const unsigned char *buf, int len)
{
	int write_len, trys = 1;
	const unsigned char *p_buf = buf;

	if (m_serial_fd != -1)	//否则serial_fd无效
	{
	      retry_write:
		write_len = (int)(write(m_serial_fd, p_buf, (size_t) len));
		if (write_len < 0)	//write() error
		{
			if ((errno == EINTR) && (trys < 2)) {
				trys++;
				goto retry_write;
			} else
				return FAILURE;
		}
		tcdrain(m_serial_fd);
		return write_len;
	} else
		return FAILURE;
}

/**********************************************************
  设定串口阻塞模式, enum SYNC {COM_BLOCK=0, COM_NBLOCK=1}
  失败返回false.
  条件：已打开端口
 ***********************************************************/
bool CSerialCom::serial_sync_mode(unsigned char type)
{
	int flags;

	if (m_serial_fd == -1)	//serial_fd无效
	{
		return false;
	}

	switch (type) {
	case COM_BLOCK:	//阻塞
		if ((flags = fcntl(m_serial_fd, F_GETFL)) == FAILURE ||
		    fcntl(m_serial_fd, F_SETFL, flags & ~O_NDELAY) == FAILURE) {
			LogD("Error: fcntl()!");
			return false;
		}
		break;
	case COM_NBLOCK:
		if ((flags = fcntl(m_serial_fd, F_GETFL)) == FAILURE ||
		    fcntl(m_serial_fd, F_SETFL, flags | O_NDELAY) == FAILURE) {
			LogD("Error: fcntl()!");
			return false;
		}
		break;
	default:
		return false;
	}
	m_para.sync_mode = type;
	return true;
}

bool CSerialCom::serial_setvtime(unsigned char tenthsec)
{
	struct termios term;
	if (m_serial_fd == -1)	//serial_fd无效
		return false;
	if (tcgetattr(m_serial_fd, &term) == 0)	//获取终端参数
	{
		tenthsec = (tenthsec < 2) ? 2 : tenthsec;
		term.c_cc[VTIME] = tenthsec;
		if (tcsetattr(m_serial_fd, TCSANOW, &term) == 0) {
			return true;
		}
	}
	return false;
}

/***************************************************
  改变串口端口enum COM_NO {COM1,COM2,COM3,...}
  仍是同一端口无任何影响，而改变端口可能导致关闭原来的
  端口，需要用open()重新开启新的端口
 ***************************************************/
void CSerialCom::change_port(unsigned char com)
{
	if (m_para.serial_num == com)	//仍是同一端口
	{
		return;
	} else {
		serial_close();
		m_para.serial_num = com;
	}
}

/***************************************
  清空输入/输出缓冲区
 ***************************************/
void CSerialCom::serial_flush(void)
{
	if (m_serial_fd != -1)	//serial_fd有效
	{
		tcflush(m_serial_fd, TCIFLUSH);
		tcflush(m_serial_fd, TCOFLUSH);
	}
}

/**********************************************************
  增补函数，设定数据位长度5，6，7，8
  参数：bits=COM_BITS5，COM_BITS6，COM_BITS7，COM_BITS8
  失败返回false, 成功返回true.
  条件：已打开端口
 **********************************************************/
bool CSerialCom::change_bits(unsigned char bits)
{
	struct termios term;

	if (m_serial_fd == -1)	//serial_fd无效
		return false;
	bzero(&term, sizeof(term));
	if (tcgetattr(m_serial_fd, &term) == FAILURE)	//获取终端参数
		return false;
	term.c_cflag &= ~CSIZE;
	switch (bits) {
	case COM_BYTESIZE5:
		term.c_cflag |= CS5;	//5 bits
		break;
	case COM_BYTESIZE6:
		term.c_cflag |= CS6;	//6 bits
		break;
	case COM_BYTESIZE7:
		term.c_cflag |= CS7;	//7 bits
		break;
	case COM_BYTESIZE8:
		term.c_cflag |= CS8;	//8 bits
		break;
	default:
		return false;
	}
	tcflush(m_serial_fd, TCIFLUSH);
	if (tcsetattr(m_serial_fd, TCSANOW, &term) < 0) {
		LogD("Setting serial port bits failed!");
		return false;
	} else {
		m_para.bytesize = bits;
		return true;
	}
}

/***********************************************
  说明：增补函数，设定/取消硬件流控制RTSCTS
  条件：已打开端口
  参数：flag==COM_RTSCTS_DISABLE,取消硬件流控制，
  flag==COM_RTSCTS_ENABLE时,开启硬件流控制.
  返回: 失败返回false, 成功返回true
 ***********************************************/
bool CSerialCom::set_rtscts(unsigned char flag)
{
	struct termios term;

	if (m_serial_fd == -1)	//serial_fd无效
		return false;
	bzero(&term, sizeof(term));
	if (tcgetattr(m_serial_fd, &term) == FAILURE)	//获取终端参数
		return false;
	if (flag == COM_RTSCTS_DISABLE)	//取消硬件流控制
		term.c_cflag &= ~CRTSCTS;
	else if (flag == COM_RTSCTS_ENABLE)	//设定硬件流控制
		term.c_cflag |= CRTSCTS;
	else
		return false;
	if (tcsetattr(m_serial_fd, TCSANOW, &term) < 0) {
		LogD("Setting serial port rtscts failed!");
		return false;
	} else {
		m_para.rtscts = flag;
		return true;
	}
}

int CSerialCom::tran_baudrate(int speed)
{
	int bps;
	switch (speed) {
	case 300:
		bps = B300;
		break;
	case 600:
		bps = B600;
		break;
	case 1200:
		bps = B1200;
		break;
	case 2400:
		bps = B2400;
		break;
	case 4800:
		bps = B4800;
		break;
	case 9600:
		bps = B9600;
		break;
	case 19200:
		bps = B19200;
		break;
	case 38400:
		bps = B38400;
		break;
	case 57600:
		bps = B57600;
		break;
	case 115200:
		bps = B115200;
		break;
	default:
		bps = B1200;
		break;
	}
	return bps;
}

bool CSerialCom::wait_for_response(int milsec)
{
	static fd_set readfs;
	static struct timeval timeout;

	/* 等待数据返回并判断是否超时 */
	if (milsec == 0)
		return true;
	timeout.tv_usec = (milsec % 1000) * 1000;	/* 设定超时时间为milsec毫秒    */
	timeout.tv_sec = milsec / 1000;	/* 设定超时时间为sec秒      */
	FD_ZERO(&readfs);
	FD_SET(m_serial_fd, &readfs);
	if (milsec < 0) {
		if (select(m_serial_fd + 1, &readfs, NULL, NULL, NULL) <= 0) {
			return false;
		}
	} else {
		if (select(m_serial_fd + 1, &readfs, NULL, NULL, &timeout) <= 0) {
			return false;
		}
	}
	/* 读句柄集合里是否是fd句柄来数据 */
	if (FD_ISSET(m_serial_fd, &readfs) <= 0) {
		tcflush(m_serial_fd, TCIOFLUSH);
		return false;
	}
	return true;
}

int CSerialCom::SerialRW(BYTE * wbuff, int wlen, BYTE * rbuff, int milsec)
{
	int len = 0;

	if (wlen > 0) {
		len = serial_write(wbuff, wlen);	//写数据
		if (len != wlen) {
			LogD("Writing data into serial failed!");
			return FAILURE;
		}
	} else {
		if (wait_for_response(milsec))	//等待数据
		{
			len = serial_read(rbuff, 1024);	//读取数据
		} else {
			LogD("Reading data from serial failed!");
			return FAILURE;
		}
	}
	return len;
}
