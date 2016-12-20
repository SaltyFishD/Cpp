//c++��׼��
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <iomanip>

//windowsƽ̨��
#include <Windows.h>
#include <conio.h>        //��ȡ���̲���

//�û�ͷ�ļ�
#include "mySerial.h"

using namespace std;
/*�����ǽǶ�*/
double Angle[3] = {0, 0, 0};
/*************************************************
* �������� ��                         open_file
* �������� ��                         ��ָ���Ĵ��ڲ���ʼ��
* �β� ��							  file_name     ���ں�
									  input_buffer  ���ջ�����  Ĭ��1024
									  ouput_buffer  ���ͻ�����  Ĭ��1024
									  baud			������		Ĭ��115200
									  byte_size		�ֽڳ�		Ĭ��8
									  parity		����λ		Ĭ����
									  stopbits		ֹͣλ		Ĭ��1
* ����ֵ ��                           0             �ɹ�
									  -1            ʧ��
****************************************************/
int MySerial::open_file(LPCWSTR file_name, int input_buffer, int ouput_buffer, int baud,
	int byte_size, int parity, int stopbits) 
{
	hCom = CreateFile(file_name, GENERIC_READ | GENERIC_WRITE,  //�����д
		0,          //�������Ϊ0
		NULL,       //no security attrs 
		OPEN_EXISTING,     //���ò�����ʽ
		FILE_FLAG_OVERLAPPED,     //�첽ͨ�� FILE_FLAG_OVERLAPPED
		NULL);
	if (hCom == INVALID_HANDLE_VALUE)
	{
		std::cout << "���ڴ�ʧ��" << std::endl;
		return -1;

	}
	int rec = init(input_buffer, ouput_buffer, baud, byte_size, parity, stopbits);
	if (rec == -1)
	{
		std::cout << "��������ʧ��" << std::endl;
		return -1;
	}
	return 0;
}
/*************************************************
* �������� ��                         open
* �������� ��                         ��ָ���Ĵ���
* �β� ��							  com_num       ���ڱ��   
									  input_buffer  ���ջ�����   Ĭ��1024
									  ouput_buffer  ���ͻ�����	 Ĭ��1024
									  baud          ������		 Ĭ��115200
									  byte_size     �ֽڳ�		 Ĭ��8
									  parity        ����λ		 Ĭ����
									  stopbits      ֹͣλ		 Ĭ��1
* ����ֵ ��							  0				�ɹ�
									  -1            ʧ��
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
* �������� ��						auto_open
* �������� ��						�Զ��������ڴ�
* �β� ��							input_buffer  ���ջ�����	Ĭ��1024
									ouput_buffer  ���ͻ�����	Ĭ��1024
									baud          ������		Ĭ��115200
									byte_size     �ֽڳ�		Ĭ��8
									parity        ����λ		Ĭ����
									stopbits      ֹͣλ		Ĭ��1
* ����ֵ ��							0			  �ɹ�
									-1			 
* ע���� ��                         ���ںŲ��ܴ��ڵ���10   
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
	
	long ret0 = RegOpenKeyEx(HKEY_LOCAL_MACHINE,  //���������� 
		data_Set,       //�򿪵��Ӽ�
		0,           //����ֵ  ����Ϊ0
		KEY_READ,               // ����Ȩ��
		&hKey);                 //���صĴ��ھ��
	//��һ���ƶ���ע���� �ɹ�����ERROR_SUCCESS
	if (ret0 == ERROR_SUCCESS)   //���óɹ�
	{
		while (status == ERROR_SUCCESS)
		{
			status = RegEnumValue(hKey, dw_index++, port_name, &dw_long,
				NULL, &type, sz_port_name, &dw_sizeof_port_name); //��ȡ��ֵ
			if (status == ERROR_SUCCESS )
			{
				//open(LPCTSTR(&sz_port_name));
				int rec = open_file(LPCTSTR(sz_port_name), input_buffer, ouput_buffer, baud, byte_size, parity, stopbits);
				if (rec == 0)
				{
					success_or_not = 1;
					std::cout << "�Ѵ򿪣�";
					for (unsigned long i = 0; i != dw_sizeof_port_name; ++i)
					{
						if (sz_port_name[i] == '\0')  //��� �õ���se_port_name��'C''\0''O''\0''M''\0''4' ������ʽ��
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
	if(success_or_not == 0)
	{
		std::cout << "����δ�򿪣� QAQ" << std::endl
				  << "ESC���˳�" << std::endl;
		while(1)
		{ 
			int key = _getche();

			if (key == ESC)
			{ 
				this->~MySerial();
				std::exit(0);
			}			
			///*if (key == SPACE)
			//{
			//	std::cout << "����δ��" << std::endl;
			//	break;
			//}*/
		}
	}
	return 0;
}
/*************************************************
* �������� ��							init
* �������� ��							���ô��ڲ���
* �β� ��								input_buffer  ���ջ�����	Ĭ��1024
										ouput_buffer  ���ͻ�����	Ĭ��1024
										baud          ������		Ĭ��115200
										byte_size     �ֽڳ�		Ĭ��8
										parity        ����λ		Ĭ����
										stopbits      ֹͣλ		Ĭ��1
* ����ֵ ��								0             �ɹ�
										-1		      ʧ��
****************************************************/
int MySerial::init(int input_buffer, int ouput_buffer, int baud , 
	int byte_size ,int parity , int stopbits ) const
{
	//���뻺���������������
	SetupComm(hCom, input_buffer, ouput_buffer);
	//�����¼���������
	SetCommMask(hCom, EV_RXCHAR);

	COMMTIMEOUTS time_outs;
	//�趨����ʱ
	time_outs.ReadIntervalTimeout = MAXWORD;
	time_outs.ReadTotalTimeoutConstant = 0;
	time_outs.ReadTotalTimeoutMultiplier = 0;
	//�趨д��ʱ
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
		std::cout << "�����ʲ�������" << std::endl;
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
		std::cout << "�ֽ�����������" << std::endl;
		return -1;
	}
	
	switch (parity)
	{
	case 0:
		dcb.Parity = NOPARITY; //��У��
		break;
	case 1:
		dcb.Parity = ODDPARITY;   //��У��
		break;
	case 2:
		dcb.Parity = EVENPARITY;   //ż����
		break;
	case 3:
		dcb.Parity = MARKPARITY;  //���У��
		break;
	default:
		std::cout << "У��λ��������" << std::endl;
		return -1;
	}

	switch (stopbits)
	{
	case 0:
		dcb.StopBits = ONESTOPBIT;     //һλֹͣλ
		break;
	case 1:
		dcb.StopBits = ONE5STOPBITS;    //1.5λֹͣλ
		break;
	case 2:
		dcb.StopBits = TWOSTOPBITS;		//2λֹͣλ
		break;
	default:
		std::cout << "ֹͣλ��������" << std::endl;
		return -1;
	}

	SetCommState(hCom, &dcb);    //д�����

	PurgeComm(hCom, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR
		| PURGE_RXCLEAR); //��ɾ����롢���������
	return 0;
}
/*************************************************
* �������� ��							send
* �������� ��							��������
* �β� ��								senf_buf   ��������
										data_len   ���ݳ���
* ����ֵ ��								0          �ɹ�
										-1		   ʧ��
****************************************************/
int MySerial::send(unsigned char *send_buf,unsigned long data_len) const
{
	DWORD errorFlags;
	COMSTAT comStat;
	DWORD numOfBytesWritten;  //ʵ��д��������ֽ���
	OVERLAPPED wOverlapped = { 0,0,0,0,NULL };  //
	memset(&wOverlapped, 0, sizeof(OVERLAPPED));
	wOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	ClearCommError(&wOverlapped, &errorFlags, &comStat);
	
	WriteFile(hCom, send_buf, data_len, &numOfBytesWritten, &wOverlapped);

	if (WaitForSingleObject(wOverlapped.hEvent, 20) == WAIT_OBJECT_0)  //�ȴ��Ķ����Ϊ��֪ͨ״̬
	{
		return 0;
	}
	return -1;
}
/*************************************************
* �������� ��							openListenThread
* �������� ��							�򿪼����߳� ��������
* �β� ��								��
* ����ֵ ��							    ��
****************************************************/

void MySerial::openListenThread()
{
	thread listenThread(&MySerial::receive,this,33);
	listenThreadID = listenThread.get_id();
	listenThread.detach();
}
/*************************************************
* �������� ��							receive
* �������� ��							��������
* �β� ��								rcv_buf	   �������ݻ�����
										data_len   �������ݳ���
* ����ֵ ��								0		   �ɹ�
										-1		   ʧ��
****************************************************/
void MySerial::receive(const int data_len)
{
	unsigned char receiveBuffer[33] = { 0 };
	unsigned char receiveData[20] = { 0 };
	  //���뻺����
     //���յ�����ȷ����
	DWORD commEvtMask = 0;
	OVERLAPPED rOverlapped;
	DWORD error;
	COMSTAT comStat;
	DWORD numOfBytesRead;  //ʵ�ʶ���������ֽ���

	memset(&rOverlapped, 0, sizeof(OVERLAPPED));
	rOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	while (!listenThreadClose)
	{
		ResetEvent(rOverlapped.hEvent);
		DWORD rec = WaitCommEvent(hCom, &commEvtMask, &rOverlapped);
		if (!rec)
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
		for (int i = 0; i != 100; ++i)
		{
			if (receiveBuffer[i] == 0xFF && receiveBuffer[i+1] == 0xDD)
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

		int i = 0;
		for (; i < 33; i++)
		{
			//��������ͷ��
			if (receiveBuffer[i] == 0x55 && receiveBuffer[i + 1] == 0x53/* && i + 11 < 34*/)
			{
				unsigned char mpu6050Data[11];
				double T;
				for (int j = 0; j < 11; j++)
				{
					mpu6050Data[j] = receiveBuffer[i++];
				}
				if (mpu6050Data[1] == 0x53)
				{
					Angle[0] = (short(mpu6050Data[3] << 8 | mpu6050Data[2])) / 32768.0 * 180;
					Angle[1] = (short(mpu6050Data[5] << 8 | mpu6050Data[4])) / 32768.0 * 180;
					Angle[2] = (short(mpu6050Data[7] << 8 | mpu6050Data[6])) / 32768.0 * 180;
					T = (short(mpu6050Data[9] << 8 | mpu6050Data[8])) / 340.0 + 36.25;
				}
			}
		}
		//memset(&receiveBuffer, 0, 100 * sizeof(unsigned char));
		//memset(&receiveData, 0, 20 * sizeof(unsigned char));
	}
}