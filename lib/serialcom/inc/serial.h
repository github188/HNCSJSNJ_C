/***************************************************************
*Serial communication functions
*Copyright (C) 2003 by WILLFAR.
*            【　　　　
*　　　◎■■■■】〓〓〓〓〓〓〓〓〓〓〓〓〓〓〉
*            【　
*Written by lilian,    2003.5.19
*Modified by lilian,   2003.5.28
***************************************************************/
#ifndef _SERIAL_H
#define _SERIAL_H

#include <termios.h>
#include <stdio.h>
#include "serialcomexport.h"

/******************************************************************
 说明：16C550串口通讯类定义
 全局变量：无
 ******************************************************************/
class CSerialCom		/*串口类*/
{
public:
    CSerialCom(void);
    CSerialCom(S_COMM_PARA m_p);
    CSerialCom(unsigned char com);

	~CSerialCom();
	bool serial_open(void);
	void serial_close(void);
	void serial_flush(void);	//清输入输出缓冲
	bool serial_verify(unsigned char parity);
	bool serial_baudrate(int speed);
	int serial_read(unsigned char *buf, int len);
	int serial_write(const unsigned char *buf, int len);
	int SerialRW(BYTE * wbuff, int wlen, BYTE * rbuff, int milsec = 2000);
	bool serial_sync_mode(unsigned char type);
	void change_port(unsigned char com);	//改变对象使用端口
	bool change_bits(unsigned char bits);	//增补函数，设定数据位长度5，6，7，8
	bool set_rtscts(unsigned char flag);	//增补函数，设定/取消硬件流控制(RTSCTS)
	bool serial_setvtime(unsigned char tenthsec);

    bool wait_for_response(int milsec = 0);
	int tran_baudrate(int type);

	inline int getfd() {
		return m_serial_fd;
	}

    static CSerialCom *m_pInstance;

protected:
	int                     m_serial_fd;	//当前串口文件描述符
	S_COMM_PARA             m_para;
	static const char       *Serial_Port_Filename[MAX_COMS];
};

#endif
