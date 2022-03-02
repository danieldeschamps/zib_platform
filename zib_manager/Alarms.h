#ifndef _ALARMS_H_
#define _ALARMS_H_

#include "ZiBDb.h"
#include <misc/CriticalSection.h>

#define ALARM_VECTOR std::vector<CAlarmEntry>

class CAlarmEntry : public ZiBAttributeHistoryRow
{
public:
	CAlarmEntry(	int64_t qwMacAddr,
					int16_t wEpNumber,
					int16_t wClusterId,
					int16_t wAttributeId,
					std::string& strValue = string(""),
					IBPP::Timestamp timeMeasure = IBPP::Timestamp(),
					std::string& strTag = string(""),
					std::string& strUpLimit = string(""),
					std::string& strDownLimit = string(""),
					DWORD dwLimitAction = 0)

	:	ZiBAttributeHistoryRow(	qwMacAddr,
								wEpNumber,
								wClusterId,
								wAttributeId,
								strValue,
								timeMeasure ),
		m_strTag(strTag),
		m_strUpLimit(strUpLimit),
		m_strDownLimit(strDownLimit),
		m_dwLimitAction(dwLimitAction),
		m_tickLastTimeSent(0),
		m_uSendCount(0),
		m_bAlarmOn(true) {};

	~CAlarmEntry(){};

	void SetTag(string &strTag) { m_strTag = strTag; };
	string GetTag(void) { return m_strTag; };
	string& GetTagRef(void) { return m_strTag; };

	void SetUpLimit(string &strUpLimit) { m_strUpLimit = strUpLimit; };
	string GetUpLimit(void) { return m_strUpLimit; };
	string& GetUpLimitRef(void) { return m_strUpLimit; };

	void SetDownLimit(string &strDownLimit) { m_strDownLimit = strDownLimit; };
	string GetDownLimit(void) { return m_strDownLimit; };
	string& GetDownLimitRef(void) { return m_strDownLimit; };

	void SetLimitAction(DWORD strLimitAction) { m_dwLimitAction = strLimitAction; };
	DWORD GetLimitAction(void) { return m_dwLimitAction; };
	DWORD& GetLimitActionRef(void) { return m_dwLimitAction; };

	void SetLastTimeSentNow(void) { m_tickLastTimeSent = ::GetTickCount(); };
	DWORD GetLastTimeSent(void) { return m_tickLastTimeSent; };
	DWORD& GetLastTimeSentRef(void) { return m_tickLastTimeSent; };

	void IncreaseSendCount(void) { m_uSendCount++; };
	void ClearSendCount(void) { m_uSendCount = 0; };
	unsigned int GetSendCount(void) { return m_uSendCount; };

	void SetAlarmOn(bool bAlarmOn = true) { m_bAlarmOn = bAlarmOn; };
	bool GetAlarmOn(void) { return m_bAlarmOn; };

private:
	string m_strTag;
	string m_strUpLimit;
	string m_strDownLimit;
	DWORD m_dwLimitAction;
	DWORD m_tickLastTimeSent;
	unsigned int m_uSendCount;
	bool m_bAlarmOn;
};

class CAlarms
{
public:
	CAlarms(void* pManager);
	virtual ~CAlarms();

	void SetAlarm(CAlarmEntry& alarmEntry);
	void ClearAlarm(CAlarmEntry& alarmEntry);
	void ProcessAlarms(void);

	void SetMaxSendCount(unsigned int uMaxSendCount) { lock locker; m_uMaxSendCount = uMaxSendCount; };
	unsigned int GetMaxSendCount(void) { lock locker; return m_uMaxSendCount; };
	void SetRepeatLatency(DWORD dwRepeatLatencyMs) { lock locker; m_dwRepeatLatencyMs = dwRepeatLatencyMs; };
	DWORD GetRepeatLatency(void) { lock locker; return m_dwRepeatLatencyMs; };
	
private:
	ALARM_VECTOR m_vecAlarms;
	void* m_pManager;
	unsigned int m_uMaxSendCount;
	DWORD m_dwRepeatLatencyMs;

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
};

#endif // _ALARMS_H_