#ifndef _SYSTEM_SERVICE_H_
#define _SYSTEM_SERVICE_H_

#include "ZiBManager.h"

// This class implements the functionality of a WIN32 System Service
class CSystemService  
{
public:
	CSystemService();
	virtual ~CSystemService();

	// Setup service name, display name, and initial status
	void Init(LPCTSTR pServiceName,LPCTSTR pServiceDisplayedName);

	// Runs the service - Called either by the SCM or when the executable is called from the command line.
    void Start();

	// Call
	void ServiceMain();
    void Handler(DWORD dwOpcode);
    void Run();
    BOOL IsInstalled();
    BOOL Install(CZiBManager::EMangerInstallComponents eComponents = CZiBManager::EMangerInstallComponentsAll);
    BOOL Uninstall(CZiBManager::EMangerInstallComponents eComponents = CZiBManager::EMangerInstallComponentsAll, DWORD dwTimeout = 10000);
	void LogEvent(LPCSTR pszFormat, ...);
    void SetServiceStatus(DWORD dwState);

private:
	static void WINAPI _ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv);
    static void WINAPI _Handler(DWORD dwOpcode);

	DWORD m_dwThreadID;

	CZiBManager * m_pManager;

public:
    TCHAR m_szServiceName[256];
	TCHAR m_szServiceDisplayedName[256];
    SERVICE_STATUS_HANDLE m_hServiceStatus;
    SERVICE_STATUS m_status;
	BOOL m_bService;
	BOOL m_bStop;
};

extern CSystemService _Module;
#endif // _SYSTEM_SERVICE_H_
