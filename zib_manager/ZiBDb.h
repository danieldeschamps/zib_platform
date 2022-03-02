#ifndef _ZIBDB_H_
#define _ZIBDB_H_

#include <ibpp/core/ibpp.h>
#include <misc/CriticalSection.h>

#include <iostream> // todo: remove this (cout)

/*
CREATE TABLE t_nodes (
mac_addr BIGINT NOT NULL,
nwk_addr SMALLINT,
profile_id SMALLINT,
device_id SMALLINT,
online SMALLINT,
tag VARCHAR(50),
PRIMARY KEY (mac_addr) );
*/
class ZiBNodeRow
{
public:
	ZiBNodeRow(int64_t qwMacAddr = 0,
		int16_t wNwkAddr = 0,
		int16_t wProfileId = 0,
		int16_t wDeviceId = 0,
		bool bOnline = false,
		const string &strTag = "")	:	m_qwMacAddr(qwMacAddr),
										m_wNwkAddr(wNwkAddr),
										m_wProfileId(wProfileId), 
										m_wDeviceId(wDeviceId),
										m_bOnline(bOnline),
										m_strTag(strTag) {};
	~ZiBNodeRow(){};

	void SetMacAddr(int64_t qwMacAddr) { m_qwMacAddr = qwMacAddr; }
	void SetNwkAddr(int16_t wNwkAddr) { m_wNwkAddr = wNwkAddr; }
	void SetProfileId(int16_t wProfileId) { m_wProfileId = wProfileId; }
	void SetDeviceId(int16_t wDeviceId) { m_wDeviceId = wDeviceId; }
	void SetOnline(bool bOnline) { m_bOnline = bOnline; }
	void SetTag(const string &strTag) { m_strTag = strTag; };

	int64_t GetMacAddr() { return m_qwMacAddr; }
	int16_t GetNwkAddr() { return m_wNwkAddr; }
	int16_t GetProfileId() { return m_wProfileId; }
	int16_t GetDeviceId() { return m_wDeviceId; }
	bool GetOnline() { return m_bOnline; }
	string GetTag() { return m_strTag; }

	int64_t& GetMacAddrRef() { return m_qwMacAddr; }
	int16_t& GetNwkAddrRef() { return m_wNwkAddr; }
	int16_t& GetProfileIdRef() { return m_wProfileId; }
	int16_t& GetDeviceIdRef() { return m_wDeviceId; }
	bool& GetOnlineRef() { return m_bOnline; }
	string& GetTagRef() { return m_strTag; }


private:
	int64_t m_qwMacAddr;
	int16_t m_wNwkAddr;
	int16_t m_wProfileId; 
	int16_t m_wDeviceId;
	bool m_bOnline;
	string m_strTag;
};

/*
CREATE TABLE t_attributes (
mac_addr BIGINT NOT NULL,
ep_number SMALLINT NOT NULL,
cluster_id SMALLINT NOT NULL,
attribute_id SMALLINT NOT NULL,
attribute_value VARCHAR(50),
up_limit VARCHAR(50),
down_limit VARCHAR(50),
limit_action SMALLINT,
FOREIGN KEY (mac_addr) REFERENCES t_nodes (mac_addr) );
*/
class ZiBAttributeRow
{
public:
	ZiBAttributeRow(	int64_t qwMacAddr = 0,
						int16_t wEpNumber = 0,
						int16_t wClusterId = 0,
						int16_t wAttributeId = 0,
						std::string strValue = "")
	:	m_qwMacAddr(qwMacAddr),
		m_wEpNumber(wEpNumber),
		m_wClusterId(wClusterId), 
		m_wAttributeId(wAttributeId),
		m_strValue(strValue) {};

	~ZiBAttributeRow(){};

	void SetMacAddr(int64_t qwMacAddr) { m_qwMacAddr = qwMacAddr; };
	void SetEpNumber(int16_t wEpNumber) { m_wEpNumber = wEpNumber; };
	void SetClusterId(int16_t wClusterId) { m_wClusterId = wClusterId; };
	void SetAttributeId(int16_t wAttributeId) { m_wAttributeId = wAttributeId; };
	void SetValue(string &strValue) { m_strValue = strValue; };

	int64_t GetMacAddr() { return m_qwMacAddr; };
	int16_t GetEpNumber() { return m_wEpNumber; };
	int16_t GetClusterId() { return m_wClusterId; };
	int16_t GetAttributeId() { return m_wAttributeId; };
	string GetValue() { return m_strValue; };

	int64_t& GetMacAddrRef() { return m_qwMacAddr; };
	int16_t& GetEpNumberRef() { return m_wEpNumber; };
	int16_t& GetClusterIdRef() { return m_wClusterId; };
	int16_t& GetAttributeIdRef() { return m_wAttributeId; };
	string& GetValueRef() { return m_strValue; };

private:
	int64_t m_qwMacAddr;
	int16_t m_wEpNumber;
	int16_t m_wClusterId;
	int16_t m_wAttributeId;
	string m_strValue;
};

/*
CREATE TABLE t_attributes_history (
mac_addr BIGINT NOT NULL,
ep_number SMALLINT NOT NULL,
cluster_id SMALLINT NOT NULL,
attribute_id SMALLINT NOT NULL,
measure_time TIMESTAMP NOT NULL,
attribute_value VARCHAR(50),
FOREIGN KEY (mac_addr) REFERENCES t_nodes (mac_addr) );
*/
class ZiBAttributeHistoryRow : public ZiBAttributeRow
{
public:
	ZiBAttributeHistoryRow(	int64_t qwMacAddr = 0,
							int16_t wEpNumber = 0,
							int16_t wClusterId = 0,
							int16_t wAttributeId = 0,
							std::string strValue = "",
							IBPP::Timestamp timeMeasure = IBPP::Timestamp() )	
	:	ZiBAttributeRow(qwMacAddr, wEpNumber, wClusterId, wAttributeId, strValue),
		m_timeMeasure(timeMeasure) {};

	~ZiBAttributeHistoryRow() {};

	void SetTimeStamp(IBPP::Timestamp timeMeasure) { m_timeMeasure = timeMeasure; };
	IBPP::Timestamp GetTimeStamp() { return m_timeMeasure; };
	IBPP::Timestamp& GetTimeStampRef() { return m_timeMeasure; };

private:
	IBPP::Timestamp m_timeMeasure;
};

// Class which access the firebird database. Thread safe.
class CZiBDb
{
public:
	CZiBDb();
	~CZiBDb();

// database
	void Setup(const std::string &strServerName, 
		const std::string &strUserName, 
		const std::string &strPassword, 
		const std::string &strDatabaseName, 
		const std::string &strBackupName,
		const std::string &strWriteMode);
	bool CreateDatabase(void);
	bool DropDatabase(void);
	bool Connect(void);
	bool CreateTables(void);
	
// nodes
	bool InsertNode(ZiBNodeRow &node);
	bool InsertNode(int64_t qwMacAddr,
		int16_t wNwkAddr, 
		int16_t wProfileId, 
		int16_t wDeviceId, 
		bool bOnline);
	bool UpdateNode(ZiBNodeRow &node);
	bool UpdateNode(int64_t qwMacAddr,
		int16_t wNwkAddr, 
		int16_t wProfileId, 
		int16_t wDeviceId, 
		bool bOnline);
	bool UpdateNode(int64_t qwMacAddr,
		int16_t wNwkAddr, 
		int16_t wProfileId, 
		int16_t wDeviceId);
	bool FindNode(int64_t qwMacAddr, ZiBNodeRow *pNode = 0);
	bool DeleteNode(int64_t qwMacAddr);
	bool AllNodesOffline(void);

// attributes
	bool InsertAttribute(ZiBAttributeRow &attrib);
	bool InsertAttribute(int64_t qwMacAddr,
		int16_t wEpNumber, 
		int16_t wClusterId, 
		int16_t wAttributeId, 
		std::string strValue);
	bool UpdateAttribute(ZiBAttributeRow &attrib);
	bool UpdateAttribute(int64_t qwMacAddr,
		int16_t wEpNumber, 
		int16_t wClusterId, 
		int16_t wAttributeId,
		const string &strValue);
	bool FindAttribute(ZiBAttributeRow &attrib);
	bool FindAttribute(int64_t qwMacAddr,
		int16_t wEpNumber, 
		int16_t wClusterId, 
		int16_t wAttributeId,
		string& strAttributeValueReturn);
	// bool DeleteAttribute(ZiBAttributeRow &attrib);
	bool GetAttributeAndLimits(int64_t qwMacAddr,
		int16_t wEpNumber, 
		int16_t wClusterId, 
		int16_t wAttributeId,
		string &strAttributeValueReturn,
		string &strUpLimitReturn,
		string &strDownLimitReturn,
		DWORD &dwLimitAction);

// attributes_history
	bool InsertAttributeHistory(ZiBAttributeHistoryRow &attrib);
	bool InsertAttributeHistory(int64_t qwMacAddr,
		int16_t wEpNumber,
		int16_t wClusterId,
		int16_t wAttributeId,
		const IBPP::Timestamp &timeMeasure,
		const std::string &strValue);
	bool UpdateAttributeHistory(ZiBAttributeHistoryRow &attrib);
	bool UpdateAttributeHistory(int64_t qwMacAddr,
		int16_t wEpNumber, 
		int16_t wClusterId, 
		int16_t wAttributeId,
		const IBPP::Timestamp &timeMeasure,
		const string &strValue);
	bool FindAttributeHistory(ZiBAttributeHistoryRow &attrib);
	bool FindAttributeHistory(int64_t qwMacAddr,
		int16_t wEpNumber, 
		int16_t wClusterId, 
		int16_t wAttributeId,
		const IBPP::Timestamp &timeMeasure,
		string &strAttributeValueReturn);
	// bool DeleteAttributeHistory(ZiBAttributeHistoryRow &attrib);

private:
	std::string m_strServerName;
	std::string m_strUserName;
	std::string m_strPassword;
	std::string m_strDatabaseName;
	std::string m_strBackupName;
	int m_iWriteMode;

	IBPP::Database m_db;
	
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

#endif // _ZIBDB_H_