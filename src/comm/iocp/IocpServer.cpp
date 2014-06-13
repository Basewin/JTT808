
#include "stdafx.h"
#include "IocpServer.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CIocpServer::CIocpServer()
{
	m_pLogFile = new CLogFileEx(".\\SrvLog", CLogFileEx :: DAY);
	try
	{
	    m_pTcpRevCallbackFuc = NULL; 
		m_pTcpSendCallbackFuc = NULL;
		m_pTcpAcceptCallbackFuc = NULL ; 
	    m_pUdpRevCallbackFuc = NULL; 
	    m_pUdpSendCallbackFuc = NULL;

		m_bExceptionStop = false;
		m_bInitOk = true;
		m_bStopServer = false;
		m_bServerIsRunning = false;
		m_iWorkerThreadNumbers = 2;
		m_iTimerId = 0;
		m_hTcpServerSocket = NULL;
		m_hUdpServerSocket = NULL;

	    m_hTcpClientSocket = NULL;
	    m_hUdpClientSocket = NULL;

		ZeroMemory(&m_TcpServerAdd,sizeof(struct sockaddr_in));
		ZeroMemory(&m_UdpServerAdd,sizeof(struct sockaddr_in));

		m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE,NULL,0,0);
		if(m_hCompletionPort == NULL)
		{
			PrintDebugMessage("��ɶ˿ڶ��󴴽�ʧ��,�����޷�����.");
		}

		for(int i=0;i<MAX_PROCESSER_NUMBERS;i++)
			m_hWorkThread[i] = NULL;

		m_hListenThread = NULL;
		m_hPostAcceptEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
		
		InitializeCriticalSectionAndSpinCount(&m_struCriSec,4000);

		m_pfAcceptEx = NULL;				  //AcceptEx������ַ
		m_pfGetAddrs = NULL;	//GetAcceptExSockaddrs�����ĵ�ַ

		m_pUdpContextManager = NULL;
		m_pTcpAcceptContextManager = NULL; //�����ڴ�ع������
		m_pTcpReceiveContextManager = NULL; //���ݽ����ڴ�ع������
		m_pTcpSendContextManager = NULL;//���ݷ����ڴ�ع������



		m_iOpCounter = 0;

		WSADATA wsData;
		if(WSAStartup(MAKEWORD(2, 2), &wsData)!=0)
			PrintDebugMessage("��ʼ��sockte�����.");
	}
	catch(CSrvException& e)
	{
		m_bInitOk = false;
		PrintDebugMessage(e);
		//if(m_bExceptionStop)
		//	throw e;
	}
	catch(...)
	{
		m_bInitOk = false;
		string message("CIocpServer ���캯������δ֪�쳣.");
		PrintDebugMessage(message.c_str());
		//if(m_bExceptionStop)
		//	throw message;
	}
}

CIocpServer::~CIocpServer()
{
	try
	{
		if(m_bServerIsRunning)
			this->Stop(); //ֹͣ������

		delete m_pLogFile;
		m_pLogFile = NULL;

		m_pTcpRevCallbackFuc = NULL; 
		m_pTcpSendCallbackFuc = NULL;
		m_pTcpAcceptCallbackFuc = NULL ; 
		m_pUdpRevCallbackFuc = NULL; 
		m_pUdpSendCallbackFuc = NULL;

		m_bInitOk = true;
		m_bStopServer = false;
		m_bServerIsRunning = false;
		m_iWorkerThreadNumbers = 0;
		m_iTimerId = 0;
		m_hTcpServerSocket = NULL;
		m_hUdpServerSocket = NULL;

		m_hTcpClientSocket = NULL;
		m_hUdpClientSocket = NULL;

		ZeroMemory(&m_TcpServerAdd,sizeof(struct sockaddr_in));
		ZeroMemory(&m_UdpServerAdd,sizeof(struct sockaddr_in));

		CloseHandle(m_hCompletionPort);
		for(int i=0;i<MAX_PROCESSER_NUMBERS;i++)
			m_hWorkThread[i] = NULL;

		m_hListenThread = NULL;

		CloseHandle(m_hPostAcceptEvent);
		m_hPostAcceptEvent = NULL;

		DeleteCriticalSection(&m_struCriSec);

		m_pfAcceptEx = NULL;				//AcceptEx������ַ
		m_pfGetAddrs = NULL;	//GetAcceptExSockaddrs�����ĵ�ַ

		m_pUdpContextManager = NULL;
		m_pTcpAcceptContextManager = NULL; //�����ڴ�ع������
		m_pTcpReceiveContextManager = NULL; //���ݽ����ڴ�ع������
		m_pTcpSendContextManager = NULL;//���ݷ����ڴ�ع������

		m_iOpCounter = 0;

		WSACleanup();
	}
	catch(CSrvException& e)
	{
		PrintDebugMessage(e);
		//if(m_bExceptionStop)
			//throw e;
	}
	catch(...)
	{
		string message("~CIocpServer ����δ֪�쳣.");
		PrintDebugMessage(message.c_str());
		//if(m_bExceptionStop)
			//throw message;
	}
}
int CIocpServer::InitServer(char* ip,unsigned short tcp_port,unsigned short udp_port)
{
	BOOL rc = FALSE;
	try
	{
		if( ip == NULL)
			throw CSrvException("������ip ��ַ�Ƿ�.",-1);
		if(tcp_port == udp_port)
			throw CSrvException("tcp �������˿ںź�udp �������˿ں��ظ�.",-1);

		if(InitTcpServer(ip,tcp_port)==FALSE)
			throw CSrvException("tcp ��������ʼ��ʧ��.",-1);
		if(InitUdpServer(ip,udp_port)==FALSE) //��ʼ��������
			throw CSrvException("udp ��������ʼ��ʧ��.",-1);

		rc = TRUE;
	}
	catch(CSrvException& e)
	{
		PrintDebugMessage(e);
		//if(m_bExceptionStop)
		//	throw e;
	}
	catch(...)
	{
		string message("InitServer ����δ֪�쳣.");
		PrintDebugMessage(message.c_str());
		if(m_bExceptionStop)
			throw message;
	}

	return rc;
}

//���������׽���
int CIocpServer::InitTcpServer(char* ip,unsigned short tcp_port)//��ʼ��tcp������
{
    BOOL rc = FALSE;
	try
	{
		int errorCode = 1;
		if(ip==NULL)
		{
			throw CSrvException("�Ƿ���tcp ip��ַ.",-1);
		}
		if(m_hTcpServerSocket)
		{
			throw CSrvException("�Ѿ���socket����Ϊtcp������ģʽ.",-1);
		}
		//�����׽���
		m_hTcpServerSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if(m_hTcpServerSocket == INVALID_SOCKET)
		{
			throw CSrvException("�����tcp socket�׽���.",-1);
		}
		//��accetpex����Ϊ�첽������ģʽ
		ULONG ul = 1;
		errorCode = ioctlsocket(m_hTcpServerSocket, FIONBIO, &ul);
		if(SOCKET_ERROR == errorCode)
		{
			throw CSrvException("Set listen socket to FIONBIO mode error.",WSAGetLastError());
		}

		//����Ϊsocket����,��ֹ������������˿��ܹ������ٴ�ʹ�û������Ľ���ʹ��
		int nOpt = 1;
		errorCode = setsockopt(m_hTcpServerSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&nOpt, sizeof(nOpt));
		if(SOCKET_ERROR == errorCode)
		{
			throw CSrvException("Set listen socket to SO_REUSEADDR mode error.",WSAGetLastError());
		}

		//�ر�ϵͳ�Ľ����뷢�ͻ�����
		int nBufferSize = 0;
		setsockopt(m_hTcpServerSocket,SOL_SOCKET,SO_SNDBUF,(char*)&nBufferSize,sizeof(int));
		nBufferSize = DEFAULT_TCP_RECEIVE_BUFFER_SIZE;
		setsockopt(m_hTcpServerSocket,SOL_SOCKET,SO_RCVBUF,(char*)&nBufferSize,sizeof(int)); 

		unsigned long dwBytes = 0;
		GUID guidAcceptEx = WSAID_ACCEPTEX;
		if (NULL == m_pfAcceptEx)
		{
			errorCode = WSAIoctl(m_hTcpServerSocket,SIO_GET_EXTENSION_FUNCTION_POINTER, &guidAcceptEx,sizeof(guidAcceptEx)
				, &m_pfAcceptEx, sizeof(m_pfAcceptEx), &dwBytes, NULL, NULL);
		}
		if (NULL == m_pfAcceptEx || SOCKET_ERROR == errorCode)
		{
			throw CSrvException("Invalid fuction pointer.",WSAGetLastError());
		}

		//����GetAcceptExSockaddrs����
		GUID guidGetAddr = WSAID_GETACCEPTEXSOCKADDRS;
		if (NULL == m_pfGetAddrs)
		{
			errorCode = WSAIoctl(m_hTcpServerSocket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidGetAddr, sizeof(guidGetAddr)
				, &m_pfGetAddrs, sizeof(m_pfGetAddrs), &dwBytes, NULL, NULL);
		}
		if (NULL == m_pfGetAddrs || SOCKET_ERROR == errorCode)
		{
			throw CSrvException("Invalid fuction pointer.",WSAGetLastError());
		}

		//����������ַ��Ϣ
		m_TcpServerAdd.sin_family = AF_INET;
		m_TcpServerAdd.sin_addr.s_addr = inet_addr(ip);
		m_TcpServerAdd.sin_port = htons(tcp_port);

		//��׽��ֺͷ�������ַ
		errorCode = bind(m_hTcpServerSocket,(struct sockaddr*)&m_TcpServerAdd,sizeof(m_TcpServerAdd));
		if(errorCode == SOCKET_ERROR)
		{
			throw CSrvException("Error socket bind operation.",WSAGetLastError());
		}
		//�Ѽ���socket����ɶ˿ڰ
		if(NULL == CreateIoCompletionPort((HANDLE)m_hTcpServerSocket,m_hCompletionPort,m_hTcpServerSocket,0))
		{
			throw CSrvException("Invalid completion port handle.",WSAGetLastError());
		}
		//������������ʱ��os���������¼�
		WSAEventSelect(m_hTcpServerSocket,m_hPostAcceptEvent,FD_ACCEPT); //���¼�����ϵͳ����
		//���ü����ȴ����
		errorCode = listen(m_hTcpServerSocket,5);//�ڶ����������������Ӷ�������ĵȴ������Ŀ.���������server��Ҫ�����ֵ
		if(errorCode)
		{
			throw CSrvException("Error socket lister operation.",WSAGetLastError());
		}
	    
		if(m_pTcpAcceptContextManager==NULL)
			m_pTcpAcceptContextManager = new CContextManager<TCP_ACCEPT_CONTEXT>(256); //�����ڴ�ع������
		if(m_pTcpReceiveContextManager==NULL)
			m_pTcpReceiveContextManager = new CContextManager<TCP_RECEIVE_CONTEXT>(512); //���ݽ����ڴ�ع������
		if(m_pTcpSendContextManager==NULL)
			m_pTcpSendContextManager = new CContextManager<TCP_SEND_CONTEXT>(512);//���ݷ����ڴ�ع������

		PostTcpAcceptOp(32);//Ͷ�����Ӳ�������ɶ˿�,Ӧ���߲�����,����ʼ�׶���32�������ڵȴ�client�Ľ���,�ڹر�client��ʱ����ɶ˿ڽ����ͷ���Ӧ��context

		//printf("tcp ��������ʼ���ɹ�.\n");
		return TRUE;
	}
	catch(CSrvException& e)
	{
		m_bInitOk = false;
		this->PrintDebugMessage(e);
		if(m_bExceptionStop)
			throw e;
	}
	catch(...)
	{
		m_bInitOk = false;
		string message("InitTcpServer ����δ֪�쳣.");
		PrintDebugMessage(message.c_str());
		//if(m_bExceptionStop)
			//throw message;
	}
	return FALSE;
}
int CIocpServer::InitUdpServer(char* ip,unsigned short udp_prot)//��ʼ��udp������
{
	try
	{
		if(!m_bInitOk)
            PrintDebugMessage("��������ʼ��ʧ��.");

		int errorCode = 1;
		if(ip==NULL)
			PrintDebugMessage("������ip��ַ����Ϊ��.");

		if(m_hUdpServerSocket)
			PrintDebugMessage("�Ѿ���socket��ʼ��Ϊudp������ģʽ.");

		//���������׽���
		m_hUdpServerSocket = WSASocket(AF_INET,SOCK_DGRAM,IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if(m_hUdpServerSocket == INVALID_SOCKET)
			PrintDebugMessage("��ʼ��udp�����׽��־��ʧ��.",WSAGetLastError());

		//��receiveform��sendto����Ϊ�첽������ģʽ,��Ϊ��WSAStartup��ʼ��Ĭ����ͬ��
		ULONG ul = 1;
		errorCode = ioctlsocket(m_hUdpServerSocket, FIONBIO, &ul);
		if(SOCKET_ERROR == errorCode)
		{
			PrintDebugMessage("Set listen socket to FIONBIO mode error.");
		}
		
		//����Ϊ��ַ���ã��ŵ����ڷ������رպ������������		
		int nOpt = 1;
		errorCode = setsockopt(m_hUdpServerSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&nOpt, sizeof(nOpt));
		if(SOCKET_ERROR == errorCode)
		{
			PrintDebugMessage("Set listen socket to SO_REUSEADDR mode error.");
		}
		
		//�رս����뷢�ͻ�����,ֱ��ʹ���û��ռ仺����
		int nBufferSize = 0;
		setsockopt(m_hUdpServerSocket,SOL_SOCKET,SO_SNDBUF,(char*)&nBufferSize,sizeof(int));
		nBufferSize = 64*10240;
		setsockopt(m_hUdpServerSocket,SOL_SOCKET,SO_RCVBUF,(char*)&nBufferSize,sizeof(int)); 
		
		unsigned long dwBytesReturned = 0;
		int bNewBehavior = FALSE;
		
		//����ĺ������ڽ��Զ��ͻȻ�رջᵼ��WSARecvFrom����10054�����·�������ɶ�����û��reeceive����������
		errorCode  = WSAIoctl(m_hUdpServerSocket, SIO_UDP_CONNRESET,&bNewBehavior, sizeof(bNewBehavior),NULL, 0, &dwBytesReturned,NULL, NULL);
		if (SOCKET_ERROR == errorCode)
		{
			PrintDebugMessage("Set listen socket to SIO_UDP_CONNRESET mode error.");
		}
		//����������ַ��Ϣ
		m_UdpServerAdd.sin_family = AF_INET;
		m_UdpServerAdd.sin_addr.s_addr = inet_addr(ip);
		m_UdpServerAdd.sin_port = htons(udp_prot);
		
		//��׽��ֺͷ�������ַ
		errorCode = bind(m_hUdpServerSocket,(struct sockaddr*)&m_UdpServerAdd,sizeof(m_UdpServerAdd));
		if(errorCode)
		{
			PrintDebugMessage("Error socket bind operation.");
		}
		
		//�Ѽ����̺߳���ɶ˿ڰ
		if(NULL == CreateIoCompletionPort((HANDLE)m_hUdpServerSocket,m_hCompletionPort,m_hUdpServerSocket,0))
			PrintDebugMessage("Invalid completion port handle.");

		if(m_pUdpContextManager==NULL)
			m_pUdpContextManager = new CContextManager<UDP_CONTEXT>(256);

		//Ͷ��reveive�������ȴ�client��udp���ݰ�����,Ӧ���߲���udp���ݱ��ĵ���
		if(PostUdpReceiveOp(m_hUdpServerSocket,64)!=64)
		{
			PrintDebugMessage("Ͷ��receive ������ʧ��.");
		}
         
		PrintDebugMessage("udp ��������ʼ���ɹ�.");

		return TRUE;
	}
	catch(CSrvException& e)
	{
       	m_bInitOk = false;
		this->PrintDebugMessage(e);
		//if(m_bExceptionStop)
		//	throw e;
	}
	catch(...)
	{
		m_bInitOk = false;

		string message("InitUdpServer ����δ֪�쳣.");
		PrintDebugMessage(message.c_str());
		if(m_bExceptionStop)
			throw message;
	}
	return FALSE;
}
int CIocpServer::InitTcpClient(char* remote_ip,unsigned short remote_port)
{
	int rc = FALSE;
	try
	{
	    m_hTcpClientSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if(m_hTcpServerSocket == INVALID_SOCKET)
		{
			PrintDebugMessage("��ʼ��tcp�׽���ʧ��.");
		}
		rc = this->AssociateAsTcpClient(m_hTcpClientSocket,remote_ip,remote_port);
		if(rc != 1) m_hTcpClientSocket = NULL;
	}
	catch(CSrvException& e)
	{
		this->PrintDebugMessage(e);
		if(m_bExceptionStop)
			throw e;
	}
	catch(...)
	{
		string message("InitTcpClient ����δ֪�쳣.");
		PrintDebugMessage(message.c_str());
		if(m_bExceptionStop)
			throw message;	
	}
	return rc;
}
int CIocpServer::InitUdpClient(char* local_ip,unsigned short local_port)
{
	int rc = FALSE;
	try
	{
	    m_hUdpClientSocket = WSASocket(AF_INET,SOCK_DGRAM,IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);

		if(m_hUdpClientSocket == INVALID_SOCKET)
			PrintDebugMessage("��ʼ��udp�׽���ʧ��.");

		rc = this->AssociateAsUdpClient(m_hUdpClientSocket,local_ip,local_port);
	}
	catch(CSrvException& e)
	{
		this->PrintDebugMessage(e);
		//if(m_bExceptionStop)
			//throw e;
	}
	catch(...)
	{
		string message("InitUdpClient ����δ֪�쳣.");
		PrintDebugMessage(message.c_str());
		//if(m_bExceptionStop)
		//	throw message;
	}
	return rc;
}

//�Ѵ����socket����ɶ˿ڹ���
//����ɶ˿ڸ����֪����ģ�����ݵĽ����뷢�͹���
int CIocpServer::AssociateAsUdpClient(SOCKET socket,char* local_ip,unsigned short local_port)
{
	BOOL rc = FALSE;
	try
	{
		if(socket == NULL)
			PrintDebugMessage("AssociateSocket() ������׽��־��.");

		if(local_ip == NULL)
			PrintDebugMessage("AssociateSocket() �����IP��ַ.");

		struct sockaddr_in local_addr;
		local_addr.sin_family = AF_INET;
		local_addr.sin_addr.s_addr = inet_addr(local_ip);
		local_addr.sin_port = htons(local_port);

		//��receiveform��sendto����Ϊ�첽������ģʽ,��Ϊ��WSAStartup��ʼ��Ĭ����ͬ��
		//�����׽��ֵ�����
		ULONG ul = 1;
		int errorCode = ioctlsocket(socket, FIONBIO, &ul);
		if(SOCKET_ERROR == errorCode)
		{
			PrintDebugMessage("Set listen socket to FIONBIO mode error.");
		}

		//����Ϊ��ַ���ã��ŵ����ڷ������رպ������������		
		int nOpt = 1;
		errorCode = setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (char*)&nOpt, sizeof(nOpt));
		if(SOCKET_ERROR == errorCode)
		{
			PrintDebugMessage("Set listen socket to SO_REUSEADDR mode error.");
		}

		//�رս����뷢�ͻ�����,ֱ��ʹ���û��ռ仺����,�����ڴ濽������
		int nBufferSize = 0;
		setsockopt(socket,SOL_SOCKET,SO_SNDBUF,(char*)&nBufferSize,sizeof(int));
		nBufferSize = 64*10240;
		setsockopt(socket,SOL_SOCKET,SO_RCVBUF,(char*)&nBufferSize,sizeof(int)); 

		unsigned long dwBytesReturned = 0;
		int bNewBehavior = FALSE;

		//����ĺ������ڽ��Զ��ͻȻ�رջᵼ��WSARecvFrom����10054�����·�������ɶ�����û��reeceive����������
		errorCode  = WSAIoctl(socket, SIO_UDP_CONNRESET,&bNewBehavior, sizeof(bNewBehavior),NULL, 0, &dwBytesReturned,NULL, NULL);
		if (SOCKET_ERROR == errorCode)
		{
			PrintDebugMessage("Set listen socket to SIO_UDP_CONNRESET mode error.");
		}

		//��׽��ֺʹ����ip��ַ
		errorCode = bind(socket,(struct sockaddr*)&local_addr,sizeof(local_addr));
		if(errorCode)
		{
			PrintDebugMessage("Error socket bind operation.",-1);
		}
		//�Ѷ˿ں���ɶ˿ڰ
		if(NULL == CreateIoCompletionPort((HANDLE)socket,m_hCompletionPort,socket,0))
			PrintDebugMessage("Invalid completion port handle.");

		if(m_pUdpContextManager==NULL)
			m_pUdpContextManager = new CContextManager<UDP_CONTEXT>(256);

		//Ͷ��reveive�������ȴ�client��udp���ݰ�����,��ҪΪӦ���߲���udp���ݱ��ĵ���
		if(PostUdpReceiveOp(socket,2)!=2)
		{
			PrintDebugMessage("Ͷ��receive ������ʧ��.");
		}

		rc = TRUE;
	}
	catch(CSrvException& e)
	{
		this->PrintDebugMessage(e);
		//if(m_bExceptionStop)
		//	throw e;
	}
	catch(...)
	{
		string message("AssociateAsUdpClient ����δ֪�쳣.");
		PrintDebugMessage(message.c_str());
		if(m_bExceptionStop)
			throw message;
	}
	return rc;
}
int CIocpServer::AssociateAsTcpClient(SOCKET socket,char* remote_ip,unsigned short remote_port)
{
	BOOL rc = FALSE;
	try
	{
		if(socket == NULL)
			PrintDebugMessage("AssociateSocket() ������׽��־��.");

		if(remote_ip == NULL)
			PrintDebugMessage("AssociateSocket() �����IP��ַ.");

		struct sockaddr_in serv_addr;
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = inet_addr(remote_ip);
		serv_addr.sin_port = htons(remote_port);

		//����Ϊsocket����,��ֹ������������˿��ܹ������ٴ�ʹ�û������Ľ���ʹ��
		int nOpt = 1;
		int errorCode = setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (char*)&nOpt, sizeof(nOpt));
		if(SOCKET_ERROR == errorCode)
		{
			PrintDebugMessage("Set client socket to SO_REUSEADDR mode error.");
		}

		//�ر�ϵͳ�Ľ����뷢�ͻ�����
		int nBufferSize = 0;
		setsockopt(socket,SOL_SOCKET,SO_SNDBUF,(char*)&nBufferSize,sizeof(int));
		nBufferSize = DEFAULT_TCP_RECEIVE_BUFFER_SIZE;
		setsockopt(socket,SOL_SOCKET,SO_RCVBUF,(char*)&nBufferSize,sizeof(int)); 

		//��׽��ֺͷ�������ַ
		errorCode = connect(socket,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
		if(errorCode == SOCKET_ERROR)
		{
			PrintDebugMessage("AssociateSocket(),���ӵ���������ʱ�����.");
		}
		//��socket����ɶ˿ڰ
		if(NULL == CreateIoCompletionPort((HANDLE)socket,m_hCompletionPort,socket,0))
		{
			PrintDebugMessage("Invalid completion port handle.");
		}

		if(m_pTcpReceiveContextManager==NULL)
			m_pTcpReceiveContextManager = new CContextManager<TCP_RECEIVE_CONTEXT>(512); //���ݽ����ڴ�ع������
		if(m_pTcpSendContextManager==NULL)
			m_pTcpSendContextManager = new CContextManager<TCP_SEND_CONTEXT>(512);//���ݷ����ڴ�ع������

		if(IssueTcpReceiveOp(socket,2)!=TRUE)
			PrintDebugMessage("AssociateAsClinetͶ���д���.");

		rc = TRUE;
	}
	catch(CSrvException& e)
	{
		this->PrintDebugMessage(e);
		PrintDebugMessage(e);
		//if(m_bExceptionStop)
			//throw e;
	}
	catch(...)
	{
		string message("AssociateAsTcpClient ����δ֪�쳣.");
		PrintDebugMessage(message.c_str());
		//if(m_bExceptionStop)
		//	throw message;
	}
	return rc;
}
int CIocpServer::CreateThread()
{
	if(m_bServerIsRunning)
	{
		PrintDebugMessage("���������߳�ʧ��,�����߳��Ѿ������е���.");
		return FALSE;
	}

	SYSTEM_INFO  sysinfo;
	GetSystemInfo(&sysinfo);
	m_iWorkerThreadNumbers = sysinfo.dwNumberOfProcessors*2+2;//����߳�������CPU������2��
	
	if(m_iWorkerThreadNumbers>=MAX_PROCESSER_NUMBERS) //ע��˴��ܿ��ܻ��������ܵľ����½�
		m_iWorkerThreadNumbers = MAX_PROCESSER_NUMBERS-1;
	
	m_iTimerId = timeSetEvent(1000,1000,TimerProc,(DWORD)this,TIME_PERIODIC);//1���Ӷ�ʱ��

	//���������߳�
	m_hListenThread = (HANDLE)_beginthreadex(NULL,0,ListenThread,this,0,NULL);
	if(!m_hListenThread)
	{
		PrintDebugMessage("���������߳�ʧ��.");
		return FALSE;
	}

	int counter = 0;
	for (int i=0; i<m_iWorkerThreadNumbers; i++)
	{
		m_hWorkThread[i] = (HANDLE)_beginthreadex(NULL,0,WorkThread,this,0,NULL);
		if(!m_hWorkThread[i]) //���ٴ���һ���߳�
		{
			m_hWorkThread[i] = NULL;
			continue;
		}	
		counter++;
	}
	m_iWorkerThreadNumbers = counter;
	return counter;
}
UINT WINAPI CIocpServer::ListenThread(LPVOID lpParama)
{
	CIocpServer* pThis = (CIocpServer*)lpParama;
	if(!pThis)
		return 0; 

	if(pThis->m_hTcpServerSocket==NULL)
	{
		pThis->PrintDebugMessage("����tcp �����߳�ʧ��,��ȷ�ϼ���socket�Ѿ�����.");
		return 0;
	}

	unsigned long rc = 0;
	unsigned short postAcceptCnt = 1;
	while(!pThis->m_bStopServer)
	{

		rc = WSAWaitForMultipleEvents(1, &pThis->m_hPostAcceptEvent, FALSE,1000, FALSE);		
		if(pThis->m_bStopServer)
			break;

		if(WSA_WAIT_FAILED == rc)
		{
			continue;
		}
		else if(rc ==WSA_WAIT_TIMEOUT)//��ʱ
		{
			continue;
		}
		else  
		{
			rc = rc - WSA_WAIT_EVENT_0;
			if(rc == 0)
				postAcceptCnt = 32;
			else
				postAcceptCnt = 1;

			pThis->PostTcpAcceptOp(postAcceptCnt);
		}
	}
	return 0;
}
UINT WINAPI CIocpServer::WorkThread(LPVOID lpParam)
{
	CIocpServer* p = (CIocpServer*)lpParam;
	if(!p)
		return 0;
	
	p->m_iOpCounter = 0;
	
	int bResult = FALSE;;
	unsigned long NumTransferred = 0;
	SOCKET socket = 0 ;
	IO_PER_HANDLE_STRUCT*  pOverlapped = NULL;
	HANDLE hCompletionPort = p->m_hCompletionPort;
	
	while(true)
	{
		bResult = FALSE;
		NumTransferred = 0;
		socket = NULL;
		pOverlapped = NULL;
		
		bResult = GetQueuedCompletionStatus(hCompletionPort,(LPDWORD)&NumTransferred,(LPDWORD)&socket,(LPOVERLAPPED *)&pOverlapped,INFINITE);
		
		if(bResult == FALSE)
		{
			if(pOverlapped !=NULL)
			{
				unsigned long errorCode = GetLastError();
				
				if(ERROR_INVALID_HANDLE == errorCode)   
				{   
					p->PrintDebugMessage("��ɶ˿ڱ��ر�,�˳�\n",errorCode);
					return 0;   
				}   
				else if(ERROR_NETNAME_DELETED == errorCode || ERROR_OPERATION_ABORTED == errorCode)   
				{   
					p->PrintDebugMessage("socket���ر� ���� ������ȡ��\n",errorCode);    
				}   
				else if(ERROR_MORE_DATA == errorCode)//�˴������ĸ��ʽϸߣ���������MTUֵ��̫���UDP�����������ϱ�����
				{
					if(pOverlapped->operate_type == IO_UDP_RECEIVE)
					{
						p->IssueUdpReceiveOp((UDP_CONTEXT*)pOverlapped);
					}
					p->PrintDebugMessage("Udp ���ݰ����ض�,���С���ݰ��ĳ���.");
					continue;
				}
				else if(ERROR_PORT_UNREACHABLE == errorCode)//Զ��ϵͳ�쳣��ֹ
				{
					p->PrintDebugMessage("�ͻ�����ֹ���ݷ���,�������:%d.\n",errorCode);
				}
				else
				{
					p->PrintDebugMessage("δ�������",errorCode);
                }
				p->ReleaseContext(pOverlapped);
			}
			else //��ʱ
			{
				if(WAIT_TIMEOUT == GetLastError() && p->m_bStopServer)
					break;
			}
			continue;
		}
		else//��������
		{
			if(pOverlapped)
			{
				p->CompleteIoProcess(pOverlapped,NumTransferred); //IO����
				::InterlockedExchangeAdd(&p->m_iOpCounter,1);
				continue;
			}
			else
			{
				p->PrintDebugMessage("�����߳������˳�.",GetLastError());
				if(socket == IO_EXIT && p->m_bStopServer)
					break;
			}
		}
	}
	return 0;
}
//��ɴ�������,��Ҫ���ͷ���Դ,�������ݻ�Ͷ���µ�io����
//��û�м��ʧ��
void CIocpServer::CompleteIoProcess(IO_PER_HANDLE_STRUCT* pOverlapped,unsigned long data_len)
{
	try
	{
		if(pOverlapped==NULL)
		{
			PrintDebugMessage("CompleteIoProcess ����pOverlapped�Ƿ�.");
			return ;
		}

		switch(pOverlapped->operate_type)
		{
		case IO_TCP_ACCEPT: //�������
			{
				TCP_ACCEPT_CONTEXT* pTcpAcceptContext = (TCP_ACCEPT_CONTEXT*)pOverlapped;

				CompleteTcpAcceptOp(pTcpAcceptContext,data_len);
				
				IssueTcpReceiveOp(pTcpAcceptContext->accept_socket,2);//�������ӵ�socket�Ϸ��ͽ������ݲ�����OP,��ʼ����2�ȴ��������ݵĵȴ�����
				IssueTcpAcceptOp(pTcpAcceptContext);//Ͷ���µ����Ӳ���������,���ֶ��������Ӳ�����ƽ�� 
				break;
			}
		case IO_TCP_RECEIVE: //�����������
			{
				TCP_RECEIVE_CONTEXT* pTcpReceiveContext = (TCP_RECEIVE_CONTEXT*)pOverlapped;
				if(data_len>0)
				{
				    CompleteTcpReceiveOp(pTcpReceiveContext,data_len);

					//����ĺ��������ã���client�����յ�������
					//this->IssueTcpSendOp(pTcpReceiveContext->socket,pTcpReceiveContext->receive_buffer,data_len);	

					this->IssueTcpReceiveOp(pTcpReceiveContext); //�����µĽ��ղ�������ɶ˿ڶ���
				}
				else //client �˳�
				{
					
					if( IsValidSocket(pTcpReceiveContext->socket))
					{	
						this->TcpCloseComplete(pTcpReceiveContext->socket) ;	
						this->CloseSocket(pTcpReceiveContext->socket) ;
						this->ReleaseContext(pOverlapped);									
					}					
				}
				break;
			}
		case IO_TCP_SEND:  //�����������
			{
				TCP_SEND_CONTEXT* pTcpSendContext = (TCP_SEND_CONTEXT*)pOverlapped;
				CompleteTcpSendOp(pTcpSendContext,data_len);

				this->ReleaseContext(pOverlapped);
				break;
			}
		case IO_UDP_RECEIVE:
			{
				UDP_CONTEXT* pUdpContext = (UDP_CONTEXT*)pOverlapped;

				this->CompleteUdpReceiveOp(pUdpContext,data_len); //��������������� 

				//����ĺ�����Ϊ������
				this->IssueUdpSendOp(pUdpContext->remote_address,pUdpContext->data_buffer,data_len,pUdpContext->socket); //��client�����յ�������

				this->IssueUdpReceiveOp(pUdpContext);//Ͷ������һ�����ղ�����������ɶ˿ڶ����еĽ��ղ���ƽ��   
				break;
			}
		case IO_UDP_SEND:
			{
				UDP_CONTEXT* pUdpContext = (UDP_CONTEXT*)pOverlapped;

				this->CompleteUdpSendOp(pUdpContext,data_len); //���������������
				this->ReleaseContext(pOverlapped);
				break;
			}
		case IO_EXIT:
			{
				break;
			}	
		default:
			break;
		}
	}
	catch(CSrvException& e)
	{
		PrintDebugMessage(e);
		//if(m_bExceptionStop)
		//	throw e;
	}
	catch(...)
	{
		string message("CompleteIoProcess ����δ֪�쳣.");
		PrintDebugMessage(message.c_str());
		//if(m_bExceptionStop)
		//	throw message;
	}
}

//�ͷ�context���ڴ��

void CIocpServer::ReleaseContext(IO_PER_HANDLE_STRUCT* pOverlapped)
{
	try
	{
		if(pOverlapped==NULL)
		{
			PrintDebugMessage("ReleaseContext(),ָ��pOverlapped�Ƿ�");
		}
		
        switch(pOverlapped->operate_type)
		{
		case IO_TCP_ACCEPT:
			{
				TCP_ACCEPT_CONTEXT* pTcpAcceptContext = (TCP_ACCEPT_CONTEXT*)pOverlapped;
				m_pTcpAcceptContextManager->ReleaseContext(pTcpAcceptContext);
				break;
			}
		case IO_TCP_RECEIVE:
			{
				TCP_RECEIVE_CONTEXT* pTcpReceiveContext = (TCP_RECEIVE_CONTEXT*)pOverlapped;
				m_pTcpReceiveContextManager->ReleaseContext(pTcpReceiveContext);
				break;
			}
		case IO_TCP_SEND:
			{
				TCP_SEND_CONTEXT* pTcpSendContext = (TCP_SEND_CONTEXT*)pOverlapped;
				m_pTcpSendContextManager->ReleaseContext(pTcpSendContext);
				break;
			}
		case IO_UDP_RECEIVE:
			{
				UDP_CONTEXT* pUdpContext = (UDP_CONTEXT*)pOverlapped;
				m_pUdpContextManager->ReleaseContext(pUdpContext);
				break;
			}
		case IO_UDP_SEND:
			{
				UDP_CONTEXT* pUdpContext = (UDP_CONTEXT*)pOverlapped;
				m_pUdpContextManager->ReleaseContext(pUdpContext);
				break;
			}
		case IO_EXIT:
			{
				break;
			}	
		default:
			break;
		}
	}
	catch(CSrvException& e)
	{
		this->PrintDebugMessage(e);
		
	}
	catch(...)
	{
		string message("ReleaseContext ����δ֪�쳣.");
		PrintDebugMessage(message.c_str());
		//if(m_bExceptionStop)
			//throw message;
	}
}
int CIocpServer::PostUdpReceiveOp(SOCKET socket,unsigned short numbers)
{
	int counter=0;
	for(int i=0;i<numbers;i++)
       counter += IssueUdpReceiveOp(socket);
	
	return counter;
}
int CIocpServer::IssueUdpReceiveOp(SOCKET socket)
{
	try
	{
		if(m_pUdpContextManager==NULL)
            PrintDebugMessage("ָ��m_pUdpContextManager�Ƿ�");

		int errorCode = 1;
		unsigned long dwPostReceiveNumbs = 0;
		
		UDP_CONTEXT* pUdpContext = NULL;
		m_pUdpContextManager->GetContext(pUdpContext);
		
		if(pUdpContext==NULL)
			PrintDebugMessage("pUdpContextָ��Ƿ�.");

		pUdpContext->socket = socket;  //�����׽��� 
		
		return IssueUdpReceiveOp(pUdpContext);
	}
	catch(CSrvException& e)
	{
		this->PrintDebugMessage(e);
		
	}
	catch(...)
	{
		string message("IssueUdpReceiveOp ����δ֪�쳣.");
		PrintDebugMessage(message.c_str());
		//if(m_bExceptionStop)
			//throw message;
	}
	return 0;
}
//�ڲ����Ժ���
int CIocpServer::IssueUdpReceiveOp(UDP_CONTEXT* pUdpContext)
{
	try
	{
		if(m_bStopServer)
		{
			ReleaseContext((IO_PER_HANDLE_STRUCT*)pUdpContext);
			return FALSE;
		}
		
		int errorCode = 1;
		unsigned long dwPostReceiveNumbs = 0;
		
		if(pUdpContext==NULL)
			throw CSrvException("pUdpContextָ��Ƿ�.",-1);
		
		unsigned long dwFlag = 0;
		unsigned long dwBytes = 0;

	    ZeroMemory(&pUdpContext->operate_overlapped,sizeof(WSAOVERLAPPED));
		pUdpContext->operate_type = IO_UDP_RECEIVE;
        //pUdpContext->listen_socket = m_hUdpServerSocket;
		pUdpContext->operate_buffer.buf = pUdpContext->data_buffer;
		pUdpContext->operate_buffer.len = DEFAULT_UDP_BUFFER_SIZE;
        ZeroMemory(&pUdpContext->remote_address,sizeof(struct sockaddr_in));
		pUdpContext->remote_address_len = sizeof(struct sockaddr);


		//���ͽ���operation����ɶ˿ڶ�����,�˴�һ�����첽����
		errorCode = WSARecvFrom(
			pUdpContext->socket,
			&pUdpContext->operate_buffer,
			1,
			&dwBytes,
			&dwFlag,
			(struct sockaddr*)&pUdpContext->remote_address,
			&pUdpContext->remote_address_len,
			&pUdpContext->operate_overlapped,
			NULL);
	  
		if (SOCKET_ERROR == errorCode && ERROR_IO_PENDING != WSAGetLastError())		
		{
			//if(WSAECONNRESET !=WSAGetLastError())//��Զ���رպ���ΪICMPЭ���ϵ������ִ˴���

			PrintDebugMessage("WSARecvFrom()������������.");
		}
		return TRUE;
	}
	catch(CSrvException& e)
	{
		ReleaseContext((IO_PER_HANDLE_STRUCT*)pUdpContext);
		this->PrintDebugMessage(e);
		if(m_bExceptionStop)
			throw e;
	}
	catch(...)
	{
		string message("IssueUdpReceiveOp ����δ֪�쳣.");
		PrintDebugMessage(message.c_str());
		//if(m_bExceptionStop)
			//throw message;
	}
	return FALSE;
}
//UDP������ɺ���
//�������֡ͷ��֡β�ļ��,������ݴ������󣬽��������(�����ʵ���ݷ������󣬽��ɸ߲������)
void CIocpServer::CompleteUdpReceiveOp(UDP_CONTEXT* pUdpContext,unsigned long data_len)
{	
	//�˴����͸��߲�������Ѿ�ȥ����֡ͷ
	if(m_pUdpRevCallbackFuc) //ִ�лص�����
	{
		m_pUdpRevCallbackFuc
		(inet_ntoa(pUdpContext->remote_address.sin_addr),
		ntohs(pUdpContext->remote_address.sin_port),
		pUdpContext->data_buffer,
		data_len,
		pUdpContext->socket
		);
	}
	
	this->UdpReceiveComplete
		(
		pUdpContext->remote_address,
		pUdpContext->data_buffer,
		data_len,
		pUdpContext->socket
		);
}
//�����첽���ͺ���
//�����ݳ��ȴ���DEFAULT_UDP_BUFFER_SIZE - 6�����ݻ���зְ�����
int CIocpServer::IssueUdpSendOp(struct sockaddr_in& remoteAddr,char* buf,unsigned long data_len,SOCKET socket)
{
	try
	{
		if(m_bStopServer) 
		{
			return FALSE;
		}

		if(m_pUdpContextManager==NULL)
			PrintDebugMessage("IssueUdpSendOp m_pUdpContextManagerָ��Ƿ�.");

			//�����ѭ���ǰ����ݿ�ְ�
			unsigned long leave_bytes = data_len;
			unsigned long trans_bytes = data_len;
			unsigned short counter = 0;
			unsigned short data_start_index = 0;  //������ʵ����ʼλ

		
			do
			{
				
				UDP_CONTEXT* pUdpContext = NULL;
				m_pUdpContextManager->GetContext(pUdpContext);
				
				if(pUdpContext==NULL)
					PrintDebugMessage("pUdpContextָ��Ƿ�.");

				if(leave_bytes <=DEFAULT_UDP_BUFFER_SIZE) //С��һ�η�����
					trans_bytes = leave_bytes;
				else
					trans_bytes = DEFAULT_UDP_BUFFER_SIZE;
				
				leave_bytes = leave_bytes - trans_bytes; //һ�η��ͺ�ʣ����ֽ���
				
				memcpy(pUdpContext->data_buffer,buf+counter*DEFAULT_UDP_BUFFER_SIZE,trans_bytes);  //�����ֽ���
				counter++;

				ZeroMemory(&pUdpContext->operate_overlapped,sizeof(WSAOVERLAPPED));
				pUdpContext->operate_type = IO_UDP_SEND;
				pUdpContext->socket = socket;
				pUdpContext->operate_buffer.buf = pUdpContext->data_buffer;
				pUdpContext->operate_buffer.len = trans_bytes;//ע��˴���6
				pUdpContext->remote_address = remoteAddr;
				pUdpContext->remote_address_len = sizeof(struct sockaddr);
				
				DWORD nubmerBytes = 0;
				int err = WSASendTo(
					pUdpContext->socket,
					&pUdpContext->operate_buffer,
					1,
					&nubmerBytes,
					0,
					(struct sockaddr*)&pUdpContext->remote_address,
					sizeof(pUdpContext->remote_address),
					&pUdpContext->operate_overlapped,
					NULL);
				if(SOCKET_ERROR == err && WSA_IO_PENDING != WSAGetLastError())
				{
					ReleaseContext((IO_PER_HANDLE_STRUCT*)pUdpContext);
					PrintDebugMessage("IssueUdpSendOp->WSASend��������.");
				}
				else if(0 == err)
				{
					//��Ҫ����������ɵ����
				}
			}while(leave_bytes>0);

		return TRUE;
	}
	catch(CSrvException& e)
	{
		this->PrintDebugMessage(e);
		//if(m_bExceptionStop)
			//throw e;
	}
	catch(...)
	{
		string message("IssueUdpSendOp ����δ֪�쳣.");
		PrintDebugMessage(message.c_str());
		if(m_bExceptionStop)
			throw message;
	}
	return FALSE;
}
//�������������Ӧ�÷��ڸ߲�
/*
unsigned short CIocpServer::AddHead(char* data,unsigned short data_len,unsigned short pack_index)   //��udp���ݰ��м���֡ͷ
{

	    WORD index = 0;
		//�����ʽ,�����ռ6���ֽ�
		data[index++] =  (BYTE)PACKET_START1;   //0xAB
		data[index++] = (BYTE)PACKET_START2;   //0xCD
		//*PWORD(&data[index]) = (WORD)pack_index;	//���ݰ�index
		//index += 2;
		*PWORD(&data[index]) = (WORD)data_len;	//���ݳ���,2���ֽ�
		index += 2;
		index += data_len;                    //������ʵ�������� 
		data[index++] = (BYTE)PACKET_END1;	  //0xDC
		data[index++] = (BYTE)PACKET_END2;	  //0xBA

		return index ;
}
int CIocpServer::CheckHead(char* data,unsigned short data_len,PPACKET_HEAD& pPacketHead) //��udp���ݰ�,�������Ƿ�Ϸ�
{
	if(data_len <= PACKET_HEAD_SIZE)	//�����ǿ�����,�϶��д���
		return FALSE;

	PPACKET_HEAD _pPackeHead = (PPACKET_HEAD)data;	//�õ������ͷ

	if( _pPackeHead->fram_head != ((DWORD)PACKET_START2<<8) + PACKET_START1 )//֡ͷ�ж�
		return FALSE;

	if( _pPackeHead->data_len+PACKET_HEAD_SIZE != data_len)	   //pChenkSum->Length����ʵ���ݵ����ݳ���, +6�Ƿ������յ����ݰ�����
		return FALSE;

	if( *PWORD(&data[_pPackeHead->data_len+PACKET_HEAD_SIZE-2]) != (((DWORD)PACKET_END2<<8) + PACKET_END1) ) //������ֽ��Ƿ���֡β
		return FALSE;

	pPacketHead = _pPackeHead;

	return TRUE;
}
*/
//����һ��udp���ݱ��������
void CIocpServer::CompleteUdpSendOp(UDP_CONTEXT* pUdpContext,unsigned long data_len)
{
	if(m_pUdpSendCallbackFuc)  //��������˻ص�����,��ִ�лٵ�����
	{
		m_pUdpSendCallbackFuc
			(
			inet_ntoa(pUdpContext->remote_address.sin_addr),
			ntohs(pUdpContext->remote_address.sin_port),
			pUdpContext->data_buffer,
			data_len,
			pUdpContext->socket
			);
	}

		this->UdpSendComplete
			(
			inet_ntoa(pUdpContext->remote_address.sin_addr),
			ntohs(pUdpContext->remote_address.sin_port),
			pUdpContext->data_buffer,
			data_len,
			pUdpContext->socket
			);
}

//����ɶ˿���Ͷ���µ�accept����
int CIocpServer::PostTcpAcceptOp(unsigned short accept_numbers)
{
	try
	{ 
		unsigned short counter = 0;
		for(int i=0;i<accept_numbers;i++)
			counter+=IssueTcpAcceptOp();
		if(counter != accept_numbers)
			PrintDebugMessage("Ͷ��accept�д�����,");

		return TRUE;
	}
	catch(CSrvException& e)
	{
		this->PrintDebugMessage(e);
		
	}
	catch(...)
	{
		string message("PostTcpAcceptOp ����δ֪�쳣.");
		PrintDebugMessage(message.c_str());
		//if(m_bExceptionStop)
			//throw message;
	}
	return FALSE;
}
int CIocpServer::IssueTcpAcceptOp()
{
	try
	{
		if(m_pTcpAcceptContextManager==NULL)
		    PrintDebugMessage("ָ��m_pTcpAcceptContextManager�Ƿ�.");

		TCP_ACCEPT_CONTEXT* pAccContext = NULL;
		m_pTcpAcceptContextManager->GetContext(pAccContext);
		if (NULL == pAccContext)
		{
		    PrintDebugMessage("ָ��pAccContext");
		}

		ZeroMemory(&pAccContext->operate_overlapped,sizeof(WSAOVERLAPPED));
		pAccContext->operate_type = IO_TCP_ACCEPT;
		pAccContext->listen_socket = this->m_hTcpServerSocket;
        pAccContext->accept_socket = NULL;
		ZeroMemory(pAccContext->remoteAddressBuffer,ADDRESS_LENGTH*2);

        return this->IssueTcpAcceptOp(pAccContext);
	}
	catch(CSrvException& e)
	{
		this->PrintDebugMessage(e);
		
	}
	catch(...)
	{
		string message("IssueTcpAcceptOp ����δ֪�쳣.");
		PrintDebugMessage(message.c_str());
		//if(m_bExceptionStop)
			//throw message;
	}
	return FALSE;
}
int CIocpServer::IssueTcpAcceptOp(TCP_ACCEPT_CONTEXT* pContext)
{
	SOCKET hClientSocket = NULL;
	try
	{
		if(pContext==NULL)
			PrintDebugMessage("IssueTcpAcceptOp(),ָ��pContext�Ƿ�.");


		int errorCode = 1;
		hClientSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (INVALID_SOCKET == hClientSocket)
		{
			PrintDebugMessage("IssueTcpAcceptOp()->WSASocket(),Invalid socket handle.");
		}

		//����Ϊ�첽ģʽ
		ULONG ul = 1;
		errorCode =ioctlsocket(hClientSocket, FIONBIO, &ul);
		if(SOCKET_ERROR == errorCode)
		{
			PrintDebugMessage("IssueTcpAcceptOp()->ioctlsocket(),incorrect ioctlsocket operation.");
		}

		long accTime = -1; //�˱�־�����������ӵ�socket���Ƿ����ӵ�socket�ؼ�
		errorCode = setsockopt(hClientSocket, SOL_SOCKET, SO_GROUP_PRIORITY, (char *)&accTime, sizeof(accTime));
		if(SOCKET_ERROR == errorCode)
		{
			PrintDebugMessage("IssueTcpAcceptOp()->setsockopt().");
		}

		ZeroMemory(&pContext->operate_overlapped,sizeof(WSAOVERLAPPED));
		pContext->operate_type = IO_TCP_ACCEPT;
		pContext->listen_socket = this->m_hTcpServerSocket;
		pContext->accept_socket = hClientSocket;
		ZeroMemory(pContext->remoteAddressBuffer,ADDRESS_LENGTH*2);


		unsigned long dwBytes = 0;
		unsigned long dwAcceptNumbs = 0;

		//����ɶ˿�Ͷ��accept����
		errorCode = m_pfAcceptEx(
			pContext->listen_socket,
			pContext->accept_socket, 
			pContext->remoteAddressBuffer,
			0,
			ADDRESS_LENGTH, 
			ADDRESS_LENGTH,
			&dwBytes,
			&(pContext->operate_overlapped));
		if(FALSE == errorCode && ERROR_IO_PENDING != WSAGetLastError())
		{
			ReleaseContext((IO_PER_HANDLE_STRUCT*)pContext);
			PrintDebugMessage("IssueTcpAcceptOp()->AcceptEx,incorrect AcceptEx operation.");
		}

		
		this->SaveConnectSocket(hClientSocket); //��socket����������й���

		return TRUE;
	}
	catch(CSrvException& e)
	{
		if(hClientSocket != INVALID_SOCKET)
		{
			CloseSocket(hClientSocket);
		}
		this->PrintDebugMessage(e);
		//if(m_bExceptionStop)
			//throw e;
	}
	catch(...)
	{
		string message("IssueTcpAcceptOp ����δ֪�쳣.");
		PrintDebugMessage(message.c_str());
		//if(m_bExceptionStop)
			//throw message;
	}
	return FALSE;
}

//�µ����Ӵ�����
void CIocpServer::CompleteTcpAcceptOp(TCP_ACCEPT_CONTEXT* pContext,unsigned long data_len)
{
	try
	{
		if(pContext == NULL)
			PrintDebugMessage("CompleteTcpAcceptOp error,ָ��pContext�Ƿ�.");

		this->GetNewConnectInf(pContext);   //�õ�������socket����Ϣ

		if(m_pTcpAcceptCallbackFuc)
			m_pTcpAcceptCallbackFuc(pContext->accept_socket,pContext->remoteAddressBuffer,ADDRESS_LENGTH*2);
		//�������޸���Ӧsocket�Ĳ���
		this->SetNewSocketParameters(pContext->accept_socket);

		
	}
	catch(CSrvException& e)
	{
		this->ReleaseContext((IO_PER_HANDLE_STRUCT*)pContext);
		this->PrintDebugMessage(e);
		if(m_bExceptionStop)
			throw e;
	}
	catch(...)
	{
		string message("CompleteTcpAcceptOp ����δ֪�쳣.");
		PrintDebugMessage(message.c_str());
		if(m_bExceptionStop)
			throw message;
	}
}
//����
//1 ����������socket�Ĳ���,ֻ���tcp
//2 �������������socket���뵽һ��map�н��м��й���
void CIocpServer::SetNewSocketParameters(SOCKET socket)
{
	try
	{
		int errorCode = 0;
		int nZero=0;
		errorCode = setsockopt(socket,SOL_SOCKET,SO_SNDBUF,(char *)&nZero,sizeof(nZero));
		if(errorCode == SOCKET_ERROR)
		{
			PrintDebugMessage("CompleteTcpAcceptOp->setsockopt() error..");
		}

		nZero = DEFAULT_TCP_RECEIVE_BUFFER_SIZE;
		errorCode = setsockopt(socket,SOL_SOCKET,SO_RCVBUF,(char *)&nZero,sizeof(nZero));
		if(errorCode == SOCKET_ERROR)
		{
			PrintDebugMessage("CompleteTcpAcceptOp->setsockopt() error..");
		}
		int nOpt = 1;//socket����,�ڳ���TIME_WAIT״̬��ʱ�����ø�socket
		errorCode = setsockopt(socket,SOL_SOCKET ,SO_REUSEADDR,(const char*)&nOpt,sizeof(nOpt));
		if(errorCode == SOCKET_ERROR)
		{
			PrintDebugMessage("CompleteTcpAcceptOp->setsockopt() error..");
		}
		int dontLinget = 1;//�������ر�socket��ʱ��,��ִ���������Ĳ����ֹرգ�����ִ��RESET
		errorCode = setsockopt(socket,SOL_SOCKET,SO_DONTLINGER,(char *)&dontLinget,sizeof(dontLinget)); 
		if(errorCode == SOCKET_ERROR)
		{
			PrintDebugMessage("CompleteTcpAcceptOp->setsockopt() error..");
		}
		char chOpt=1; //���͵�ʱ��ر�Nagle�㷨,�ر�nagel�㷨�п��ܻ��������(ѹ��)
		errorCode = setsockopt(socket,IPPROTO_TCP,TCP_NODELAY,&chOpt,sizeof(char));   
		if(errorCode == SOCKET_ERROR)
		{
			PrintDebugMessage("CompleteTcpAcceptOp->setsockopt() error..");
		}

		//���µ�ǰsocket���ʱ��,���ڳ�ʱ���
		long accTime = ::InterlockedExchangeAdd(&m_iCurrentTime,0);
		errorCode = setsockopt(socket, SOL_SOCKET, SO_GROUP_PRIORITY, (char *)&accTime, sizeof(accTime));
		if(errorCode == SOCKET_ERROR)
		{
			PrintDebugMessage("CompleteTcpAcceptOp->setsockopt() error..");
		}
		if(!this->SetHeartBeatCheck(socket))//��������������
		{
			PrintDebugMessage("CompleteTcpAcceptOp->setsockopt() error..");
		}
		//���µ�socket����ɶ˿ڰ�
		if(NULL == CreateIoCompletionPort((HANDLE)socket,this->m_hCompletionPort,(ULONG_PTR)socket,0))
		{
			PrintDebugMessage("CompleteTcpAcceptOp->setsockopt() error..");
		}
	}
	catch(CSrvException& e)
	{
		/**
		 *	��ֹĿ��ڵ㲻�����Է�������
		 */
	
		this->PrintDebugMessage(e);
		if(m_bExceptionStop)
			throw e.GetExpCode();
	}
	catch(...)
	{
		string message("SetNewSocketParameters ����δ֪�쳣.");
		PrintDebugMessage(message.c_str());
		//if(m_bExceptionStop)
			//throw message;
	}
}
//��ʾ������client��Ϣ
void CIocpServer::GetNewConnectInf(TCP_ACCEPT_CONTEXT* pContext)
{
	//�ڿͻ��˷����������ʱ��,��������ù��ܣ�����ֺ����IssueReceiveOperation�б�10057����,���˺�������ɿ���û����ȷ�������ͻ�����Ϣ
	sockaddr_in* pClientAddr = NULL;
	sockaddr_in* pLocalAddr = NULL;
	int nClientAddrLen = 0;
	int nLocalAddrLen = 0;

	m_pfGetAddrs(pContext->remoteAddressBuffer, 0,ADDRESS_LENGTH,ADDRESS_LENGTH,
		(LPSOCKADDR*)&pLocalAddr, &nLocalAddrLen, (LPSOCKADDR*)&pClientAddr, &nClientAddrLen);

	//printf("�ͻ���: %s : %u ���ӷ������ɹ�.\n",inet_ntoa(pClientAddr->sin_addr),ntohs(pClientAddr->sin_port));

	this->TcpAcceptComplete(pContext->accept_socket,inet_ntoa(pClientAddr->sin_addr),ntohs(pClientAddr->sin_port));
								  
}
int CIocpServer::IssueTcpReceiveOp(SOCKET socket,unsigned short times/*=1*/)
{  
	TCP_RECEIVE_CONTEXT* pContext = NULL;
	try
	{
		if(m_pTcpReceiveContextManager==NULL)
			PrintDebugMessage("IssueTcpReceiveOp(),m_pTcpReceiveContextManagerָ��Ƿ�.");

		for(int i=0;i<times;i++)
		{
			m_pTcpReceiveContextManager->GetContext(pContext);
			if(pContext==NULL)
			{
				PrintDebugMessage("IssueTcpReceiveOp,ȡ����pContextʧ��.");
			}

			ZeroMemory(&pContext->operate_overlapped,sizeof(WSAOVERLAPPED));
			pContext->operate_type = IO_TCP_RECEIVE;
			pContext->socket = socket;
			pContext->operate_buffer.buf = pContext->receive_buffer;
			pContext->operate_buffer.len = DEFAULT_TCP_RECEIVE_BUFFER_SIZE;

			if(!this->IssueTcpReceiveOp(pContext))
				 PrintDebugMessage("IssueTcpReceiveOp,Ͷ��receive ����ʧ��.");

		}
		return TRUE;
	}
	catch(CSrvException& e)
	{
		this->ReleaseContext((IO_PER_HANDLE_STRUCT*)pContext);
		this->PrintDebugMessage(e);
		if(m_bExceptionStop)
			throw e;
	}
	catch(...)
	{
		string message("IssueTcpReceiveOp ����δ֪�쳣.");
		PrintDebugMessage(message.c_str());
		//if(m_bExceptionStop)
			//throw message;
	}
	return FALSE;
}
int CIocpServer::IssueTcpReceiveOp(TCP_RECEIVE_CONTEXT* pContext)
{
	try
	{
		unsigned long bytes = 0;
		unsigned long flag = 0;
		int err = WSARecv(pContext->socket,&pContext->operate_buffer,1,&bytes,&flag, &(pContext->operate_overlapped), NULL);
		if (SOCKET_ERROR == err && WSA_IO_PENDING != WSAGetLastError())
		{
			PrintDebugMessage("IssueTcpReceiveOp->WSARecv() error.");
		}
		return TRUE;
	}
	catch(CSrvException& e)
	{
		this->PrintDebugMessage(e);
		//if(m_bExceptionStop)
			//throw e;
	}
	catch(...)
	{
		string message("IssueTcpReceiveOp ����δ֪�쳣.");
		PrintDebugMessage(message.c_str());
		//if(m_bExceptionStop)
			//throw message;
	}
	return FALSE;
}
void CIocpServer::CompleteTcpReceiveOp(TCP_RECEIVE_CONTEXT* pContext,unsigned long data_len)
{
	long accTime = ::InterlockedExchangeAdd(&m_iCurrentTime,0);
	setsockopt(pContext->socket, SOL_SOCKET, SO_GROUP_PRIORITY, (char *)&accTime, sizeof(accTime));
	
	if(m_pTcpRevCallbackFuc)
        m_pTcpRevCallbackFuc(pContext->socket,pContext->receive_buffer,data_len);

	this->TcpReceiveComplete(pContext->socket,pContext->receive_buffer,data_len);
}

int CIocpServer::IssueTcpSendOp(SOCKET client,char* buf,unsigned long data_len)
{
	try
	{//�����ѭ���ǰ����ݿ�ְ�
		unsigned long leave_bytes = data_len;
		unsigned long trans_bytes = data_len;
		unsigned long counter = 0;
		do
		{

			TCP_SEND_CONTEXT* pContext = NULL;
			m_pTcpSendContextManager->GetContext(pContext);

			if(pContext==NULL)
			{
				return 0;
				//throw CSrvException("IssueTcpSendOp(),pContextָ��Ƿ�.",-1);
			}

			//ZeroMemory(pUdpContext,sizeof(UDP_CONTEXT));


			if(leave_bytes <=DEFAULT_TCP_SEND_BUFFER_SIZE) //С��һ�η�����
				trans_bytes = leave_bytes;
			else
				trans_bytes = DEFAULT_TCP_SEND_BUFFER_SIZE;

			leave_bytes = leave_bytes - trans_bytes; //һ�η��ͺ�ʣ����ֽ���

			memcpy(pContext->send_buffer,buf+counter*DEFAULT_TCP_SEND_BUFFER_SIZE,trans_bytes);  //�����ֽ���
			counter++;

			ZeroMemory(&pContext->operate_overlapped,sizeof(WSAOVERLAPPED));
			pContext->operate_type = IO_TCP_SEND;
			pContext->socket = client;
			pContext->operate_buffer.buf = pContext->send_buffer;
			pContext->operate_buffer.len = trans_bytes;

			DWORD nubmerBytes = 0;
			int err = WSASend(pContext->socket,&pContext->operate_buffer,1,&nubmerBytes,0,&pContext->operate_overlapped,NULL);
			if(SOCKET_ERROR == err && WSA_IO_PENDING != WSAGetLastError())
			{
				/**
				 *	�쳣���ߣ����ͻ���쳣
				 */
				this->ReleaseContext((IO_PER_HANDLE_STRUCT*)pContext);
				PrintDebugMessage("IssueTcpSendOp->WSASend occured an error.1111");
			}
			else if(0 == err)
			{
				;//��Ҫ����������ɵ����,������������ɶ˿��ϵ��߳�
			}
		}
		while(leave_bytes>0);

		return TRUE;
	}
	catch(CSrvException& e)
	{
		//PrintDebugMessage(e);
		throw e; 
		
		//if(m_bExceptionStop)
			//throw e;
	}
	catch(...)
	{
		string message("IssueTcpSendOp ����δ֪�쳣.");
		PrintDebugMessage(message.c_str());
	}
	return FALSE;
}

void CIocpServer::CompleteTcpSendOp(TCP_SEND_CONTEXT* pContext,unsigned long data_len)
{
	long accTime = ::InterlockedExchangeAdd(&m_iCurrentTime,0);
	setsockopt(pContext->socket, SOL_SOCKET, SO_GROUP_PRIORITY, (char *)&accTime, sizeof(accTime));

	if(m_pTcpSendCallbackFuc)
		m_pTcpSendCallbackFuc(pContext->socket,pContext->send_buffer,data_len);

	this->TcpSendComplete(pContext->socket,pContext->send_buffer,data_len);
}

//�ⲿ�����첽���ͺ���
//���socket == NULL,���ʾ�Ƿ���������client��������
//���socket�ǹ�����socket,������ǿͻ���,Ҳ�����Ƿ�������
int CIocpServer::UdpSend(char* remote_ip,unsigned short remote_port,char* buf,unsigned long data_len,SOCKET socket/*=NULL*/)
{
	try
	{
		if(remote_ip==NULL)
			PrintDebugMessage("�ͻ���ip��ַ����.");
		if(data_len==0)
			PrintDebugMessage("�����ֽ���Ϊ0.");

		struct sockaddr_in remote_address;
		remote_address.sin_family = AF_INET;
		remote_address.sin_addr.s_addr = inet_addr(remote_ip);
		remote_address.sin_port = htons(remote_port);

		SOCKET temp_socket = socket;
		if(socket == NULL)   //Ĭ����ͨ��udp��������socket��������
			temp_socket = this->m_hUdpServerSocket;
		if(temp_socket == NULL)
			PrintDebugMessage("�ڷ�������ʱ��Ҫָ��socket���.");

        if(IssueUdpSendOp(remote_address,buf,data_len,temp_socket)==FALSE) //��������
			PrintDebugMessage("��������ʧ��.");

		return TRUE;
	}
	catch(CSrvException& e)
	{
		this->PrintDebugMessage(e);
		if(m_bExceptionStop)
			throw e.GetExpCode();
	}
	catch(...)
	{
		string message("UdpSend ����δ֪�쳣.");
		PrintDebugMessage(message.c_str());
		//if(m_bExceptionStop)
			//throw message;
	}
	return FALSE;
}
//���ⲿʹ�õ�tcp���ݷ��ͺ���
int CIocpServer::TcpSend(SOCKET client,char* buf,unsigned long data_len)
{
try
	{
		if(client==NULL)
			PrintDebugMessage("�ͻ���socket����.");
		if(data_len==0)
			PrintDebugMessage("�����ֽ���Ϊ0.");

        if(IssueTcpSendOp(client,buf,data_len)==FALSE) //��������
			PrintDebugMessage("��������ʧ��.");

		return TRUE;
	}
	catch(CSrvException& e)
	{
		if( IsValidSocket( client ) )
		{
			TcpCloseComplete(client) ;	
			CloseSocket(client);
		}	
		this->PrintDebugMessage(e);
	}
	catch(...)
	{
		string message("TcpSend ����δ֪�쳣.");
		PrintDebugMessage(message.c_str());
		//if(m_bExceptionStop)
			//throw message;
	}
	return FALSE;
}
int CIocpServer::Run()
{
	try
	{
		if(m_bServerIsRunning)
			PrintDebugMessage("�������Ѿ�����.");

		CreateThread(); //������صĹ����߳�
        
		m_bServerIsRunning = true;
		return TRUE;
	}
	catch(CSrvException& e)
	{
		this->PrintDebugMessage(e,false);
		//if(m_bExceptionStop)
		//	throw e;
	}
	catch(...)
	{
		string message("Run ����δ֪�쳣.");
		PrintDebugMessage(message.c_str());
		//if(m_bExceptionStop)
			//throw message;
	}
	return FALSE;
}
int CIocpServer::Stop()
{
	try
	{
		if(!m_bInitOk)
            PrintDebugMessage("��������ʼ��ʧ��,�޷�����������.");
		if(!m_bServerIsRunning)
			PrintDebugMessage("�������Ѿ�ֹͣ.");
		
		this->ReleaseResource();

		m_bInitOk = true;
		m_bStopServer = false;
		m_bServerIsRunning = false;

		return TRUE;
	}
	catch(CSrvException& e)
	{
		this->PrintDebugMessage(e);
		//if(m_bExceptionStop)
		//	throw e;
	}
	catch(...)
	{
		string message("Stop ����δ֪�쳣.");
		PrintDebugMessage(message.c_str());
		//if(m_bExceptionStop)
		//	throw message;
	}
	return FALSE;
}
void PASCAL CIocpServer::TimerProc(UINT wTimerID,UINT msg,DWORD dwUser,DWORD dw1,DWORD dw2)
{
	
 	CIocpServer* p = (CIocpServer*)dwUser;
 
 	p->m_iCurrentTime = (long)time(NULL); 
// 
 	p->ConnectTimeOutCheck(); //���ӳ�ʱ���
 	p->TimerSecond() ; 
//    
// 	long contextSize=0;
// 	long busyContext =0;
// 
// 	//printf("IO op counter:%ld\n",p->m_iOpCounter);
// 
// 	if(p->m_pUdpContextManager)
// 	{
// 		p->m_pUdpContextManager->GetSize(contextSize);
// 		p->m_pUdpContextManager->GetBusyCounter(busyContext);
// 		//printf("udp context:%ld,udp busy context:%ld\n",contextSize,busyContext);
// 	}
// 
// 	if(p->m_pTcpAcceptContextManager)
// 	{
// 		p->m_pTcpAcceptContextManager->GetSize(contextSize);
// 		p->m_pTcpAcceptContextManager->GetBusyCounter(busyContext);
// 		//printf("tcp accept context:%ld,tcp accept busy context:%ld\n",contextSize,busyContext);
// 	}
// 	if(p->m_pTcpReceiveContextManager)
// 	{
// 		p->m_pTcpReceiveContextManager->GetSize(contextSize);
// 		p->m_pTcpReceiveContextManager->GetBusyCounter(busyContext);
// 		//printf("tcp receive context:%ld,tcp receive busy context:%ld\n",contextSize,busyContext);
// 	}
// 
// 	if(p->m_pTcpSendContextManager)
// 	{
// 		p->m_pTcpSendContextManager->GetSize(contextSize);
// 		p->m_pTcpSendContextManager->GetBusyCounter(busyContext);
// 		//printf("tcp send context:%ld,tcp send busy context:%ld\n\n",contextSize,busyContext);
// 	}

}
//�ͷŷ����˵���Դ
void CIocpServer::ReleaseResource()
{
	try
	{
	m_bStopServer = true;
	timeKillEvent(m_iTimerId);
	m_iTimerId = 0;

	CloseAllSocket(); //�ر�����socket,������ͷǻ��,��ʹ��ɶ˿��е������ŶӵĲ�������

	if(m_hUdpServerSocket)
	{
		CancelIo((HANDLE)m_hUdpServerSocket);
		shutdown(m_hUdpServerSocket,SD_BOTH); 
		closesocket(m_hUdpServerSocket);
		m_hUdpServerSocket = NULL;
	}
	if(m_hTcpServerSocket)
	{
		CancelIo((HANDLE)m_hTcpServerSocket);
		shutdown(m_hTcpServerSocket,SD_BOTH); 
		closesocket(m_hTcpServerSocket);
		m_hTcpServerSocket = NULL;
	}
	if(m_hTcpClientSocket)
	{
		CancelIo((HANDLE)m_hTcpClientSocket);
		shutdown(m_hTcpClientSocket,SD_BOTH); 
		closesocket(m_hTcpClientSocket);
		m_hTcpClientSocket = NULL;
	}
	if(m_hUdpClientSocket)
	{
		CancelIo((HANDLE)m_hUdpClientSocket);
		shutdown(m_hUdpClientSocket,SD_BOTH); 
		closesocket(m_hUdpClientSocket);
		m_hUdpClientSocket = NULL;
	}

    Sleep(1000);

	for(int i=0;i<m_iWorkerThreadNumbers;i++)
		PostQueuedCompletionStatus(m_hCompletionPort,0,IO_EXIT,NULL);

	WaitForMultipleObjects(m_iWorkerThreadNumbers,m_hWorkThread,TRUE,INFINITE);
	for(int i=0;i<m_iWorkerThreadNumbers;i++)
	{
		CloseHandle(m_hWorkThread[i]);
		m_hWorkThread[i] = NULL;
	}

	if(m_hListenThread)
	{
		SetEvent(m_hPostAcceptEvent);
		WaitForSingleObject(m_hListenThread,INFINITE);
		CloseHandle(m_hListenThread);
		m_hListenThread = NULL;
	}

	long busyCounter = 0;
	if(m_pUdpContextManager)
	{	
		m_pUdpContextManager->GetBusyCounter(busyCounter);
		if(busyCounter != 0)
		   PrintDebugMessage("m_pUdpContextManager �������ڴ�й©.",busyCounter);

		delete m_pUdpContextManager;
		m_pUdpContextManager = NULL;
	}
	if(m_pTcpAcceptContextManager)
	{
		m_pTcpAcceptContextManager->GetBusyCounter(busyCounter);
		if(busyCounter != 0)
		   PrintDebugMessage("m_pTcpAcceptContextManager �������ڴ�й©.",busyCounter);

		delete m_pTcpAcceptContextManager;
		m_pTcpAcceptContextManager = NULL;
	}
	if(m_pTcpReceiveContextManager)
	{
		m_pTcpReceiveContextManager->GetBusyCounter(busyCounter);
		if(busyCounter != 0)
		   PrintDebugMessage("m_pTcpReceiveContextManager �������ڴ�й©.",busyCounter);

		delete m_pTcpReceiveContextManager;
		m_pTcpReceiveContextManager = NULL;
	}
	if(m_pTcpSendContextManager)
	{
		m_pTcpSendContextManager->GetBusyCounter(busyCounter);
		if(busyCounter != 0)
		   PrintDebugMessage("m_pTcpSendContextManager �������ڴ�й©.",busyCounter);

		delete m_pTcpSendContextManager;
		m_pTcpSendContextManager = NULL;
	}
	}
	catch(CSrvException& e)
	{
		PrintDebugMessage(e);
		if(m_bExceptionStop)
			throw e;
	}
	catch(...)
	{
		string message("ReleaseResource ����δ֪�쳣.");
		PrintDebugMessage(message.c_str());
		//if(m_bExceptionStop)
			//throw message;
	}
}
int CIocpServer::SaveConnectSocket(SOCKET socket)
{
	int rc = 0;
	::EnterCriticalSection(&m_struCriSec);

	rc = (int)m_mapAcceptSockQueue.size();
	if(m_mapAcceptSockQueue.insert(std::make_pair(socket,rc++)).second)
		rc = 1;
	else
		rc = 0;
	LeaveCriticalSection(&m_struCriSec);
	return rc;
}
bool CIocpServer::IsValidSocket(SOCKET socket)
{
	bool validSocket = false;
	::EnterCriticalSection(&this->m_struCriSec);
	SOCKET sock = NULL;
	map<SOCKET,long>::iterator pos;
	pos = m_mapAcceptSockQueue.find(socket);
	if(pos!=m_mapAcceptSockQueue.end())
	{
		validSocket = true;
	}
	::LeaveCriticalSection(&this->m_struCriSec);
	return validSocket;
}
//�ر�socket
void CIocpServer::CloseSocket(SOCKET socket)
{
	EnterCriticalSection(&this->m_struCriSec);
	SOCKET sock = NULL;
	map<SOCKET,long>::iterator pos;
	pos = m_mapAcceptSockQueue.find(socket);
	if(pos!=m_mapAcceptSockQueue.end())
	{
		sock = pos->first;
		m_mapAcceptSockQueue.erase(pos);
	}

	if(sock)
	{
	    CancelIo((HANDLE)sock);
		shutdown(sock,SD_BOTH); 
		closesocket(sock);//SO_UPDATE_ACCEPT_CONTEXT �����ᵼ�³���TIME_WAIT,��ʹ������DONT
	} 

	::LeaveCriticalSection(&this->m_struCriSec);
}
void CIocpServer::CloseAllSocket()//�ر�����ɶ˿��Ͻ��в���������socket
{
   EnterCriticalSection(&m_struCriSec);

   map<SOCKET,long>::iterator pos;
   for(pos = m_mapAcceptSockQueue.begin();pos!=m_mapAcceptSockQueue.end();)
   {
	   CancelIo((HANDLE)pos->first);
	   shutdown(pos->first,SD_BOTH); 
	   closesocket(pos->first);
	   m_mapAcceptSockQueue.erase(pos++);
   }

   LeaveCriticalSection(&m_struCriSec);
}
//���ͻȻ���ߵ�socket��ʱʱ��
void CIocpServer::ConnectTimeOutCheck()
{
	if(TryEnterCriticalSection(&m_struCriSec))
	{

		map<SOCKET,long>::iterator pos;
		int iIdleTime = 0;
		int nTimeLen = 0;
		for(pos = m_mapAcceptSockQueue.begin();pos!=m_mapAcceptSockQueue.end();)
		{
			nTimeLen = sizeof(iIdleTime);
			//getsockopt(pos->first,SOL_SOCKET,SO_CONNECT_TIME,(char *)&iConnectTime, &nTimeLen);//��ֻ�ܼ��ӿ�ʼ���ӵ����ڵ�ʱ�䣬�����socket���м������ݽ��ջ��ͣ���Ȼ��Ͽ�����
			getsockopt(pos->first,SOL_SOCKET, SO_GROUP_PRIORITY, (char *)&iIdleTime, &nTimeLen);
			if(iIdleTime == -1)	//������socket
			{
				++pos;
				continue;
			}
			if((int)time(NULL) - iIdleTime > MAX_CONNECT_TIMEOUT) //timeout value is 2 minutes.
			{
				printf("�ͻ���: %d ��ʱ,ϵͳ���رմ�����.\n",pos->first);
			    CancelIo((HANDLE)pos->first);
				shutdown(pos->first,SD_BOTH); 
				closesocket(pos->first);
				m_mapAcceptSockQueue.erase(pos++);
			}
			else
			{
				++pos;
			}
		}
		LeaveCriticalSection(&m_struCriSec);
	}
}
//���߼��,ͨ�������������
int CIocpServer::SetHeartBeatCheck(SOCKET socket)
{
	DWORD dwError = 0L,dwBytes = 0;
	TCP_KEEPALIVE1 sKA_Settings = {0},sReturned = {0};
	sKA_Settings.onoff = 1 ;
	sKA_Settings.keepalivetime = 3000 ; // Keep Alive in 5.5 sec.
	sKA_Settings.keepaliveinterval = 1000 ; // Resend if No-Reply

	dwError = WSAIoctl(socket, SIO_KEEPALIVE_VALS, &sKA_Settings,sizeof(sKA_Settings), &sReturned, sizeof(sReturned),&dwBytes,NULL, NULL);
	if(dwError == SOCKET_ERROR )
	{
		dwError = WSAGetLastError();
		//m_pLogFile->LogEx("SetHeartBeatCheck->WSAIoctl()��������,�������: %ld",dwError);	
		return FALSE;
	}
	return TRUE;
}
void CIocpServer::PrintDebugMessage(CSrvException& e,bool log/*=true*/)
{
    ostringstream message;
	message<<e.GetExpDescription()<<endl;
	printf(message.str().c_str());
	if(log)
	{
		if(m_pLogFile)
			m_pLogFile->Log(message.str().c_str());
	}
	else
	{
		//MessageBox(NULL,message.str().c_str(),"�������쳣",MB_OK|MB_ICONERROR); 
	}
}
void CIocpServer::PrintDebugMessage(const char* str,int errorCode/*=-1*/,bool log/* = true*/)
{
	ostringstream message;
	message<<str<< ":" <<errorCode << endl;


	if(log)
	{
		if(m_pLogFile)
			m_pLogFile->Log(message.str().c_str());
	}
	else
	{
		//MessageBox(NULL,message.str().c_str(),"�������쳣",MB_OK|MB_ICONERROR);
	}

}