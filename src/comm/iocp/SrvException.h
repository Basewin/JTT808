#pragma once

#pragma warning( disable : 4290 )  //vc++����������֧���쳣�淶�����Ժ��Դ˾���
#include <iostream>
#include <sstream>
using namespace std;

class CSrvException
{
public:
	CSrvException(void);
	CSrvException(const char* expDescription,int expCode=0);
	~CSrvException(void);
public:
	string GetExpDescription(){return this->m_strExpDescription;};
	int GetExpCode(){return this->m_iExpCode;};
	long GetExpLine(){return this->m_iExpLineNumber;};
private:
	string m_strExpDescription;
	int m_iExpCode;//�쳣����
	long m_iExpLineNumber;//�쳣Դ������������Ե��԰汾��Ч
};

