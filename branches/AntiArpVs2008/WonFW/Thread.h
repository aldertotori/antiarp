#ifndef _THREAD_H
#define _THREAD_H

class CThread  
{
public:
	CThread();
	~CThread();
protected:
	UINT32 CreateThread(void* pCallBackFun,void* pParam);
#ifdef _WIN32
	UINT32		m_uThreadID;		//�߳�ID
	HANDLE		m_hThread;			//�̶߳���
#else
	pthread_t	m_pthread;
#endif
};

#endif


