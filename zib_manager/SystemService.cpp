// Service.cpp: implementation of the CSystemService class.
//
//////////////////////////////////////////////////////////////////////

#include "Stdafx.h"
#include "SystemService.h"

#include <stdio.h>

CSystemService _Module;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSystemService::CSystemService ()
: m_bStop (FALSE)
{

}

CSystemService::~CSystemService ()
{

}

void CSystemService::Init (LPCTSTR pServiceName, LPCTSTR pServiceDisplayedName)
{
    lstrcpy (m_szServiceName,pServiceName);
    lstrcpy (m_szServiceDisplayedName,pServiceDisplayedName);

    // set up the initial service status 
    m_hServiceStatus = NULL;
    m_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    m_status.dwCurrentState = SERVICE_STOPPED;
    m_status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN; // dds: added SERVICE_ACCEPT_SHUTDOWN
    m_status.dwWin32ExitCode = 0;
    m_status.dwServiceSpecificExitCode = 0;
    m_status.dwCheckPoint = 0;
    m_status.dwWaitHint = 0;
}

void CSystemService::Start ()
{
    SERVICE_TABLE_ENTRY st[] =
    {
		{ m_szServiceName, _ServiceMain },
        { NULL, NULL } // end of table
    };
    if (!::StartServiceCtrlDispatcher(st) && m_bService) // Register to the SCM with a Name and a Function
	{
		DWORD dw = GetLastError();
		LogEvent ("StartServiceCtrlDispatcher Error=%d",dw);
		m_bService = FALSE;
	}

	if (m_bService == FALSE)
        Run();
}

// Runs the service code
void CSystemService::ServiceMain ()
{
    // Register the control request handler
    m_status.dwCurrentState = SERVICE_START_PENDING;
    m_hServiceStatus = RegisterServiceCtrlHandler (m_szServiceName, _Handler); // register the control handler, and keep it to be used along with SetServiceStatus
    if (m_hServiceStatus == NULL)
    {
        LogEvent ("Handler not installed");
        return;
    }
    SetServiceStatus (SERVICE_START_PENDING); // set the start pending state

    m_status.dwWin32ExitCode = S_OK;
    m_status.dwCheckPoint = 0;
    m_status.dwWaitHint = 0;

    // When the Run function returns, the service has stopped.
    Run ();

    SetServiceStatus (SERVICE_STOPPED);
    LogEvent ("Service stopped");
}

// Receives commands from SCM
inline void CSystemService::Handler (DWORD dwOpcode)
{
    switch (dwOpcode)
    {
		case SERVICE_CONTROL_STOP:
		case SERVICE_CONTROL_SHUTDOWN:
			LogEvent ("Request to stop or shutdown...");
			SetServiceStatus (SERVICE_STOP_PENDING);
			if (m_pManager)
			{
				m_pManager->Stop();
			}
			else
			{
				LogEvent ("Error while trying to stop the service. The main object seems to be already stoped");
			}
			break;

		/*case SERVICE_CONTROL_PAUSE:
			break;
		case SERVICE_CONTROL_CONTINUE:
			break;
		case SERVICE_CONTROL_INTERROGATE:
			break;*/

		default:
			LogEvent ("Service request no implemented");
			break;
    }
}

// SCM creates a new thread using this function
void WINAPI CSystemService::_ServiceMain (DWORD dwArgc, LPTSTR* lpszArgv)
{
    _Module.ServiceMain ();
}

// SCM sends commands using this function
void WINAPI CSystemService::_Handler (DWORD dwOpcode)
{
    _Module.Handler (dwOpcode); 
}

void CSystemService::SetServiceStatus (DWORD dwState)
{
    m_status.dwCurrentState = dwState;
    ::SetServiceStatus (m_hServiceStatus, &m_status);
}

void CSystemService::Run ()
{
	bool bReset = true;
    LogEvent ("Service started");
	m_dwThreadID = GetCurrentThreadId ();

    if (m_bService)
        SetServiceStatus (SERVICE_RUNNING);

	// Run in a loop until Exit is signaled
	while (bReset)
	{
		// The service is running.
		m_pManager = new CZiBManager;
		switch (m_pManager->Run())
		{
			case CZiBManager::EMangerReturnActionExit:
				bReset = false;
				break;
			case CZiBManager::EMangerReturnActionReset:
				bReset = true;
				break;
			case CZiBManager::EMangerReturnActionError:
				LogEvent("Unable to run the ZiB Manager. Please check the debug log for details");
				bReset = false;
				break;
		}
		delete m_pManager; // stops implicitly the serial port, the rx listener thread and the process message thread
		m_pManager = 0;
	}

	// The service is going to be stopped.
}

BOOL CSystemService::Install (CZiBManager::EMangerInstallComponents eComponents)
{
    if (IsInstalled ())
	{
		printf("The WIN32 System Service '%s' is already installed\n", m_szServiceDisplayedName);
        return TRUE;
	}

	CZiBManager manager;
	if (!manager.Install(eComponents))
	{
		printf("Failure: Couldn't install the '%s' components\n", m_szServiceDisplayedName);
		return FALSE;
	}

    SC_HANDLE hSCM = ::OpenSCManager (NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCM == NULL)
    {
        printf("Failure: Couldn't open SCM\n");
        return FALSE;
    }

    // Get the executable file path
    TCHAR szFilePath[_MAX_PATH];
    ::GetModuleFileName (NULL, szFilePath, _MAX_PATH);

	DWORD dwStartupType = /*SERVICE_AUTO_START*/ SERVICE_DEMAND_START; // dds: changed to demand start
    SC_HANDLE hService = ::CreateService(
        hSCM, m_szServiceName, m_szServiceDisplayedName,
        SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS ,
        dwStartupType, SERVICE_ERROR_NORMAL,
        szFilePath, NULL, NULL, NULL, NULL, NULL);

    if (hService == NULL)
    {
        ::CloseServiceHandle (hSCM);
        printf("Failure: Couldn't create the WIN32 System Service '%s'", m_szServiceDisplayedName);
        return FALSE;
    }

	if (dwStartupType == SERVICE_AUTO_START)
		StartService (hService, 0, NULL);

    ::CloseServiceHandle (hService);
    ::CloseServiceHandle (hSCM);
	
	printf("The WIN32 System Service '%s' was successfully installed\n", m_szServiceDisplayedName);
	return TRUE;
}

BOOL CSystemService::Uninstall (CZiBManager::EMangerInstallComponents eComponents, DWORD dwTimeout)
{
    if (!IsInstalled ())
	{
		printf("The WIN32 System Service '%s' is already uninstalled\n", m_szServiceDisplayedName);
        return TRUE;
	}

    SC_HANDLE hSCM = ::OpenSCManager (NULL, NULL, SC_MANAGER_ALL_ACCESS);

    if (hSCM == NULL)
    {
        printf("Failure: Couldn't open SCM");
        return FALSE;
    }

    SC_HANDLE hService = ::OpenService (hSCM, m_szServiceName, SERVICE_STOP | DELETE | SERVICE_QUERY_STATUS);

    if (hService == NULL)
    {
        ::CloseServiceHandle (hSCM);
        printf("Failure: Couldn't open the WIN32 System Service '%s'", m_szServiceDisplayedName);
        return FALSE;
    }
    SERVICE_STATUS status = {0};
	DWORD dwStartTime = GetTickCount ();

    if (ControlService (hService, SERVICE_CONTROL_STOP, &status))
	{
		// Wait for the service to stop
		while (status.dwCurrentState != SERVICE_STOPPED)
		{
			Sleep (status.dwWaitHint);
			if (!QueryServiceStatus( hService, &status ))
				return FALSE;

			if (status.dwCurrentState == SERVICE_STOPPED)
				break;

			if (GetTickCount() - dwStartTime > dwTimeout)
			{
				MessageBox (NULL,"Service could not be stopped", NULL, MB_OK);
				return FALSE;
			}
		}
	}


	CZiBManager manager;
	if (manager.Uninstall(eComponents))
	{
		if (::DeleteService (hService))
		{
			printf("The WIN32 System Service '%s' was successfully uninstalled\n", m_szServiceDisplayedName);
			return TRUE;
		}
	}

	::CloseServiceHandle (hService);
    ::CloseServiceHandle (hSCM);

    printf("Failed to uninstall the WIN32 System Service '%s'\n", m_szServiceDisplayedName);
    return FALSE;
}

BOOL CSystemService::IsInstalled ()
{
    BOOL bResult = FALSE;

    SC_HANDLE hSCM = ::OpenSCManager (NULL, NULL, SC_MANAGER_ALL_ACCESS);

    if (hSCM != NULL)
    {
        SC_HANDLE hService = ::OpenService (hSCM, m_szServiceName, SERVICE_QUERY_CONFIG);
        if (hService != NULL)
        {
            bResult = TRUE;
            ::CloseServiceHandle (hService);
        }
        ::CloseServiceHandle (hSCM);
    }
    return bResult;
}

///////////////////////////////////////////////////////////////////////////////////////
// Logging functions
void CSystemService::LogEvent (LPCSTR pFormat, ...)
{
    TCHAR   chMsg[256];
    HANDLE  hEventSource;
    LPTSTR  lpszStrings[1];
    va_list pArg;

    va_start (pArg, pFormat);
    vsprintf (chMsg, pFormat, pArg);
    va_end (pArg);

    lpszStrings[0] = chMsg;

    if (m_bService)
    {
        // Get a handle to use with ReportEvent(). 
        hEventSource = RegisterEventSource (NULL, m_szServiceName);
        if (hEventSource != NULL)
        {
            // Write to event log.
            ReportEvent (hEventSource, EVENTLOG_INFORMATION_TYPE, 0, 0, NULL, 1, 0, (LPCTSTR*) &lpszStrings[0], NULL);
            DeregisterEventSource (hEventSource);
        }
    }
    else
    {
        // As we are not running as a service, just write the error to the console.
        printf (chMsg);
    }
}
