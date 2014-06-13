#pragma once

#include <string>
#include <map> 
#include <iostream>
using namespace std;

// std::string Util::bytes_to_hexstr(unsigned char* first, unsigned char* last)
// {    
// 	std::ostringstream oss;    
// 	oss << std::hex << std::setfill('0');    
// 	while(first<last) 
// 		oss << std::setw(2) <<  int(*first++) << " ";    
// 	return oss.str();
// }


typedef struct Common_Position
{
	char gpsid[20] ;//GPSID
	bool valid ;   //��Ч��
	bool alarm ;   //����
	float lat ;    //γ��
	float lon ;    //����
	float height ; //�߳�
	float speed ;  //�ٶ�
	float heading; //����
	char  gpstime[20] ;  //ʱ��


}Common_Position;


/**
* �ص��ӿ�����
*/
interface IPositionServer
{
	virtual void ReceivePosition(const Common_Position &cPos) = 0  ; 
	virtual void SendToTerminal(SOCKET client , CHAR *pData , ULONG datalen ) = 0  ;
};