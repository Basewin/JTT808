#pragma once

#include "AutoLock.h"
#include <string>
#include "IServerDefine.h"
using namespace std ; 


/**
* 
    SYSTEMTIME tm1;
    _stscanf( msg.gpstime, _T("%4d-%2d-%2d %2d:%2d:%2d"),   
	&tm1.wYear, 
	&tm1.wMonth, 
	&tm1.wDay, 
	&tm1.wHour, 
	&tm1.wMinute,
	&tm1.wSecond );

	double dt;
	SystemTimeToVariantTime(&tm1,&dt);
*/
// /*����GPS��Ϣ�ṹ*/
// typedef struct UP_GPSDATA 
// {
// 	CHAR  chTID[20] ;//GPSid
// 	float fLat ;
// 	float fLon ; 
// 	float fspeed ; 
// 	float fheading ; 
// };

//int BCDToInt(byte bcd) //BCDתʮ����
//{
//	return (0xff & (bcd>>4))*10 +(0xf & bcd);
//}
//
//int IntToBCD(byte int) //ʮ����תBCD
//{
//	return ((int/10)<<4+((int%10)&0x0f);
//}

//��֤����� ����json����õ�ѡ��


class CPositionUnit
{
public :
	CPositionUnit(IPositionServer *pServer);
	enum{ BUFFER_SIZE = 40960 }; /**< ��������С  */
	enum{ HEART_BEAT_LIMIT = 1 };    /**< ���� ������ */

	virtual ~CPositionUnit(void);

public : //����
	void SetInitParam( SOCKET s ,char* ip,USHORT port) ; 
	virtual bool ReceiveData( char *buffer , int bufferlen);
	virtual void ProcessData() ;

	bool IsLostConnect();

	SOCKET getSocket()  { return m_s ; }
	const string &getIP() { return m_ip ; }
	USHORT getPort() {return m_port ; }	

protected:
	CPositionUnit(void);
	void ScanInvalidData() ;
	void Scan(int iScan = 1) ; 	

	SOCKET m_s ; 
	string m_ip ; 
	USHORT m_port ;

	/**
	*
	*/
	time_t _lastrecvtime ;     /**< ������ʱ�� */
	time_t _lastsendtime ; /**< �����ʱ�� */
	BYTE   _buffer[BUFFER_SIZE] ; /**< ������ */
	int    _dataptr ;      /**< ����ָ�� */
	BOOL   _isLogon ;      /**< �Ƿ��¼ */
	IPositionServer *m_pServer ; 

};
