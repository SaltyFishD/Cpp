#include "stdafx.h"

//c++标准库
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <iomanip>

//windows平台库
#include <Windows.h>
#include <conio.h>        //获取键盘操作

//用户头文件
#include "mySerial.h"

using namespace std;
/*陀螺仪角度*/
double Angle[3] = { 0, 0, 0 };
float receiveX = 0;
float receiveY = 0;
float receiveAngle = 0;
/*************************************************
* 函数名称 ：                         open_file
* 函数功能 ：                         打开指定的串口并初始化
* 形参 ：							  file_name     串口号
input_buffer  接收缓冲区  默认1024
ouput_buffer  发送缓冲区  默认1024
baud			波特率		默认115200
byte_size		字节长		默认8
parity		检验位		默认无
stopbits		停止位		默认1
* 返回值 ：                           0             成功
-1            失败
****************************************************/
int MySerial::open_file(LPCWSTR file_name, int input_buffer, int ouput_buffer, int baud,
	int byte_size, int parity, int stopbits)
{
	hCom = CreateFile(file_name, GENERIC_READ | GENERIC_WRITE,  //允许读写
		0,          //此项必须为0
		NULL,       //no security attrs 
		OPEN_EXISTING,     //设置产生方式
		FILE_FLAG_OVERLAPPED,     //异步通信 FILE_FLAG_OVERLAPPED
		NULL);
	if (hCom == INVALID_HANDLE_VALUE)
	{
		std::cout << "串口打开失败" << std::endl;
		return -1;

	}
	int rec = init(input_buffer, ouput_buffer, baud, byte_size, parity, stopbits);
	if (rec == -1)
	{
		std::cout << "串口设置失败" << std::endl;
		return -1;
	}
	return 0;
}
/*************************************************
* 函数名称 ：                         open
* 函数功能 ：                         打开指定的串口
* 形参 ：							  com_num       串口编号
input_buffer  接收缓冲区   默认1024
ouput_buffer  发送缓冲区	 默认1024
baud          波特率		 默认115200
byte_size     字节长		 默认8
parity        检验位		 默认无
stopbits      停止位		 默认1
* 返回值 ：							  0				成功
-1            失败
****************************************************/
int MySerial::open(int com_num, int input_buffer, int ouput_buffer, int baud,
	int byte_size, int parity, int stopbits)
{
	const std::wstring str = L"COM";
	std::wstringstream stream;
	stream.clear();
	stream << com_num;
	std::wstring com_name;
	stream >> com_name;
	com_name = str + com_name;

	int rec = open_file(com_name.c_str(), input_buffer, ouput_buffer, baud, byte_size, parity, stopbits);
	if (rec == -1)
	{
		while (1)
		{
			int key = _getch();
			if (key == ESC)
			{
				this->~MySerial();
				std::exit(0);
			}
		}
	}
	return 0;
}
/*************************************************
* 函数名称 ：						auto_open
* 函数功能 ：						自动搜索串口打开
* 形参 ：							input_buffer  接收缓冲区	默认1024
ouput_buffer  发送缓冲区	默认1024
baud          波特率		默认115200
byte_size     字节长		默认8
parity        检验位		默认无
stopbits      停止位		默认1
* 返回值 ：							0			  成功
-1
* 注意项 ：                         串口号不能大于等于10
****************************************************/
int MySerial::auto_open(int input_buffer, int ouput_buffer, int baud,
	int byte_size, int parity, int stopbits)
{
	int i = 0;
	TCHAR port_name[25] = { 0 };
	unsigned char sz_port_name[25] = { 0 };
	long status = ERROR_SUCCESS;
	unsigned long dw_index = 0;
	unsigned long dw_long;
	unsigned long dw_sizeof_port_name;
	unsigned long type;
	HKEY hKey;
	LPCTSTR data_Set = L"HARDWARE\\DEVICEMAP\\SERIALCOMM\\";
	//LPCTSTR data_Set = L"HARDWARE\\DEVICEMAP\\SERIALCOMM";
	dw_long = sizeof(port_name);
	dw_sizeof_port_name = sizeof(sz_port_name);
	bool success_or_not = 0;

	long ret0 = RegOpenKeyEx(HKEY_LOCAL_MACHINE,  //打开主键名称 
		data_Set,       //打开的子键
		0,           //保留值  必须为0
		KEY_READ,               // 访问权限
		&hKey);                 //返回的串口句柄
								//打开一个制定的注册表键 成功返回ERROR_SUCCESS
	if (ret0 == ERROR_SUCCESS)   //调用成功
	{
		while (status == ERROR_SUCCESS)
		{
			status = RegEnumValue(hKey, dw_index++, port_name, &dw_long,
				NULL, &type, sz_port_name, &dw_sizeof_port_name); //读取键值
			if (status == ERROR_SUCCESS)
			{
				//open(LPCTSTR(&sz_port_name));
				int rec = open_file(LPCTSTR(sz_port_name), input_buffer, ouput_buffer, baud, byte_size, parity, stopbits);
				if (rec == 0)
				{
					success_or_not = 1;
					std::cout << "已打开：";
					for (unsigned long i = 0; i != dw_sizeof_port_name; ++i)
					{
						if (sz_port_name[i] == '\0')  //奇怪 得到的se_port_name是'C''\0''O''\0''M''\0''4' 这种形式的
							continue;
						std::cout << sz_port_name[i];
					}
					std::cout << std::endl;
				}
			}
			dw_index = sizeof(port_name);
			dw_sizeof_port_name = sizeof(sz_port_name);
		}
		RegCloseKey(hKey);
	}
	if (success_or_not == 0)
	{
		std::cout << "串口未打开！ QAQ" << std::endl
			<< "ESC键退出" << std::endl;
		while (1)
		{
			int key = _getche();

			if (key == ESC)
			{
				this->~MySerial();
				std::exit(0);
			}
			///*if (key == SPACE)
			//{
			//	std::cout << "串口未打开" << std::endl;
			//	break;
			//}*/
		}
	}
	return 0;
}
/*************************************************
* 函数名称 ：							init
* 函数功能 ：							设置串口参数
* 形参 ：								input_buffer  接收缓冲区	默认1024
ouput_buffer  发送缓冲区	默认1024
baud          波特率		默认115200
byte_size     字节长		默认8
parity        检验位		默认无
stopbits      停止位		默认1
* 返回值 ：								0             成功
-1		      失败
****************************************************/
int MySerial::init(int input_buffer, int ouput_buffer, int baud,
	int byte_size, int parity, int stopbits) const
{
	//输入缓冲区和输出缓冲区
	SetupComm(hCom, input_buffer, ouput_buffer);
	//设置事件驱动类型
	SetCommMask(hCom, EV_RXCHAR);

	COMMTIMEOUTS time_outs;
	//设定读超时
	time_outs.ReadIntervalTimeout = MAXWORD;
	time_outs.ReadTotalTimeoutConstant = 0;
	time_outs.ReadTotalTimeoutMultiplier = 0;
	//设定写超时
	time_outs.WriteTotalTimeoutConstant = 2000;
	time_outs.WriteTotalTimeoutMultiplier = 50;
	SetCommTimeouts(hCom, &time_outs);

	DCB dcb;
	GetCommState(hCom, &dcb);
	switch (baud)
	{
	case 2400:
	case 4800:
	case 9600:
	case 115200:
	case 38400:
		dcb.BaudRate = baud;
		break;
	default:
		std::cout << "波特率参数错误" << std::endl;
		return -1;
	}

	switch (byte_size)
	{
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
		dcb.ByteSize = byte_size;
		break;
	default:
		std::cout << "字节数参数错误" << std::endl;
		return -1;
	}

	switch (parity)
	{
	case 0:
		dcb.Parity = NOPARITY; //无校验
		break;
	case 1:
		dcb.Parity = ODDPARITY;   //奇校验
		break;
	case 2:
		dcb.Parity = EVENPARITY;   //偶检验
		break;
	case 3:
		dcb.Parity = MARKPARITY;  //标记校验
		break;
	default:
		std::cout << "校验位参数错误" << std::endl;
		return -1;
	}

	switch (stopbits)
	{
	case 0:
		dcb.StopBits = ONESTOPBIT;     //一位停止位
		break;
	case 1:
		dcb.StopBits = ONE5STOPBITS;    //1.5位停止位
		break;
	case 2:
		dcb.StopBits = TWOSTOPBITS;		//2位停止位
		break;
	default:
		std::cout << "停止位参数错误" << std::endl;
		return -1;
	}

	SetCommState(hCom, &dcb);    //写入参数

	PurgeComm(hCom, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR
		| PURGE_RXCLEAR); //清干净输入、输出缓冲区
	return 0;
}
/*************************************************
* 函数名称 ：							send
* 函数功能 ：							发送数据
* 形参 ：								senf_buf   发送数据
data_len   数据长度
* 返回值 ：								0          成功
-1		   失败
****************************************************/
int MySerial::send(unsigned char *send_buf, unsigned long data_len) const
{
	DWORD errorFlags;
	COMSTAT comStat;
	DWORD numOfBytesWritten;  //实际写入的数据字节数
	OVERLAPPED wOverlapped = { 0,0,0,0,NULL };  //
	memset(&wOverlapped, 0, sizeof(OVERLAPPED));
	wOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	ClearCommError(&wOverlapped, &errorFlags, &comStat);

	WriteFile(hCom, send_buf, data_len, &numOfBytesWritten, &wOverlapped);

	if (WaitForSingleObject(wOverlapped.hEvent, 20) == WAIT_OBJECT_0)  //等待的对象变为已通知状态
	{
		return 0;
	}
	return -1;
}
/*************************************************
* 函数名称 ：							openListenThread
* 函数功能 ：							打开监听线程 接收数据
* 形参 ：								无
* 返回值 ：							    无
****************************************************/

void MySerial::openListenThread()
{
	thread listenThread(&MySerial::receive, this, 33);
	listenThreadID = listenThread.get_id();
	listenThread.detach();
}
/*************************************************
* 函数名称 ：							receive
* 函数功能 ：							接收数据
* 形参 ：								rcv_buf	   接收数据缓存区
data_len   接收数据长度
* 返回值 ：								0		   成功
-1		   失败
****************************************************/
void MySerial::receive(const int data_len)
{
	unsigned char receiveBuffer[12] = { 0 };
	unsigned char receiveData[20] = { 0 };
	//输入缓冲区
	//接收到的正确数据
	DWORD commEvtMask = 0;
	OVERLAPPED rOverlapped;
	DWORD error;
	COMSTAT comStat;
	DWORD numOfBytesRead;  //实际读入的数据字节数

	memset(&rOverlapped, 0, sizeof(OVERLAPPED));
	rOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	while (!listenThreadClose)
	{
		ResetEvent(rOverlapped.hEvent);
		DWORD rec = WaitCommEvent(hCom, &commEvtMask, &rOverlapped);
		if (rec)
		{
			continue;
		}
		else
		{
			rec = ClearCommError(hCom, &error, &comStat);
			if (comStat.cbInQue == 0)
				continue;
		}

		WaitForSingleObject(rOverlapped.hEvent, INFINITE);  //
		rec = ClearCommError(hCom, &error, &comStat);
		if (comStat.cbInQue == 0)
			continue;

		ReadFile(hCom, receiveBuffer, comStat.cbInQue, &numOfBytesRead, &rOverlapped);

		int receiveFlag = 0;
		int j = 0;
		for (int i = 0; i != 12; ++i)
		{
			if (receiveBuffer[i] == 0xFF && receiveBuffer[i + 1] == 0xDD)
				receiveFlag = 1;
			if (receiveFlag == 1)
			{
				receiveData[j] = receiveBuffer[i];
				if (j == data_len + 2)
				{
					unsigned char checkNum = 0;
					for (int k = 0; k < data_len + 2; ++k)
					{
						checkNum += receiveData[k];
					}
					if (checkNum == receiveData[j])
					{
						receiveFlag = 2;
						break;
					}
					else
					{
						receiveFlag = 0;
						break;
					}
				}
				j++;
			}
		}
		PurgeComm(hCom, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);

		//int i = 0;
		//for (; i < 33; i++)
		//{
		//	//真是让人头大
		//	if (receiveBuffer[i] == 0x55 && receiveBuffer[i + 1] == 0x53/* && i + 11 < 34*/)
		//	{
		//		unsigned char mpu6050Data[11];
		//		double T;
		//		for (int j = 0; j < 11; j++)
		//		{
		//			mpu6050Data[j] = receiveBuffer[i++];
		//		}
		//		if (mpu6050Data[1] == 0x53)
		//		{
		//			Angle[0] = (short(mpu6050Data[3] << 8 | mpu6050Data[2])) / 32768.0 * 180;
		//			Angle[1] = (short(mpu6050Data[5] << 8 | mpu6050Data[4])) / 32768.0 * 180;
		//			Angle[2] = (short(mpu6050Data[7] << 8 | mpu6050Data[6])) / 32768.0 * 180;
		//			T = (short(mpu6050Data[9] << 8 | mpu6050Data[8])) / 340.0 + 36.25;
		//		}
		//	}
		//}

		for (int i = 0; i < 12; i++)
		{
			if (receiveBuffer[i] == 0xB6 && receiveBuffer[i + 1] == 0xAB && receiveBuffer[i + 10] == 0xBE && receiveBuffer[i + 11] == 0xA9)
			{
				unsigned char receData[12];
				for (int j = 0; j < 12; j++)
				{
					receData[j] = receiveBuffer[i++];
				}
				//校验
				if (receData[8] == ((receData[2] * 256 + receData[3] + receData[4] * 256 + receData[5] + receData[6] * 256 + receData[7] - 50000) & 0xff))
				{
					receiveX = receData[2] * 256 + receData[3] - 20000;
					receiveY = receData[4] * 256 + receData[5] - 20000;
					receiveAngle = (receData[6] * 256 + receData[7] - 10000) / 100.0;
					//unsigned char tmp = ((receData[2] * 256 + receData[3] + receData[4] * 256 + receData[5] + receData[6] * 256 + receData[7] - 50000) & 0xff);
					//if (tmp != receData[8])
					//{
					//	receiveX = 0;
					//	receiveY = 0;
					//	receiveAngle = 0;
					//}
				}
			}
		}

		memset(&receiveBuffer, 0, 12 * sizeof(unsigned char));
		memset(&receiveData, 0, 20 * sizeof(unsigned char));

		//cout << Angle[0] << "    " << Angle[1] << "    " << Angle[2] << endl;
		cout << receiveX << "    " << receiveY << "    " << receiveAngle << endl;

		Sleep(10);
	}
}