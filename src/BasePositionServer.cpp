#include "StdAfx.h"
#include "BasePositionServer.h"

#include "JTT808ServerDlg.h"

CBasePositionServer::CBasePositionServer(void)
{
	m_pFac = new CPositonUnitFactory(this);
}

void CBasePositionServer::SetMainFrame( CJTT808ServerDlg *pDlg) 
{
	
	m_pDlg = pDlg ; 

}
CBasePositionServer::~CBasePositionServer(void)
{
	if( m_pFac != NULL )
	{
		delete m_pFac  ; 
		m_pFac = NULL ; 
	}	
	
	ReleaseAllUnits() ;
}

/*
*	�µĿͻ�������
*/
void CBasePositionServer::TcpAcceptComplete(SOCKET client,char* ip,unsigned short port)
{	
	AddUnit(client, ip, port);
}
/*�����¼�*/
void CBasePositionServer::TcpSendComplete(SOCKET client,char* buf,unsigned long data_len)
{
	//TRACE0(" Send Complete !\n") ; 
}
/**
* ���ձ���
*/
void CBasePositionServer::TcpReceiveComplete(SOCKET client,char* buf,unsigned long data_len)
{
	//TRACE0(" Recv Complete !\n") ; 
	CPositionUnit *pUnit = FindUnit(client) ; 
	if( pUnit == NULL )
		return ;
	CAutoLock l(&m_cri);
	pUnit->ReceiveData(buf , data_len) ; 

}


void CBasePositionServer::TcpCloseComplete( SOCKET client ) 
{	
	ReleaseOneUnit(client) ;
}

/**
*  1����ִ��
*/
void CBasePositionServer::TimerSecond() 
{	
#pragma region    ����ʱ�ն�����
	CAutoLock l(&m_cri);
	for(Iterator  it = m_vUnits.begin() ; it != m_vUnits.end() ; it++ )
	{		
		if( (*it)->IsLostConnect() ) 
		{
			CloseSocket((*it)->getSocket()) ; 
			delete (*it);
			m_vUnits.erase(it) ; 
			return ;
		}		
	}
#pragma endregion

}

#pragma region Unit��ز���

BOOL CBasePositionServer::HasUnit(SOCKET client)
{
	for( vector<CPositionUnit*>::iterator it =m_vUnits.begin() ; it != m_vUnits.end() ; it ++ )
	{
		if( (*it)->getSocket() == client )
			return TRUE ; 
	}
	return FALSE ; 
}

void CBasePositionServer::SendToTerminal(SOCKET client , CHAR *pData , ULONG datalen )
{
	TcpSend(client , pData , datalen ) ; 
}

void   CBasePositionServer::ReceivePosition(const Common_Position &cPos)
{

	//103@117.27419299749023@31.871791818227194@0.0@0.0@2014-03-05 14:22:39@0@100@0@9939/10689@0/0@1
	/*GPSID ���� γ�� �ٶ� �Ƕ� ʱ�� GPS״̬ ���� ���������ô洢�� ���ô洢��	�Ƿ���*/
	//if(cPos.valid )
	//{
		char ss[200] = {0} ; 
		sprintf(ss , "%s@%f@%f@%4.1f@%4.1f@%s@0@100@1/1@1/1@1" ,cPos.gpsid ,  cPos.lon ,  cPos.lat ,cPos.speed ,cPos.heading,  cPos.gpstime ) ; 
		string str = ss ; 


		m_pDlg->AddLogText(ss) ; 
		m_pDlg->SendToDatabaseServer(str) ; 

	//}
	//else
	//{
	//	TRACE0("Invalid Position\n" ) ; 
	//}

}

void CBasePositionServer::ReleaseAllUnits() 
{
	CAutoLock l(&m_cri);
	for(Iterator  it = m_vUnits.begin() ; it != m_vUnits.end() ; it++ )
	{		
		CloseSocket((*it)->getSocket()) ; 
		delete (*it);
	}
	m_vUnits.clear();
}

void CBasePositionServer::AddUnit( SOCKET client, char* ip, unsigned short port )
{
	/*���Դ������ļ��ж�ȡ�����Unit���ͣ�Ȼ�󴴽�*/
	if( HasUnit(client ))
		return ; 

	CAutoLock l(&m_cri);

	CPositionUnit *pUnit = m_pFac->CreatePositionUnit("JT808") ; 
	pUnit->SetInitParam(client , ip , port) ; 
	m_vUnits.push_back(pUnit) ; 
}

void CBasePositionServer::ReleaseOneUnit(SOCKET client)
{
	CAutoLock l(&m_cri);
	
	for(Iterator  it = m_vUnits.begin() ; it != m_vUnits.end() ; it++ )
	{		
		if( (*it)->getSocket() == client ) 
		{
			CloseSocket((*it)->getSocket()) ; 
			delete (*it);
			m_vUnits.erase(it) ; 
			return ;
		}		
	}
}

CPositionUnit *CBasePositionServer::FindUnit(SOCKET client) 
{
	for( Iterator it = m_vUnits.begin() ; it != m_vUnits.end() ; it++ )
	{
		if( (*it)->getSocket() == client  )
		{
			return (*it);
		}
	}
	return NULL ; 
}

#pragma endregion

#pragma region ���ݴ�����ز���
void CBasePositionServer::ProcessUpData()
{
	
	for( Iterator it = m_vUnits.begin() ; it != m_vUnits.end() ; it++ )
	{		
		CAutoLock l(&m_cri);
		(*it)->ProcessData();
	}
}


int CBasePositionServer::Run()
{
	AfxBeginThread((AFX_THREADPROC)PacketThreadFunc , this) ; 
	return CIocpServer::Run() ; 

}

UINT WINAPI CBasePositionServer::PacketThreadFunc(LPVOID lparam)
{	
	CBasePositionServer  *pServer = (CBasePositionServer* ) lparam ; 
	while(1)
	{	
		if( pServer->m_bStopServer )
		{
			AfxEndThread(0) ; 
			return 1 ; 
		}
		pServer->ProcessUpData() ; 
		Sleep(1) ; 
	}
	return  0 ;  
}

#pragma endregion