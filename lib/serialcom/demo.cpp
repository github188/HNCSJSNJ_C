/**************************************************************
* Demo.h
* Copyright (C) 2016 CHANGSHA WILLFAR INFORMATION Ltd.
* Author: Shuiming Peng(pengshuiming@wasion.com)
* Date: 2016.12.16
**************************************************************/

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "logexport.h"
#include "serialcomexport.h"

int main(int argc, char** argv)
{
	LogD("Hello World!");
	unsigned char buff[256] = {0};
	int rlen = 0;
	int wlen = 0;
	int nCount = 5;

	S_COMM_PARA scParam = {0};
	scParam.serial_num = COM2;
	scParam.baudrate = 9600;
	scParam.parity = PARITY_EVEN;
	scParam.sync_mode = COM_BLOCK;
	scParam.rtscts = COM_RTSCTS_DISABLE;
	scParam.bytesize = COM_BYTESIZE8;
	scParam.stopbits = COM_STOPBITS1;

	SC_HANDLER scHandler = SC_SerialCreate(scParam);

	if (!scHandler) {
		LogE("Create Serial Failed!");
		return 0;
	}

	if (!SC_SerialOpen(scHandler)) {
		LogE("Open Serial Failed!");
		return 0;
	}

	while(nCount--) {
		rlen = SC_SerialRead(scHandler, buff, sizeof(buff));

		if (FAILURE != rlen) {
			LogD("rlen is %d Read buffer is %s!", rlen, buff);
		}

		rlen = 0;
		memset(buff, 0, sizeof(buff));
		sleep(1);
	}

	nCount = 5;
	memset(buff, 'A', 10);
	buff[10] = '\0';

	while(nCount--) {
		wlen = SC_SerialWrite(scHandler, buff, 10);
		if (FAILURE != wlen) {
			LogD("wlen is %d Write buffer is %s!", wlen, buff);
		}

		wlen = 0;
		sleep(1);
	}

	SC_SerialClose(scHandler);
	SC_SerialDestroy(scHandler);
	return 0;
}
