#pragma once

//����������vs2003�б���Ҫ����#include <winsock2.h>����ǰ��,�������ʾInitializeCriticalSectionAndSpinCountû�ж���Ĵ��󣬵���vc6.0�������������κεط�������,���������ĺ����#include <winsock2.h>����ǰ��,������ַǳ���ı������.


//#ifndef _WIN32_WINNT		// ����ʹ�� Windows NT 4 ����߰汾���ض����ܡ�
//#define _WIN32_WINNT 0x0500		//Ϊ Windows98 �� Windows 2000 �����°汾�ı�Ϊ�ʵ���ֵ��
//#endif


#pragma warning(disable:4996)
#pragma warning(disable:4311)
#pragma warning(disable:4312)

//for InitializeCriticalSectionAndSpinCount()
#ifndef _WIN32_WINNT		// ����ʹ�� Windows NT 4 ����߰汾���ض����ܡ�
#define _WIN32_WINNT 0x0500		//Ϊ Windows98 �� Windows 2000 �����°汾�ı�Ϊ�ʵ���ֵ��
#endif

#include <winsock2.h>
//windows.hͷ�ļ��������winsock2.h�����棬����������ִ���
#include <windows.h>
#include <MSWSock.h>


#pragma comment(lib,"ws2_32")   // Standard socket API.
#pragma comment(lib,"mswsock")  // AcceptEx, TransmitFile, etc,.
#pragma comment(lib,"shlwapi")  // UrlUnescape.

#include <Mmsystem.h> //for timeSetEvent()
#pragma comment (lib,"winmm.lib")

#include <stdio.h> //for sprintf()
#include <process.h> //for _beginthreadex()

#include <vector>
#include <map>
#include <string>




//���������vc6����ļ���û�ж��壬��Ҫ�Լ�����   
#define IOC_VENDOR 0x18000000   
#define _WSAIOW(x,y) (IOC_IN|(x)|(y))   
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR,12)

#define  SIO_KEEPALIVE_VALS  IOC_IN | IOC_VENDOR | 4
//�Զ���Ľṹ��,����TCP������



typedef struct TCP_KEEPALIVE1
{
	unsigned long onoff;
	unsigned long keepalivetime;
	unsigned long keepaliveinterval;
}
TCP_KEEPALIVE1,*PTCP_KEEPALIVE;

//�ص�����ָ��

typedef void tcp_receive_send_callback_fuc(SOCKET client,char* buf,unsigned long data_len);
typedef tcp_receive_send_callback_fuc* pTcpReceiveCallbackFun;
typedef tcp_receive_send_callback_fuc* pTcpSendCallbackFun;
typedef tcp_receive_send_callback_fuc* pTcpAcceptCallbackFun;

typedef unsigned long udp_receive_send_callback_fuc(char* remote_ip,unsigned short remote_port,char* buf,unsigned long data_len,SOCKET socket);
typedef udp_receive_send_callback_fuc* pUdpReceiveCallbackFun;
typedef udp_receive_send_callback_fuc* pUdpSendCallbackFun;


enum IO_OPERATION_TYPE //IO ��������
{
	IO_EXIT=0,
	IO_TCP_ACCEPT=1,
	IO_TCP_RECEIVE=2,
	IO_TCP_SEND=3,
	IO_UDP_RECEIVE=4,
	IO_UDP_SEND=5,
};

#pragma pack(1)
typedef struct IO_PER_HANDLE_STRUCT
{
	WSAOVERLAPPED operate_overlapped;//peer handle overlapped struct
	int operate_type ;//current context operation mode
}IO_PER_HANDLE_STRUCT;
#pragma pack()

const int ADDRESS_LENGTH = ((sizeof( struct sockaddr_in) + 16));

#pragma pack(1)
typedef struct TCP_ACCEPT_CONTEXT
{
	WSAOVERLAPPED operate_overlapped;//peer handle overlapped struct
	int operate_type ;        //current context operation mode
	SOCKET listen_socket;
	SOCKET accept_socket;
	char remoteAddressBuffer[ADDRESS_LENGTH*2];//���Զ�˵�ַ��Ϣ�Ļ�����

}TCP_ACCEPT_CONTEXT;
#pragma pack()

const int DEFAULT_TCP_RECEIVE_BUFFER_SIZE = 8192;
#pragma pack(1)
typedef struct TCP_RECEIVE_CONTEXT
{
	WSAOVERLAPPED operate_overlapped;//peer handle overlapped struct
	int operate_type ;//current context operation mode
	SOCKET socket;    
	WSABUF operate_buffer;   //ÿ�εĲ���������
	char receive_buffer[DEFAULT_TCP_RECEIVE_BUFFER_SIZE];
    
}TCP_RECEIVE_CONTEXT;
#pragma pack()

const int DEFAULT_TCP_SEND_BUFFER_SIZE = 4096;
#pragma pack(1)
typedef struct TCP_SEND_CONTEXT
{
	WSAOVERLAPPED operate_overlapped;//peer handle overlapped struct
	int operate_type ;//current context operation mode
	SOCKET socket;    
	WSABUF operate_buffer;   //ÿ�εĲ���������
	char send_buffer[DEFAULT_TCP_SEND_BUFFER_SIZE];
    
}TCP_SEND_CONTEXT;
#pragma pack()


//udp�ķ��ͺͽ��չ���һ�����͵�context
const int DEFAULT_UDP_BUFFER_SIZE = 4096;
#pragma pack(1)
typedef struct UDP_CONTEXT
{
	WSAOVERLAPPED operate_overlapped;
	int operate_type ;
	SOCKET socket;   
	WSABUF operate_buffer; 
	char data_buffer[DEFAULT_UDP_BUFFER_SIZE];
	struct sockaddr_in remote_address;
	int remote_address_len;  //��ֵ�̶�ΪADDRESS_LENGTH
    
}UDP_CONTEXT;
#pragma pack()



