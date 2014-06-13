#include "StdAfx.h"
#include "PositionUnit.h"


CPositionUnit::CPositionUnit(IPositionServer *pServer):_dataptr(0)
{
	_lastsendtime = _lastrecvtime = time(NULL) ;
	memset( _buffer , 0 , BUFFER_SIZE  ) ; 
	_isLogon = FALSE ; 
	m_pServer = pServer ; 
}

CPositionUnit::~CPositionUnit(void)
{
}


bool CPositionUnit::IsLostConnect() 
{
	time_t now = time(NULL ) ; 
	if( difftime ( now , _lastrecvtime ) > 60 * HEART_BEAT_LIMIT * 1) /**<����һ����һ��,2����û���յ���Ϣ����Ϊ�Ѿ�����*/
	{
		return true ; 
	}
	return false ; 
}


void CPositionUnit::SetInitParam( SOCKET s ,char* ip,USHORT port )
{
	m_s = s; 
	m_ip  = ip ; 
	m_port = port;	
}
/**
* ���յ�����
*/
bool CPositionUnit::ReceiveData( char *buffer , int bufferlen) 
{
	_lastrecvtime = time(NULL) ; 
	if( bufferlen + _dataptr > BUFFER_SIZE )
		return false ; 
	memcpy( _buffer + _dataptr  , buffer , bufferlen ) ; 
	_dataptr += bufferlen ; 
	return true ; 

}
void CPositionUnit::ProcessData() 
{
	
}



/**
* ����ͷ����ʶ������Ч����
*/
void CPositionUnit::ScanInvalidData() 
{
	int i = 0 ;
	while( _buffer[i] != 0x7e && i < _dataptr ){i++;} ; 
	if( i == _dataptr )
	{
		_dataptr = 0 ; 
		return ; 
	}
	if( i != 0 )
	{
		memcpy( _buffer , _buffer + i ,  _dataptr -i )   ; 
		_dataptr -= i ; 
	}	
}


/**
* ����������iScan�ֽ�
*/
void CPositionUnit::Scan(int iScan )
{
	if( 0 == _dataptr )
		return ;
	memcpy( _buffer , _buffer +iScan ,  _dataptr -iScan )   ; 
	_dataptr -= iScan ; 

}