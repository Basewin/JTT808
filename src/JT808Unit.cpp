#include "StdAfx.h"
#include "JT808Unit.h"

#include <string>
#include <map> 
#include <iostream>
using namespace std ; 

#pragma region ����У��
BYTE MakeCheck(BYTE *psrc, UINT16 ilen) 
{
	BYTE btResult = psrc[0] ; 
	for(int i=1; i<ilen ; i++ )
	{
		btResult ^= psrc[i] ;
	}
	return btResult ; 

}

BYTE MakeCheckProtocol(JT808Protocol *pData )
{
	return MakeCheck(pData->Sign +1  , pData->GetMsgBodyLength()  + JT808Protocol::HEADERSIZE-1) ;
}

#pragma endregion

CJT808Unit::CJT808Unit(IPositionServer *pServer) : CPositionUnit(pServer),miOnePtr(0) , miRawPtr(0)
{	
}

CJT808Unit::~CJT808Unit(void)
{
}

bool CJT808Unit::ReceiveData( char *buffer , int bufferlen) 
{		
	return CPositionUnit::ReceiveData(buffer , bufferlen) ; 
}



#pragma region ��Ч���ж�
/*��ȡһ����ͷβ��ʶ��ԭʼ��*/
bool CJT808Unit::GetOneRawPacket()
{

	miRawPtr = 0 ; 

	int iPos = 0 ; 
	ScanInvalidData() ; 

	/*�쳣�̰�*/
	if( _dataptr <  JT808Protocol::HEADERSIZE ) 
		return false;


	for( int i = 1 ; i < _dataptr ; i++ )
	{
		/*��ȡ��һ������ͷβ��־λ�İ�*/
		if( _buffer[i] == 0x7e )
		{
			iPos = i ; 
			memcpy( mchRawOneBuffer , _buffer , iPos+1 ) ;
			miRawPtr = iPos + 1 ; 
			Scan(iPos+1) ; 
			return true ; 
		}
	}
	/*�쳣����*/
	if( _dataptr > MAX_PACKET_LEN )
		Scan(_dataptr) ; 
	return false ; 
}
/**
* ��ȡһ����Ч��
*/
bool CJT808Unit::GetOnePacket()
{
	
	if( !GetOneRawPacket() )
		return false ; 
	/*�ָ�ת��*/
	

	UpBufferTransfer();
	miRawPtr = 0  ;

	/*У��*/
	JT808Protocol *pOnePacket = (JT808Protocol*)mchOneBuffer ; 
	if( MakeCheckProtocol(pOnePacket) == pOnePacket->GetCheckCode() )
		return true ;	

	miOnePtr = 0 ; 
	return false ;
}

#pragma endregion

#pragma region ת�崦��
void CJT808Unit::UpBufferTransfer()
{
	miOnePtr = 0 ; 
	int iTempPos = 0 ; 
	int i = 0 ;  	
	for(  i = 0 ,iTempPos = 0 ; i< miRawPtr-1 ; iTempPos ++ )
	{
		if( mchRawOneBuffer[i] == 0x7d && mchRawOneBuffer[i+1] == 0x02) {
			mchOneBuffer[iTempPos] = 0x7e ;	i+=2 ; 			
		}
		else if( mchRawOneBuffer[i] == 0x7d && mchRawOneBuffer[i+1] == 0x01 ){
			mchOneBuffer[iTempPos] = 0x7d ;	i+=2;
		}
		else{		mchOneBuffer[iTempPos] = mchRawOneBuffer[i] ; i++ ; }
	}
	mchOneBuffer[iTempPos] = mchRawOneBuffer[i] ;  //0x7e
	miOnePtr = iTempPos ;
}

void CJT808Unit::DownBufferTransfer()
{
	miReplyRawPtr = 0 ; 
	int iTempPos = 0 ; 
	int i = 0 ;  
	mchReplyRawOneBuffer[0] = 0x7e ; 
	for(  i = 1 ,iTempPos = 1 ; i< miReplyOnePtr ; i ++ )
	{
		if( mchReplyOneBuffer[i] == 0x7e ) {
			mchReplyRawOneBuffer[iTempPos] = 0x7d ; mchReplyRawOneBuffer[iTempPos+1] = 0x02;    iTempPos+=2 ; 			
		}	
		else if( mchReplyOneBuffer[i] == 0x7d ) {
			mchReplyRawOneBuffer[iTempPos] = 0x7d ; mchReplyRawOneBuffer[iTempPos+1] = 0x01 ;   iTempPos+=2 ; 			
		}	
		else{		mchReplyRawOneBuffer[iTempPos] = mchReplyOneBuffer[i] ; iTempPos++ ; }
	}

	mchReplyRawOneBuffer[iTempPos] = mchReplyOneBuffer[i] ;  //0x7e
	miReplyRawPtr = iTempPos ;
}

#pragma endregion


//BYTE   _buffer[BUFFER_SIZE] ; /**< ������   */
//int    _dataptr ;             /**< ����ָ�� */

void CJT808Unit::ProcessData()
{	
	while(GetOnePacket())
	{
		JT808Protocol *pOnePacket = (JT808Protocol*)mchOneBuffer ; 
		ProcessPacket(pOnePacket) ;		
	}
}


/*������Ϣ���ն�*/
void CJT808Unit::SendTerminal()
{
	m_pServer->SendToTerminal(m_s , mchReplyRawOneBuffer , miReplyRawPtr + 1) ; //+��Ϊ����
}


/**
* �������Ϣ
*/
void CJT808Unit::ProcessPacket(JT808Protocol *pData)
{
	switch( pData->GetCmd() )
	{

	case JTT_REGISTER:
		{
			TRACE0("JTT_REGISTER\n") ; 
			PacketRegisterReply(); //�����
			DownBufferTransfer() ; //ת��
			SendTerminal(); //����		
			
		}
		break;
	case JTT_REGOUT:
		{
			TRACE0("JTT_REGOUT\n") ; 
			PacketGeneralReply(); //�����
			DownBufferTransfer() ; //ת��
			SendTerminal(); //����		
		}
		break;
	case JTT_CHECK_AUTH:
		{
			TRACE0("JTT_CHECK_AUTH\n") ; 
			PacketGeneralReply(); //�����
			DownBufferTransfer() ; //ת��
			SendTerminal(); //����		
			
			//����λ����Ϣ


		}
		break;
	case JTT_TERM_GENERAL_REPLY:
		{
			TRACE0("JTT_TERM_GENERAL_REPLY\n") ; 
		}
		break;
	case JTT_TERM_HEART:
		{
			TRACE0("JTT_TERM_HEART\n") ; 
			PacketGeneralReply(); //�����
			DownBufferTransfer() ; //ת��
			SendTerminal(); //����		

		}
		break;
	case JTT_TERM_POSITION:
		{
			TRACE0("JTT_TERM_POSITION\n") ;
			PacketGeneralReply(); //�����
			DownBufferTransfer() ; //ת��
			SendTerminal(); //����		

			
			ProcessPosition(pData) ; 


		}
		break;
	case JTT_TERM_POSITION+1:
		{
			TRACE0("JTT_TERM_POSITION+1\n") ;
			ProcessPosition(pData) ; 
		}
		break;
	default:
		TRACE0("JTT_UNKOWN\n") ;
		break;
	}
	
}

BYTE CJT808Unit::BCDtoNumber(const BYTE& bcd)
{
	return (0xff & (bcd>>4))*10 +(0xf & bcd);
}

void CJT808Unit::ProcessPosition(JT808Protocol *pData)
{

	 JT808Body_PositionUP *pPosInfo =(JT808Body_PositionUP *)pData->MsgBd ; 
	 Common_Position cp;

	 DWORD dwTemp = 0 ; 

	 dwTemp = pPosInfo->posstate ; 
	 endswap(&dwTemp) ; 	
	 cp.valid = (( dwTemp & 0x2 ) != 0 )  ;

	  cp.alarm = false ;

	 dwTemp = pPosInfo->lat ; 
	 endswap(&dwTemp) ;
	 cp.lat = dwTemp / 1000000.0f ;

	 dwTemp = pPosInfo->lon; 
	 endswap(&dwTemp) ;
	 cp.lon = dwTemp / 1000000.0f ;

	 WORD wTemp = pPosInfo->height ; 
	 endswap(&wTemp) ;
	 cp.height = wTemp ; 

	 wTemp = pPosInfo->heading ; 
	 endswap(&wTemp) ;
	 cp.heading = wTemp ;

	 wTemp = pPosInfo->speed ; 
	 endswap(&wTemp) ;
	 cp.speed = wTemp / 10.0f;
	 
	 sprintf_s( cp.gpstime , "%4d-%02d-%02d %02d:%02d:%02d\0" , BCDtoNumber(pPosInfo->bcdtime[0])+2000 , 
		 BCDtoNumber(pPosInfo->bcdtime[1]),
		 BCDtoNumber(pPosInfo->bcdtime[2]),
		 BCDtoNumber(pPosInfo->bcdtime[3]),
		 BCDtoNumber(pPosInfo->bcdtime[4]),
		 BCDtoNumber(pPosInfo->bcdtime[5])
		 );

	 sprintf_s( cp.gpsid , "%02d%02d%02d%02d%02d%02d\0" , 
		 BCDtoNumber(pData->SimNo[0]), 
		 BCDtoNumber(pData->SimNo[1]),
		 BCDtoNumber(pData->SimNo[2]),
		 BCDtoNumber(pData->SimNo[3]),
		 BCDtoNumber(pData->SimNo[4]),
		 BCDtoNumber(pData->SimNo[5])
		 );


	 m_pServer->ReceivePosition(cp);
}


#pragma region ���췴����Ϣ
/*����ע��ظ���Ϣ*/
void CJT808Unit::PacketRegisterReply()
{
	miReplyOnePtr = 0;
	memcpy( mchReplyOneBuffer , mchOneBuffer , JT808Protocol::HEADERSIZE) ; 
	JT808Protocol *pReply = (JT808Protocol *)mchReplyOneBuffer ; 
	pReply->SetCmd(0x8100) ; 
	WORD dwSeqNo = pReply->SeqNo;   //Ӧ����ˮ��		
	memcpy( pReply->MsgBd ,  &dwSeqNo ,2 ) ; //
	*(pReply->MsgBd+2) = 0 ; 
	memcpy( pReply->MsgBd + 3 ,  "YCGIS",6 ) ; //
	pReply->SetMsgBodyLength(9) ; 
	*(pReply->MsgBd +9) =  MakeCheckProtocol(pReply) ; 
	*(pReply->MsgBd +10) = 0x7e ; 
	miReplyOnePtr = JT808Protocol::HEADERSIZE+pReply->GetMsgBodyLength()+1 ; //һ���ֽڵ�β��־
}

/*����ƽ̨ͨ��Ӧ����Ϣ*/
void CJT808Unit::PacketGeneralReply()
{
	miReplyOnePtr = 0;
	memcpy( mchReplyOneBuffer , mchOneBuffer , JT808Protocol::HEADERSIZE) ; 
	JT808Protocol *pReply = (JT808Protocol *)mchReplyOneBuffer ; 
	pReply->SetCmd(0x8001) ; 
	WORD dwSeqNo = pReply->SeqNo;   //Ӧ����ˮ��		
	memcpy( pReply->MsgBd ,  &dwSeqNo ,2 ) ; //

	WORD dwCmd = pReply->Cmd;   //Ӧ����ˮ��		
	memcpy( pReply->MsgBd +2 ,  &dwCmd ,2 ) ; //
	*(pReply->MsgBd +4) = 0 ; 
	pReply->SetMsgBodyLength(5) ;

	*(pReply->MsgBd +5) =  MakeCheckProtocol(pReply) ; 
	*(pReply->MsgBd +6) = 0x7e ; 
	miReplyOnePtr = JT808Protocol::HEADERSIZE+ pReply->GetMsgBodyLength() + 1 ; //һ���ֽڵ�β��־
}

#pragma endregion


