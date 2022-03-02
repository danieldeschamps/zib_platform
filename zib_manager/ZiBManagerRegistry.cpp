#include "Stdafx.h"
#include "ZiBManagerRegistry.h"
#include <misc/Debug.h>
#include <serial/Serial.h>

#define REGKEY "Software\\ZiBAutomation\\ZiBManager"

#define REGVALUE_GATEAWAYCOMPORT						"GatewayComPort"
#define REGVALUE_GATEAWAYCOMPORT_DEFAULT				"COM1"
#define REGVALUE_GATEAWAYCOMRATE						"GatewayComRate"
#define REGVALUE_GATEAWAYCOMRATE_DEFAULT				CSerial::EBaudrate::EBaud19200

#define REGVALUE_MODEMCOMPORT							"ModemComPort"
#define REGVALUE_MODEMCOMPORT_DEFAULT					"COM2"
#define REGVALUE_MODEMCOMRATE							"ModemComRate"
#define REGVALUE_MODEMCOMRATE_DEFAULT					CSerial::EBaudrate::EBaud19200

#define REGVALUE_DB_SERVERNAME							"DbServerName"
#define REGVALUE_DB_SERVERNAME_DEFAULT					"localhost"
#define REGVALUE_DB_USERNAME							"DbUserName"
#define REGVALUE_DB_USERNAME_DEFAULT					"SYSDBA"
#define REGVALUE_DB_PASSWORD							"DbPassword"
#define REGVALUE_DB_PASSWORD_DEFAULT					"masterkey"
#define REGVALUE_DB_DATABASEPATH						"DbPath"
#define REGVALUE_DB_DATABASEPATH_DEFAULT				"C:\\ZiBManager\\ZiB Database.fdb"
#define REGVALUE_DB_BACKUPPATH							"DbBackupPath"
#define REGVALUE_DB_BACKUPPATH_DEFAULT					"C:\\ZiBManager\\ZiB Database.fdk"

#define REGVALUE_MANAGER_JOB_TIMEBASE					"ManagerJobsTimeBase"
#define REGVALUE_MANAGER_JOB_TIMEBASE_DEFAULT			5000
#define REGVALUE_MANAGER_JOB_DISCOVERYLATENCY			"ManagerJobDiscoveryLatency"
#define REGVALUE_MANAGER_JOB_DISCOVERYLATENCY_DEFAULT	30000
#define REGVALUE_MANAGER_JOB_ATTRIBUTESLATENCY			"ManagerJobAttributesHandleLatency"
#define REGVALUE_MANAGER_JOB_ATTRIBUTESLATENCY_DEFAULT	10000
#define REGVALUE_MANAGER_JOB_CHECKALARMLATENCY			"ManagerJobCheckAlarmLatency"
#define REGVALUE_MANAGER_JOB_CHECKALARMLATENCY_DEFAULT	5000
#define REGVALUE_MANAGER_DEBUGMASK						"ManagerDebugMask"
#define REGVALUE_MANAGER_DEBUGMASK_DEFAULT				0xF

#define REGVALUE_ALARM_SMSNUMBER						"AlarmSmsNumber"
#define REGVALUE_ALARM_SMSNUMBER_DEFAULT				"554191615262"
#define REGVALUE_ALARM_EMAIL							"AlarmEmail"
#define REGVALUE_ALARM_EMAIL_DEFAULT					"daniel.deschamps@gmail.com"
#define REGVALUE_ALARM_SMTPSERVER						"AlarmSmtpServer"
#define REGVALUE_ALARM_SMTPSERVER_DEFAULT				"localhost"

#define REGVALUE_ALARM_MAXSENDCOUNT						"AlarmMaxSendCount"
#define REGVALUE_ALARM_MAXSENDCOUNT_DEFAULT				3
#define REGVALUE_ALARM_REPEATLATENCY					"AlarmRepeatLatency"
#define REGVALUE_ALARM_REPEATLATENCY_DEFAULT			10*60000

CCriticalSection CZiBManagerRegistry::lock::m_cs;

CZiBManagerRegistry::CZiBManagerRegistry()
{
}

CZiBManagerRegistry::~CZiBManagerRegistry()
{
}

bool CZiBManagerRegistry::RefreshRegistrySettings(void)
{
	lock locker;

	TRACE(DBG_LVL_DBG4, "CZiBManagerRegistry::RefreshRegistrySettings\n");
	bool bExists = CRegistry::KeyExists(REGKEY,  HKEY_LOCAL_MACHINE);
	if (m_reg.Open(REGKEY, HKEY_LOCAL_MACHINE)) // If the key does not exist, this method creates it
	{
		if (!bExists)
		{
			// Create the default values
			m_reg[REGVALUE_GATEAWAYCOMPORT] = REGVALUE_GATEAWAYCOMPORT_DEFAULT;
			m_reg[REGVALUE_GATEAWAYCOMRATE] = (DWORD)REGVALUE_GATEAWAYCOMRATE_DEFAULT;
			
			m_reg[REGVALUE_MODEMCOMPORT] = REGVALUE_MODEMCOMPORT_DEFAULT;
			m_reg[REGVALUE_MODEMCOMRATE] = (DWORD)REGVALUE_MODEMCOMRATE_DEFAULT;

			m_reg[REGVALUE_DB_SERVERNAME] = REGVALUE_DB_SERVERNAME_DEFAULT;
			m_reg[REGVALUE_DB_USERNAME] = REGVALUE_DB_USERNAME_DEFAULT;
			m_reg[REGVALUE_DB_PASSWORD] = REGVALUE_DB_PASSWORD_DEFAULT;
			m_reg[REGVALUE_DB_DATABASEPATH] = REGVALUE_DB_DATABASEPATH_DEFAULT;
			m_reg[REGVALUE_DB_BACKUPPATH] = REGVALUE_DB_BACKUPPATH_DEFAULT;

			m_reg[REGVALUE_MANAGER_JOB_TIMEBASE] = (DWORD)REGVALUE_MANAGER_JOB_TIMEBASE_DEFAULT;
			m_reg[REGVALUE_MANAGER_JOB_DISCOVERYLATENCY] = (DWORD)REGVALUE_MANAGER_JOB_DISCOVERYLATENCY_DEFAULT;
			m_reg[REGVALUE_MANAGER_JOB_ATTRIBUTESLATENCY] = (DWORD)REGVALUE_MANAGER_JOB_ATTRIBUTESLATENCY_DEFAULT;
			m_reg[REGVALUE_MANAGER_JOB_CHECKALARMLATENCY] = (DWORD)REGVALUE_MANAGER_JOB_CHECKALARMLATENCY_DEFAULT;
			m_reg[REGVALUE_MANAGER_DEBUGMASK] = (DWORD)REGVALUE_MANAGER_DEBUGMASK_DEFAULT;

			m_reg[REGVALUE_ALARM_SMSNUMBER] = REGVALUE_ALARM_SMSNUMBER_DEFAULT;
			m_reg[REGVALUE_ALARM_EMAIL] = REGVALUE_ALARM_EMAIL_DEFAULT;
			m_reg[REGVALUE_ALARM_SMTPSERVER] = REGVALUE_ALARM_SMTPSERVER_DEFAULT;
			m_reg[REGVALUE_ALARM_MAXSENDCOUNT] = (DWORD)REGVALUE_ALARM_MAXSENDCOUNT_DEFAULT;
			m_reg[REGVALUE_ALARM_REPEATLATENCY] = (DWORD)REGVALUE_ALARM_REPEATLATENCY_DEFAULT;
		}

		m_settings.strGatewayComPort = (char*)m_reg[REGVALUE_GATEAWAYCOMPORT];
		m_settings.dwGatewayComRate = (DWORD)m_reg[REGVALUE_GATEAWAYCOMRATE];

		m_settings.strModemComPort = (char*)m_reg[REGVALUE_MODEMCOMPORT];
		m_settings.dwModemComRate = (DWORD)m_reg[REGVALUE_MODEMCOMRATE];

		m_settings.strDbServerName = (char*)m_reg[REGVALUE_DB_SERVERNAME];
		m_settings.strDbUserName = (char*)m_reg[REGVALUE_DB_USERNAME];
		m_settings.strDbPassword = (char*)m_reg[REGVALUE_DB_PASSWORD];
		m_settings.strDbPath = (char*)m_reg[REGVALUE_DB_DATABASEPATH];
		m_settings.strDbBackupPath = (char*)m_reg[REGVALUE_DB_BACKUPPATH];

		m_settings.dwManagerJobTimeBase = (DWORD)m_reg[REGVALUE_MANAGER_JOB_TIMEBASE];
		m_settings.dwManagerJobDiscoveryLatency = (DWORD)m_reg[REGVALUE_MANAGER_JOB_DISCOVERYLATENCY];
		m_settings.dwManagerJobAttributesHandleLatency = (DWORD)m_reg[REGVALUE_MANAGER_JOB_ATTRIBUTESLATENCY];
		m_settings.dwManagerJobCheckAlarmLatency = (DWORD)m_reg[REGVALUE_MANAGER_JOB_CHECKALARMLATENCY];
		m_settings.dwManagerDebugMask = (DWORD)m_reg[REGVALUE_MANAGER_DEBUGMASK];
	
		m_settings.strAlarmSmsNumber = (char*)m_reg[REGVALUE_ALARM_SMSNUMBER];
		m_settings.strAlarmEmail = (char*)m_reg[REGVALUE_ALARM_EMAIL];
		m_settings.strAlarmSmtpServer = (char*)m_reg[REGVALUE_ALARM_SMTPSERVER];
		m_settings.dwAlarmMaxSendCount = (DWORD)m_reg[REGVALUE_ALARM_MAXSENDCOUNT];
		m_settings.dwAlarmRepeatLatency = (DWORD)m_reg[REGVALUE_ALARM_REPEATLATENCY];

		// Consistency Verification
		if (m_settings.dwManagerJobAttributesHandleLatency > m_settings.dwManagerJobDiscoveryLatency)
		{
			TRACE(DBG_LVL_DBG4, "CZiBManagerRegistry::RefreshRegistrySettings - %s cannot be grater then %s\n",
				REGVALUE_MANAGER_JOB_ATTRIBUTESLATENCY,
				REGVALUE_MANAGER_JOB_DISCOVERYLATENCY);
			m_reg[REGVALUE_MANAGER_JOB_ATTRIBUTESLATENCY] = m_settings.dwManagerJobAttributesHandleLatency = m_settings.dwManagerJobDiscoveryLatency;
			m_reg.Refresh();
		}

		m_reg.Close();
		return true;
	}
	else
	{
		TRACE(DBG_LVL_WARNING, "CZiBManager::RefreshRegistrySettings - Error while trying to open the registry key '%s'", REGKEY);
		return false;
	}
}

bool CZiBManagerRegistry::DeleteRegistrySettings(void)
{
	lock locker;

	TRACE(DBG_LVL_DBG4, "CZiBManagerRegistry::DeleteRegistrySettings\n");
	bool bRet = true;
	if (CRegistry::KeyExists(REGKEY,  HKEY_LOCAL_MACHINE))
	{
		if (m_reg.Open(REGKEY, HKEY_LOCAL_MACHINE)) // If the key does not exist, this method creates it
		{
			m_reg.DeleteKey();
			m_reg.Close();
			bRet = true;
		}
		else
		{
			TRACE(DBG_LVL_WARNING, "CZiBManager::DeleteRegistrySettings - Error while trying to open the registry key '%s'", REGKEY);
			bRet = false;
		}
	}

	return bRet;
}

CZiBManagerRegistry::Settings CZiBManagerRegistry::Get(void)
{
	lock locker;
	TRACE(DBG_LVL_DBG4, "CZiBManagerRegistry::Get\n");
	return m_settings;
}
