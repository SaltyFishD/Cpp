#ifndef MYSERIAL_H
#define MYSERIAL_H

#include <Windows.h>
#include <iostream>
#include <thread>
#include <mutex>

class MySerial
{
public:
	
	MySerial():hCom(INVALID_HANDLE_VALUE),listenThreadClose(false),listenThreadID()
	{}

	
	~MySerial()
	{
		std::cout << "已调用析构函数" << std::endl;
		//CloseHandle(hCom);
		//listenThreadClose = true;
		//Sleep(10);
	}

	const int ESC = 27;
	const int SPACE = 16;

	std::mutex  dataMutex;

	int open(int com_num, int input_buffer = 1024, int ouput_buffer = 1024, int baud = 115200, 
		int byte_size = 8,int parity = 0, int stopbits = 0);
	int auto_open(int input_buffer = 1024, int ouput_buffer = 1024, int baud = 115200, 
		int byte_size = 8,int parity = 0, int stopbits = 0);
	int send(unsigned char *send_buf,unsigned long data_len) const;
	void openListenThread();   
	void receive(const int data_len);

private:
	HANDLE hCom;
	std::thread::id listenThreadID;
	bool listenThreadClose;
	int init(int input_buffer, int ouput_buffer, int baud, 
		int byte_size,int parity, int stopbits) const;
	int open_file(LPCWSTR file_name, int input_buffer, int ouput_buffer, int baud,
		int byte_size, int parity, int stopbits);
};

#endif
