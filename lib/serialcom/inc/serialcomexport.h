/**************************************************************
* serialcomexport.h
* Copyright (C) 2016 CHANGSHA WILLFAR INFORMATION Ltd.
* Author: Shuiming Peng(pengshuiming@wasion.com)
* Date: 2016.12.16
**************************************************************/

#ifndef _SERIAL_COM_EXPORT_H_
#define _SERIAL_COM_EXPORT_H_

#ifndef _T_BYTE
#define _T_BYTE
typedef unsigned char   BYTE;
#endif

/*func return value*/
#define SUCCESS 1
#define FAILURE -1

/*最大端口数目*/
#define  MAX_COMS 12
#define  COM1     0
#define  COM2     1
#define  COM3     2
#define  COM4     3
#define  COM5     4
#define  COM6     5
#define  COM7     6
#define  COM8     7
#define  COM9     8
#define  COM10    9
#define  COM11    10
#define  COM12    11

/* 校验方式定义 */
#define  PARITY_ODD   0
#define  PARITY_EVEN  1
#define  PARITY_NONE  2

/*阻塞方式定义*/
#define  COM_BLOCK    0
#define  COM_NBLOCK   1
/*RTS CTS，流控制定义*/
#define  COM_RTSCTS_ENABLE   0
#define  COM_RTSCTS_DISABLE  1

/*数据位定义*/
#define  COM_BYTESIZE5  5
#define  COM_BYTESIZE6  6
#define  COM_BYTESIZE7  7
#define  COM_BYTESIZE8  8

/*停止位定义*/
#define  COM_STOPBITS1  0
#define  COM_STOPBITS15 1
#define  COM_STOPBITS2  2

#define  SC_HANDLER		void*

/*定义485控制的读写、自动控制*/
#define  STATUS_AUTO   1
#define  STATUS_READ   2
#define  STATUS_WRITE  3

typedef struct {
	unsigned char flag;	//有效性
	int baudrate;		//速率
	unsigned char serial_num;	//端口号enum COM_NO { COM1=0, COM2=1, ... };
	unsigned char parity;	//校验
	unsigned char sync_mode;	//同步方式
	unsigned char rtscts;	//流控制
	unsigned char bytesize;	//数据位长度
	unsigned char stopbits;	//停止位长度
} __attribute__ ((packed)) S_COMM_PARA;

#ifdef __cplusplus
extern "C" {
#endif

	/*创建或者销毁串口*/
	SC_HANDLER SC_SerialCreate(S_COMM_PARA scParam);
	bool SC_SerialDestroy(SC_HANDLER sc_handler);

	/*打开或者关闭串口 */
	bool SC_SerialOpen(SC_HANDLER sc_handler);
	bool SC_SerialClose(SC_HANDLER sc_handler);

	/*串口读写*/
    int SC_SerialRead(SC_HANDLER sc_handler, unsigned char *rbuff, int len);
    int SC_SerialWrite(SC_HANDLER sc_handler, const unsigned char *wbuff, int len);
    void SC_Flush(SC_HANDLER sc_handler);

	/*设置串口参数*/
    bool SC_ChangeBaudrate(SC_HANDLER sc_handler, int speed);
    bool SC_ChangePort(SC_HANDLER sc_handler, unsigned char com);

#ifdef __cplusplus
}
#endif

#endif /*_SERIAL_COM_EXPORT_H_*/
