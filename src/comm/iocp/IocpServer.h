#pragma once

#include "base_define.h"
#include "ContextManager.h"
#include "srvexception.h"
#include "logfile.h"

#include <map>	 //for map collection
using namespace std;


const int MAX_PROCESSER_NUMBERS = 10;
const int MAX_CONNECT_TIMEOUT = 1000; //���ӳ�ʱʱ��

class CIocpServer  
{
public:
	bool m_bStopServer;
	CIocpServer();
	virtual ~CIocpServer();
public:
	int InitServer(char* server_ip,unsigned short server_tcp_port,unsigned short server_udp_prot) throw(CSrvException);   //��ʼ��Ϊ������
	int InitTcpServer(char* server_ip,unsigned short server_tcp_port) throw(CSrvException);  //��ʼ��Ϊtcp������
	int InitUdpServer(char* server_ip,unsigned short server_udp_prot) throw(CSrvException);  //��ʼ��Ϊudp������

	int InitTcpClient(char* remote_ip,unsigned short remote_port) throw(CSrvException); //��ʼ��Ϊtcp�ͻ���
	int InitUdpClient(char* local_ip,unsigned short local_port) throw(CSrvException); //��ʼ��Ϊudp�ͻ���



	virtual int Run()throw(CSrvException);//����������
	int Stop()throw(CSrvException);//ֹͣ������

public://���ݷ�������ղ���
    int UdpSend(char* remote_ip,unsigned short remote_port,char* buf,unsigned long data_len,SOCKET socket=NULL)throw(CSrvException);
	virtual void UdpSendComplete(char* remote_ip,unsigned short remote_port,char* buf,unsigned long data_len,SOCKET socket=NULL){;}
	virtual void UdpReceiveComplete(sockaddr_in sockaddr,char* pByte,unsigned long data_len,SOCKET socket){;}
	
	int TcpSend(SOCKET client,char* buf,unsigned long data_len)throw(CSrvException);
	virtual void TcpAcceptComplete(SOCKET client,char* ip,unsigned short port){;};
	virtual void TcpSendComplete(SOCKET client,char* buf,unsigned long data_len){;}
	virtual void TcpReceiveComplete(SOCKET client,char* buf,unsigned long data_len){;};
	virtual void TcpCloseComplete( SOCKET client ) {;}
	virtual void TimerSecond() {} 

public:
	void SetExceptionStop(bool flag = true){m_bExceptionStop = flag;}
	SOCKET GetUdpServerSocket(){return m_hUdpServerSocket;}
	SOCKET GetTcpServerSocket(){return m_hTcpServerSocket;}
	SOCKET GetUdpClientSocket(){return m_hUdpClientSocket;}
	SOCKET GetTcpClientSocket(){return m_hTcpClientSocket;}
	
public:	//���ý���,������ɲ����Ļص�����,��ҪΪ�Ǽ̳�������
	void SetTcpCallbackPointer(pTcpReceiveCallbackFun pRevFuc,pTcpSendCallbackFun pSendFuc,pTcpAcceptCallbackFun pAcceptFuc){m_pTcpRevCallbackFuc = pRevFuc;m_pTcpSendCallbackFuc = pSendFuc;m_pTcpAcceptCallbackFuc = pAcceptFuc;}
	void SetUdpCallbackPointer(pUdpReceiveCallbackFun pRevFuc,pUdpSendCallbackFun pSendFuc){m_pUdpRevCallbackFuc = pRevFuc;m_pUdpSendCallbackFuc = pSendFuc;}
	bool IsValidSocket(SOCKET socket); //�ж�socket�Ƿ��ƺ��Ϸ���socket
private:	
	pTcpReceiveCallbackFun m_pTcpRevCallbackFuc;
	pTcpSendCallbackFun	 m_pTcpSendCallbackFuc;
	pTcpAcceptCallbackFun	 m_pTcpAcceptCallbackFuc;

	pUdpReceiveCallbackFun m_pUdpRevCallbackFuc; 
	pUdpSendCallbackFun m_pUdpSendCallbackFuc;

private:
	static UINT WINAPI WorkThread(LPVOID lpParma);
	static UINT WINAPI ListenThread(LPVOID lpParama); 
	static void PASCAL TimerProc(UINT wTimerID,UINT msg,DWORD dwUser,DWORD dw1,DWORD dw2);
protected:
	void CloseSocket(SOCKET socket);
	map<SOCKET,long> m_mapAcceptSockQueue;
private: //���ܺ���
	int  CreateThread();
	void CompleteIoProcess(IO_PER_HANDLE_STRUCT* pOverlapped,unsigned long data_len) throw(CSrvException);
	void ReleaseContext(IO_PER_HANDLE_STRUCT* pOverlapped); //�ͷ�context
	void ReleaseResource(); //�ͷŷ����˵���Դ	
	int  SaveConnectSocket(SOCKET socket); 
	
	
	void CloseAllSocket();//�ر�����ɶ˿��Ͻ��в���������socket
	int  SetHeartBeatCheck(SOCKET socket);
	void SetNewSocketParameters(SOCKET socket); 
	void ConnectTimeOutCheck();	//��ʱ���

	int AssociateAsUdpClient(SOCKET socket,char* local_ip,unsigned short local_port) throw(CSrvException); //���ⲿ������socket��������ɶ˿�,�����˵Ļ����ܽ�������,����,���ܽ�������
	int AssociateAsTcpClient(SOCKET socket,char* remote_ip,unsigned short remote_port)throw(CSrvException);

private: //UDP IO ����
	int PostUdpReceiveOp(SOCKET socket,unsigned short numbers) throw(CSrvException);
	int IssueUdpReceiveOp(SOCKET socket)throw(CSrvException);
	int IssueUdpReceiveOp(UDP_CONTEXT* pUdpContext);
	void CompleteUdpReceiveOp(UDP_CONTEXT* pUdpContext,unsigned long data_len) throw(CSrvException);

	int IssueUdpSendOp(struct sockaddr_in& remoteAddr,char* buf,unsigned long data_len,SOCKET socket) throw(CSrvException);
	void CompleteUdpSendOp(UDP_CONTEXT* pUdpContext,unsigned long data_len) throw(CSrvException);
   
	//unsigned short AddHead(char* data,unsigned short data_len,unsigned short pack_index);   //��udp���ݰ��м���֡ͷ
	//int CheckHead(char* data,unsigned short data_len,PPACKET_HEAD& pPacketHead); //��udp���ݰ�,�������Ƿ�Ϸ�
	
private: //TCP IO ����
    int PostTcpAcceptOp(unsigned short accept_numbers) throw(CSrvException);
	int IssueTcpAcceptOp() throw(CSrvException);
	int IssueTcpAcceptOp(TCP_ACCEPT_CONTEXT* pContext) throw(CSrvException);
    void CompleteTcpAcceptOp(TCP_ACCEPT_CONTEXT* pContext,unsigned long data_len) throw(CSrvException);
	void GetNewConnectInf(TCP_ACCEPT_CONTEXT* pContext);

	int IssueTcpReceiveOp(SOCKET socket,unsigned short times=1) throw(CSrvException);
	int IssueTcpReceiveOp(TCP_RECEIVE_CONTEXT* pContext) throw(CSrvException);
	void CompleteTcpReceiveOp(TCP_RECEIVE_CONTEXT* pUdpContext,unsigned long data_len);

	int IssueTcpSendOp(SOCKET client,char* buf,unsigned long data_len);
	void CompleteTcpSendOp(TCP_SEND_CONTEXT* pContext,unsigned long data_len);

protected:
	void PrintDebugMessage(CSrvException& e,bool log=true);
	void PrintDebugMessage(const char* message,int errorCode=-1,bool log = true);
private:
	bool m_bExceptionStop; //�����쳣ʱֹͣ����
	bool m_bInitOk;

	bool m_bServerIsRunning;
	int m_iWorkerThreadNumbers;
	unsigned long m_iTimerId;
	long m_iCurrentTime;
	SOCKET m_hTcpServerSocket;
    SOCKET m_hUdpServerSocket;
	SOCKET m_hTcpClientSocket;
	SOCKET m_hUdpClientSocket;
	struct sockaddr_in m_TcpServerAdd;
	struct sockaddr_in m_UdpServerAdd;
    HANDLE m_hCompletionPort;
	HANDLE m_hWorkThread[MAX_PROCESSER_NUMBERS];
	HANDLE m_hListenThread;
	HANDLE m_hPostAcceptEvent;
	CRITICAL_SECTION m_struCriSec;

private:
	LPFN_ACCEPTEX m_pfAcceptEx;				//AcceptEx������ַ
	LPFN_GETACCEPTEXSOCKADDRS m_pfGetAddrs;	//GetAcceptExSockaddrs�����ĵ�ַ

private:
	long m_iOpCounter;
private:
	CContextManager<UDP_CONTEXT>* m_pUdpContextManager;
	CContextManager<TCP_ACCEPT_CONTEXT>* m_pTcpAcceptContextManager;
	CContextManager<TCP_RECEIVE_CONTEXT>* m_pTcpReceiveContextManager;
	CContextManager<TCP_SEND_CONTEXT>* m_pTcpSendContextManager;

public:
	CLogFileEx*	m_pLogFile;
};
