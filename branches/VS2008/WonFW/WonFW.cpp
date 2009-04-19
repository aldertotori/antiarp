// WonArpFW.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "WonFW.h"
#include "WonFWDlg.h"
#include "Shlwapi.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BOOL					g_StartupHide = FALSE;

/////////////////////////////////////////////////////////////////////////////
// CWonArpFWApp

BEGIN_MESSAGE_MAP(CWonArpFWApp, CWinApp)
	//{{AFX_MSG_MAP(CWonArpFWApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWonArpFWApp construction

CWonArpFWApp::CWonArpFWApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CWonArpFWApp object

CWonArpFWApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CWonArpFWApp initialization

BOOL CWonArpFWApp::InitInstance()
{
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

	//���Windows�汾����ͬ��IO����
	BOOL  IsVista = FALSE;
	
	OSVERSIONINFOEX osvi;
	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	
	GetVersionEx((OSVERSIONINFO*)&osvi);
	switch (osvi.dwPlatformId)
	{
				case VER_PLATFORM_WIN32_NT:
					{
						if( osvi.dwMajorVersion >= 6 )
						{
							IsVista = TRUE;
						}

					}
					break;
				default:
					break;
	}

	//�������������Dll��
	if(InitObjects() == FALSE)
	{
		MessageBox(NULL,TEXT("�����ʼ��ʧ��"),TEXT("��� AntiArp"),MB_OK | MB_ICONERROR);
		return FALSE;
	}

	char		   szMiniprotInf[MAX_PATH];
	char		   szProtocolInf6[MAX_PATH];
	char		   szProtocolInf5[MAX_PATH];
	char		   szCurrentPath[MAX_PATH];
	char		   szPathDestMini[MAX_PATH];
	char		   szClearFile[MAX_PATH];

	PGLOBAL_CONFIG CurrentConfig;

	g_Config->GetCurrentConfig(&CurrentConfig);

	LPCWSTR pwszCmdLine = NULL;

	LPWSTR * argv = NULL;
	int		  argc = 0;

	pwszCmdLine = GetCommandLineW();

	argv = CommandLineToArgvW(pwszCmdLine,&argc);

	GetCurrentDirectoryA(MAX_PATH,szCurrentPath);

	GetWindowsDirectory(szPathDestMini,MAX_PATH);		
	lstrcat(szPathDestMini,"\\inf\\");
	lstrcat(szPathDestMini,"MPWONARP.inf");

	if(argc > 1)
	{
		// /Auto  ����������ʱ�Զ����ص�״̬��
		if(_wcsnicmp(argv[1],L"/Auto",wcslen(L"/Auto")) == 0)
		{
			//�����ֹ����������رձ�����
			if(!CurrentConfig->SYSTEM_BOOT_AUTOSTART)
			{
				return FALSE;
			}
			g_StartupHide = TRUE;
		}
		// /DisableSign ��������ǩ��
		else if(_wcsnicmp(argv[1],L"/DisableSign",wcslen(L"/DisableSign")) == 0)
		{
			//�����ֹ����������رձ�����
			g_Install->DriverSignCheck(FALSE);
			return TRUE;
		}
		// /EnableSign  ��������ǩ��
		else if(_wcsnicmp(argv[1],L"/EnableSign",wcslen(L"/EnableSign")) == 0)
		{
			//�����ֹ����������رձ�����
			g_Install->DriverSignCheck(TRUE);
			return TRUE;
		}
		// /Install   ���ڰ�װ����
		else if(_wcsnicmp(argv[1],L"/Install",wcslen(L"/Install")) == 0)
		{
			lstrcpy(szMiniprotInf,szCurrentPath);
			lstrcat(szMiniprotInf,"\\MPWONARP.inf");

			if(!IsVista) // 2k/xp/2003
			{
				CopyFile(szMiniprotInf,szPathDestMini,FALSE);
				lstrcpy(szProtocolInf5,szCurrentPath);
				lstrcat(szProtocolInf5,"\\PTWONARP.inf");
				g_Install->InstallNetServiceDriver(szProtocolInf5);
			}
			else // Vista
			{
				lstrcpy(szProtocolInf6,szCurrentPath);
				lstrcat(szProtocolInf6,"\\WonArp6.inf");
				g_Install->InstallNetServiceDriver(szProtocolInf6);
			}
			
			//Vista �����ļ�
			lstrcpy(szClearFile,szCurrentPath);
			lstrcat(szClearFile,"\\WonArp6.sys");
			DeleteFile(szClearFile);
			lstrcpy(szClearFile,szCurrentPath);
			lstrcat(szClearFile,"\\WonArp6.inf");
			DeleteFile(szClearFile);
			//Windows 2k/XP/2003 �����ļ�
			lstrcpy(szClearFile,szCurrentPath);
			lstrcat(szClearFile,"\\WonArp.sys");
			DeleteFile(szClearFile);
			lstrcpy(szClearFile,szCurrentPath);
			lstrcat(szClearFile,"\\PTWONARP.inf");
			DeleteFile(szClearFile);
			lstrcpy(szClearFile,szCurrentPath);
			lstrcat(szClearFile,"\\MPWONARP.inf");
			DeleteFile(szClearFile);

			return FALSE;
		}
		// /Remove	  ����ж�س���
		else if(_wcsnicmp(argv[1],L"/Remove",wcslen(L"/Remove")) == 0)
		{
			if(!IsVista)
			{
				if(g_Install->RemoveNetServiceDriver(TEXT("ras_arpim")) == S_OK)
				{
					//��Miniprot�ļ���infĿ¼ɾ��
					DeleteFile(szPathDestMini);
					PathRenameExtension(szPathDestMini,TEXT(".PNF"));
					DeleteFile(szPathDestMini);
				}
			}
			else
			{
				g_Install->RemoveNetServiceDriver(TEXT("MS_WonArp"));
			}
			return TRUE;
		}
	}

	GlobalFree(argv);

	HANDLE h_mutex  = CreateMutex(NULL,TRUE,TEXT("WonArpFW_Process_Mutex"));
	if(h_mutex != NULL)
	{
		if(GetLastError() == ERROR_ALREADY_EXISTS)
		{
			CloseHandle(h_mutex);
			h_mutex = NULL;
			MessageBox(NULL,TEXT("��� AntiArp ��������..."),TEXT("��� AntiArp"),MB_OK|MB_ICONINFORMATION);
			TerminateProcess(GetCurrentProcess(),-1);
			return FALSE;
		}
	}

	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(2,2);

	int err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 ) 
	{
		MessageBox(NULL,TEXT("Socket ����ӿڳ�ʼ��ʧ��"),TEXT("��� AntiArp"),MB_OK | MB_ICONERROR);
		return FALSE;
	}

#if !DBGTestGUI

	//��������Ƿ����
	if(g_ArpMgr->Open() != S_OK)
	{
		MessageBox(NULL,TEXT("����ǽ����ģ�鶪ʧ�������°�װ������"),
			TEXT("��� AntiARP"),MB_OK | MB_ICONERROR);
		return FALSE;
	}

#else

	MessageBox(NULL,TEXT("����ǽ����ģ��δ���أ���������!!"),
		TEXT("�������!!"),MB_OK | MB_ICONERROR);

#endif

	// ���ؼ��ɹ�
	CWonArpFWDlg dlg;
	m_pMainWnd = &dlg;
	
	int nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}

	g_ArpMgr->Close();

	WSACleanup();

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

