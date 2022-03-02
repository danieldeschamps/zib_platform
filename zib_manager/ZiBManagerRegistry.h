#ifndef _ZIB_MANAGER_REGISTRY_H_
#define _ZIB_MANAGER_REGISTRY_H_

#include <registry/Registry.h>
#include <misc/CriticalSection.h>

// Class to get the ZiB Manager registry settings. It's Thread Safe.
class CZiBManagerRegistry
{
private:
	struct Settings;
public:
	CZiBManagerRegistry();
	~CZiBManagerRegistry();

	bool RefreshRegistrySettings(void);
	bool DeleteRegistrySettings(void);
	Settings Get(void);
	
private:
	CRegistry m_reg;

	class lock
	{
	public:
		lock()
		{
			m_cs.lock();
		}

		virtual ~lock()
		{
			m_cs.unlock();
		}
	protected:
		static CCriticalSection m_cs;
	};

	struct Settings
	{
		// Gateway comm settings
		string	strGatewayComPort;
		DWORD	dwGatewayComRate;

		// Modem comm settings
		string	strModemComPort;
		DWORD	dwModemComRate;

		// Database settings
		string	strDbServerName;
		string	strDbUserName;
		string	strDbPassword;
		string	strDbPath;
		string	strDbBackupPath;

		// Internal settings
		DWORD	dwManagerJobTimeBase;
		DWORD	dwManagerJobDiscoveryLatency;
		DWORD	dwManagerJobAttributesHandleLatency;
		DWORD	dwManagerJobCheckAlarmLatency;
		DWORD	dwManagerDebugMask;

		// Alarm settings
		string	strAlarmSmsNumber;
		string	strAlarmEmail;
		string	strAlarmSmtpServer;
		DWORD	dwAlarmMaxSendCount;
		DWORD	dwAlarmRepeatLatency;
	} m_settings;
};

#endif // _ZIB_MANAGER_REGISTRY_H_