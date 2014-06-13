#pragma once
#include "positionunit.h"

#include <algorithm>


template <class T>
void endswap(T *objp)
{
	unsigned char *memp = reinterpret_cast<unsigned char*>(objp);
	std::reverse(memp, memp + sizeof(T));
}




#pragma pack( push ,1 )
enum {

	/**
	*	�ն�ע��
	*/
	JTT_REGISTER = 0x0100,
	JTT_REGISTER_REPLY = 0X8100,

	/**
	*	ע��
	*/
	JTT_REGOUT = 0x0003,
	/**
	*  ��Ȩ
	*/
	JTT_CHECK_AUTH = 0x0102 ,

	/**
	* ƽ̨ͨ����Ϣ
	*/
	JTT_PLAT_GENERAL_REPLY = 0x8001,
	JTT_TERM_GENERAL_REPLY = 0x0001 ,
	/**
	* ������Ϣ
	*/
	JTT_TERM_HEART = 0X0002 ,

	/*�ն�λ����Ϣ�ϱ�*/
	JTT_TERM_POSITION = 0x0200 

};



/**
* MsgProp˵��
* 15	14 | 13	| 12	11	10	   | 9	8	7	6	5	4	3	2	1	0
����	    �ְ�  ���ݼ��ܷ�ʽ	           ��Ϣ�峤��
*/
/*JTTЭ��ͷ*/
typedef struct JT808Protocol
{
	enum { HEADERSIZE = 13 };
	BYTE    Sign[1] ;     /**<Э��ͷ0x7e*/
	WORD    Cmd;          /**<��ϢID*/  
	WORD    Mask ;        /**<��Ϣ������*/
	BYTE    SimNo[6] ;   /**<�ֻ���*/ 
	WORD    SeqNo   ;    /**<��Ϣ��ˮ��*/

	BYTE    MsgBd[2] ;   /*��Ϣ��*/	
	/**
	* ��Ϣ�峤��
	*/
	WORD    GetMsgBodyLength()
	{	
		WORD ulen = Mask ; 
		endswap(&ulen) ; 
		ulen =  0X01FF & ulen  ; 
		return ulen;
	}
	/**С�ֽ������ó���*/
	void   SetMsgBodyLength(WORD wLen)
	{		
		endswap(&wLen) ; 
		WORD uProp = 0x00FC & Mask ; 
		Mask = uProp | wLen ; 
	}

	WORD    GetCmd()
	{
		WORD wcmd = Cmd ; 
		endswap(&wcmd) ;
		return wcmd ; 
	}
	void SetCmd(WORD cmd) 
	{
		Cmd = cmd ; 
		endswap(&Cmd) ; 
	}

	BYTE    GetCheckCode()
	{
		if(IsMultiPacket())
		{
			return 0 ; //��ʱ������ְ������
		}
		else  //���ְ�
		{
			return  *( Sign + 13 + GetMsgBodyLength()) ;
		}
	}

	/**
	* �Ƿ�ְ�
	*/
	bool IsMultiPacket()
	{
		WORD uData = Mask ; 
		endswap(&uData) ; 

		WORD uVal = 0x2000 & uData ; 
		if( uVal == 0 )
			return false ; 
		return true ; 
	}
	/**
	* �Ƿ����
	*/
	bool IsEncrypted()
	{
		WORD uData = Mask ; 
		endswap(&uData) ; 
		WORD uVal = 0x1c00 & uData ; 
		if( uVal == 0 )
			return false ; 
		return true ; 
	}

	
}JT808Protocol;


/**�ն�ע��ṹ*/
typedef struct JT808Body_RegisterUP
{
	WORD shen ;            //ʡ
	WORD shi ;             //����
	BYTE manufacturer[5] ; //������id
	BYTE terminaltype[8]; //�ն�����
	BYTE terminalID[7] ; //�ն�ID
	BYTE color[1] ;   //������ɫ
	BYTE plateID[1] ; //���ƺ�
}JT808Body_RegisterUP;


/**λ�û�����Ϣ*/
typedef struct JT808Body_PositionUP
{

	DWORD alarmstate ; //������־λ
	DWORD posstate ;   //״̬��־λ
	DWORD lat ; //γ��
	DWORD lon ; //����
	WORD  height ; //�߳�
	WORD  speed ;   //�ٶ�
	WORD  heading ;//����
	BYTE  bcdtime[6] ;//ʱ��

}JT808Body_PositionUP;


BYTE MakeCheck(BYTE *psrc, UINT16 ilen) ;
BYTE MakeCheckProtocol(JT808Protocol *pData ) ; 

#pragma pack( pop )


class CJT808Unit :
	public CPositionUnit
{
public:

	/** GPS���� */
	enum { MAX_PACKET_LEN  = 1024 };

	CJT808Unit(IPositionServer *pServer);
	virtual ~CJT808Unit(void);
	virtual bool ReceiveData( char *buffer , int bufferlen)  ;
	virtual void ProcessData();


protected:
	
private:
	CJT808Unit(void);

    /*Ԥ���建��������ߴ���Ч��*/
	char mchOneBuffer[MAX_PACKET_LEN];      // һ��ת���İ�����
	int  miOnePtr ;                         //ת���ָ��
	char mchRawOneBuffer[MAX_PACKET_LEN]  ; //һ��δת��ԭʼ������
	int  miRawPtr ;                         //ԭʼָ��

	char mchReplyOneBuffer[MAX_PACKET_LEN]  ;    //һ��δת�����а�����
	int  miReplyOnePtr ;                            //δת�����а�ָ��
	char mchReplyRawOneBuffer[MAX_PACKET_LEN]  ; //һ��ת������а�����
	int  miReplyRawPtr ;                         //ת������а�ָ��
	
	/*�ն�ID*/
	char TerminalID[8] ;  //
	
private:
	bool GetOneRawPacket();
	bool GetOnePacket() ;    
	void UpBufferTransfer();  //���а�ȥ��ת��
	void DownBufferTransfer();//���а�����ת��
	void ProcessPacket(JT808Protocol *pData) ; 
	void SendTerminal();

private:
	/*�ն�ע��Ӧ��*/
	void PacketRegisterReply();
	/*ƽ̨ͨ��Ӧ��*/
	void PacketGeneralReply();

	/*λ����Ϣ����*/
	void ProcessPosition(JT808Protocol *pData);

	/*Util  : BCD->NUM */
	BYTE BCDtoNumber(const BYTE& bcd) ; 


};
