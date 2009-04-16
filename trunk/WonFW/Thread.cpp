/**********************************************************************************
    ���� : ad_pas
	C++��: CThread
	���� : Windows �� Linux ���õ��̴߳�����
	Author:�ž۳�

**********************************************************************************/
#include "StdAfx.h"
#include <process.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CThread::CThread()
{
#ifdef _WIN32
	m_uThreadID = (UINT32)-1;
#else
	m_pthread   = (pthread_t)-1;
#endif
}

CThread::~CThread()
{

}

UINT32 CThread::CreateThread(void *pCallBackFun, void *pParam)
{
#ifdef _WIN32
	m_uThreadID = _beginthread((void (__cdecl *)(void *))pCallBackFun,0,pParam);
#else
	pthread_create(&m_pthread,NULL,(void*(*)(void*))pCallBackFun,pParam);
#endif
	return 0;
}
