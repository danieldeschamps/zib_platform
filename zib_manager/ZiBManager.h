#ifndef _ZIB_MANAGER_H_
#define _ZIB_MANAGER_H_

#include "ZiBGatewayComm.h"
#include "ZiBDb.h"
#include "ZiBManagerRegistry.h"
#include "ModemEricsson.h"
#include "Alarms.h"

#include <email/FastSmtp.h>
#include <misc/CriticalSection.h>

#include <map>

#define EP_NUMBER		BYTE
#define CLUSTER_ID		BYTE
#define ATTRIBUTE_ID	WORD
#define NWK_ADDR		WORD

#define ATTRIBUTE_MAP		map < ATTRIBUTE_ID,	string >
#define CLUSTER_MAP			map < CLUSTER_ID,	ATTRIBUTE_MAP >
#define EP_MAP				map < EP_NUMBER,	CLUSTER_MAP >
#define NODE_LOGICAL_MAP	map < NWK_ADDR,		EP_MAP >
#define NODE_PHYSICAL_MAP	map < NWK_ADDR,		ZiBNode >

#define JOB_MAP				map < EManagerJob, DWORD>

#define ATTRIBUTE_VECTOR		vector < ATTRIBUTE_ID >
#define CLUSTER_ATTRIBUTES_MAP	map <CLUSTER_ID, ATTRIBUTE_VECTOR >


// todo: change this to protected so SystemService.cpp cannot access inherited members
class CZiBManager: public IZiBGatewayComm
{
public:

	friend class CAlarms;

	typedef enum
	{
		EMangerReturnActionExit,
		EMangerReturnActionReset,
		EMangerReturnActionError
	} EMangerReturnAction;

	typedef enum
	{
		EMangerInstallComponentsRegistry = 0x1,
		EMangerInstallComponentsDatabase = 0x2,
		EMangerInstallComponentsAll = -1
	} EMangerInstallComponents;


	CZiBManager();
	~CZiBManager();

	bool Install(EMangerInstallComponents eComponents = EMangerInstallComponentsAll);
	bool Uninstall(EMangerInstallComponents eComponents = EMangerInstallComponentsAll);
	EMangerReturnAction Run(void);
	void Stop(void);

	// CONFIRMATIONS
	virtual void ResetConf(BYTE byteStatus);
	virtual void PanIdConf(BYTE byteStatus, 
		WORD wPanId);
	virtual void NodeListConf(BYTE byteStatus, 
		vector<ZiBNode>& vecNodes);
	virtual void EpListConf(BYTE byteStatus,
		WORD wNwkAddress,
		vector<BYTE>& vecEps);
	virtual void ClusterListConf(BYTE byteStatus,
		WORD wNwkAddress,
		BYTE byteEp,
		vector<BYTE>& vecClusters);
	virtual void AttributeValueConf(BYTE byteStatus, 
		WORD wNwkAddress,
		BYTE byteEp,
		BYTE byteClusterId,
		WORD wAttributeId,
		BYTE byteSize,
		void* pData);

	// INDICATIONS
	virtual void ResetInd(void);
	virtual void GatewayDebugInd (const string& strTrace);

private:
	// Main loop events
	HANDLE m_hStopEvent;
	HANDLE m_hResetEvent;
	HANDLE m_hJobsTimerEvent;

	// Jobs
	typedef enum
	{
		EManagerJobDiscovery,
		EManagerJobAttributesHandle,
		EManagerJobProcessAlarms
	} EManagerJob;
	JOB_MAP m_mapJobs;
	bool StartJobsTimer(void);
	bool StopJobsTimer(void);
	void ProcessJobsTimer(void);

	// Shared (among threads) internal objects
	// ### HAVE TO BE CRITICAL SECTION PROTECTED ###

	// Registry
	CZiBManagerRegistry m_reg;

	// Database
	CZiBDb m_db;
	
	// Modem
	CModemEricsson m_modem;

	// Email
	CFastSmtp m_mail; // todo: critical section protect

	// Alarms
	CAlarms m_alarms;

	// Network Discovery
	// todo: make this as an object
	CCriticalSection m_csNetDiscovery;
	void DiscoveryStart(void);
	void DiscoveryClear(void);
	void DiscoveryDumpPhysical(void);
	void DiscoveryDumpLogical(void);
	void AttributeHandleStart(void);
	NODE_LOGICAL_MAP m_mapNodesLogical;
	NODE_PHYSICAL_MAP m_mapNodesPhysical;
	CLUSTER_ATTRIBUTES_MAP m_mapClusterAttributes;
	WORD m_wPanId;

	// Flags
	bool m_bModemActivated;
	bool m_bSmtpConnectionActivated;
};

#endif //_ZIB_MANAGER_H_