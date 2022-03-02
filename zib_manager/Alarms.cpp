#include "stdafx.h"
#include "Alarms.h"

#include "ZiBManager.h"

#define DEFAULT_MAX_SEND_COUNT	3 // 3 alarms
#define DEFAULT_ALARM_EXPIRE	60000*10 // each 10 minutes

// If the alarm is set, it will be signaled 3 times each 10 minutes.
// If alarm is set and the value goes back to normal state, the alarm is cleared
// If the value goes out of range again, then the alarm is set agin, but the 10 minutes timer will be respected!

CCriticalSection CAlarms::lock::m_cs;

CAlarms::CAlarms(void* pManager) 
:	m_pManager((CZiBManager*)pManager),
	m_uMaxSendCount(DEFAULT_MAX_SEND_COUNT),
	m_dwRepeatLatencyMs(DEFAULT_ALARM_EXPIRE)
{
}

CAlarms::~CAlarms()
{
}

void CAlarms::SetAlarm(CAlarmEntry& alarmEntry)
{
	lock locker;
	bool bFound = false;

	TRACE(DBG_LVL_DBG2, "CAlarms::SetAlarm - Setting alarm for node_tag<%s> ep<0x%02X> cluster<0x%02X> attribute<0x%04X>\n", 
		alarmEntry.GetTag().c_str(), alarmEntry.GetEpNumber(), alarmEntry.GetClusterId(), alarmEntry.GetAttributeId());

	// Find this attribute
	for (int i = 0; i < m_vecAlarms.size(); i++)
	{
		CAlarmEntry* pAlarm = &m_vecAlarms[i];
		if (	pAlarm->GetMacAddr() == alarmEntry.GetMacAddr() &&
				pAlarm->GetEpNumber() == alarmEntry.GetEpNumber() &&
				pAlarm->GetClusterId() == alarmEntry.GetClusterId() &&
				pAlarm->GetAttributeId() == alarmEntry.GetAttributeId() )
		{
			TRACE(DBG_LVL_DBG2, "CAlarms::SetAlarm - Alarm already stored. TimeToResend = %d [ms]\n",
				m_dwRepeatLatencyMs - (::GetTickCount() - pAlarm->GetLastTimeSent()));
			
			// Update the attribute alarm settings
			pAlarm->SetValue(alarmEntry.GetValue());
			pAlarm->SetTimeStamp(alarmEntry.GetTimeStamp());
			pAlarm->SetTag(alarmEntry.GetTag());
			pAlarm->SetUpLimit(alarmEntry.GetUpLimit());
			pAlarm->SetDownLimit(alarmEntry.GetDownLimit());
			pAlarm->SetLimitAction(alarmEntry.GetLimitAction());
			pAlarm->SetAlarmOn(true);

			bFound = true;
			break;
		}
	}
	
	// If not found, insert it in the alarm list
	if (!bFound)
	{
		m_vecAlarms.push_back(alarmEntry);
	}
}

void CAlarms::ClearAlarm(CAlarmEntry& alarmEntry)
{
	lock locker;

	TRACE(DBG_LVL_DBG2, "CAlarms::ClearAlarm - Clearing alarm for node_tag<%s> ep<0x%02X> cluster<0x%02X> attribute<0x%04X>\n", 
		alarmEntry.GetTag().c_str(), alarmEntry.GetEpNumber(), alarmEntry.GetClusterId(), alarmEntry.GetAttributeId());

	for (ALARM_VECTOR::iterator it = m_vecAlarms.begin(); it != m_vecAlarms.end(); it++)
	{
		CAlarmEntry* pAlarm = &(*it);
		if (	pAlarm->GetMacAddr() == alarmEntry.GetMacAddr() &&
				pAlarm->GetEpNumber() == alarmEntry.GetEpNumber() &&
				pAlarm->GetClusterId() == alarmEntry.GetClusterId() &&
				pAlarm->GetAttributeId() == alarmEntry.GetAttributeId() )
		{
			pAlarm->SetAlarmOn(false); // keep it stored, but off
			pAlarm->ClearSendCount();
			break;
		}
	}

}

void CAlarms::ProcessAlarms(void)
{
	lock locker;
	
	TRACE(DBG_LVL_DBG2, "CAlarms::ProcessAlarms - Started processing alarms. AlarmCount=<%u>>\n", m_vecAlarms.size());

	CZiBManager* pManager = (CZiBManager*)m_pManager;
	for (ALARM_VECTOR::iterator it = m_vecAlarms.begin(); it != m_vecAlarms.end(); it++)
	{
		CAlarmEntry* pAlarm = &(*it);

		TRACE(DBG_LVL_DBG2, "CAlarms::ProcessAlarms - Processing alarm for node_tag<%s> ep<0x%02X> cluster<0x%02X> attribute<0x%04X>\n", 
			pAlarm->GetTag().c_str(), pAlarm->GetEpNumber(), pAlarm->GetClusterId(), pAlarm->GetAttributeId());
		TRACE(DBG_LVL_DBG2, "CAlarms::ProcessAlarms - SendCount<%u> AlarmOn=<%s> TimeToResend = %d [ms]\n", 
			pAlarm->GetSendCount(), pAlarm->GetAlarmOn()? "true": "false", pAlarm->GetLastTimeSent()? (m_dwRepeatLatencyMs - (::GetTickCount() - pAlarm->GetLastTimeSent())): 0);

		if (pAlarm->GetSendCount() >= m_uMaxSendCount || !pAlarm->GetAlarmOn())
		{
			continue;
		}

		DWORD tickNow = ::GetTickCount();
		if ( pManager && (	pAlarm->GetSendCount == 0 // has never been sent
							|| tickNow < pAlarm->GetLastTimeSent() // tick has rolled over
							|| (tickNow - pAlarm->GetLastTimeSent()) > m_dwRepeatLatencyMs)) // The last alarm sent to this attribute is expired
		{
			TRACE(DBG_LVL_DBG2, "CAlarms::ProcessAlarms - *** Alarm triggered, send now *** LimitAction=<0x%02X>\n", 
				pAlarm->GetLimitAction());
			
			char szSmsMessage[160*3]; // todo: verify this size
			int64_t mac = pAlarm->GetMacAddr();
			sprintf(szSmsMessage,
				"ZiB Manager Alarm: tag=<%s> mac=<0x%08X%08X> ep=<0x%02X> cluster=<0x%02X> attribute=<0x%04X> value=<%s> limits=<%s><%s>",
				pAlarm->GetTag().c_str(),
				*((int32_t*)(&mac)+1),
				*((int32_t*)(&mac)),
				pAlarm->GetEpNumber(),
				pAlarm->GetClusterId(),
				pAlarm->GetAttributeId(),
				pAlarm->GetValue().c_str(),
				pAlarm->GetDownLimit().c_str(),
				pAlarm->GetUpLimit().c_str());

			if (pAlarm->GetLimitAction() & 0x1) // SMS
			{
				if (pManager->m_bModemActivated)
				{
					if (!pManager->m_modem.SendSms(pManager->m_reg.Get().strAlarmSmsNumber, szSmsMessage))
					{
						TRACE(DBG_LVL_WARNING, "CZiBManager::AttributeHandleStart - Unable to send SMS alarm\n");
					}
				}
			}
			if (pAlarm->GetLimitAction() & 0x2) // Email
			{
				if (pManager->m_bSmtpConnectionActivated && pManager->m_mail.GetConnectStatus()) 
				{
					pManager->m_mail.SetMessageBody(szSmsMessage);
					if (!pManager->m_mail.Send())
					{
						TRACE(DBG_LVL_WARNING, "CZiBManager::AttributeHandleStart - Unable to send Email alarm\n");
					}
				}
			}

			pAlarm->SetLastTimeSentNow();
			pAlarm->IncreaseSendCount();
		}
	}
}