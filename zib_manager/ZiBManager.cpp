#include "StdAfx.h"
#include "ZiBManager.h"
#include <zib_network/ZigBeeStack/zZiBPlatform.h> // todo: move this file to <common/zZiBPlatform.h>

/*
0x01	DBG_LVL_INFO
0x02	DBG_LVL_ERROR
0x04	DBG_LVL_WARNING
0x80	DBG_LVL_DEBUG -> general information
0x10	DBG_LVL_DBG1 -> DISCOVERY AND ATTRIBUTE HANDLE
0x20	DBG_LVL_DBG2 -> Alarms
0x40	DBG_LVL_DBG3 -> Gateway debug messages
0x80	DBG_LVL_DBG4 -> function calls
*/

CZiBManager::CZiBManager()
:	m_bModemActivated (false),
	m_bSmtpConnectionActivated (false),
	m_alarms(this)
{
	TRACE (DBG_LVL_DBG4, "CZiBManager::CZiBManager - Creating the ZiB Manager object\n");
	
	m_hStopEvent = ::CreateEvent (NULL, FALSE, FALSE, NULL);
	m_hResetEvent = ::CreateEvent (NULL, FALSE, FALSE, NULL);
	m_hJobsTimerEvent = ::CreateWaitableTimer(NULL, FALSE, NULL);

	// Cluster x Attributes table.
	ATTRIBUTE_VECTOR vecAnalog;
	ATTRIBUTE_VECTOR vecSensors;
	ATTRIBUTE_VECTOR vecActuators;

	vecAnalog.push_back(ATTRIBUTE_ZIB_DTP_ANALOG_VOLTS);
	vecSensors.push_back(ATTRIBUTE_ZIB_DTP_SENSORS_TEMPERATURE);
	vecActuators.push_back(ATTRIBUTE_ZIB_DTP_ACTUATORS_LED1);
	vecActuators.push_back(ATTRIBUTE_ZIB_DTP_ACTUATORS_LED2);

	m_mapClusterAttributes[ZIB_DTP_ANALOG_CLUSTER] = vecAnalog;
	m_mapClusterAttributes[ZIB_DTP_SENSORS_CLUSTER] = vecSensors;
	m_mapClusterAttributes[ZIB_DTP_ACTUATORS_CLUSTER] = vecActuators;
}

CZiBManager::~CZiBManager()
{
	TRACE (DBG_LVL_DBG4, "CZiBManager::~CZiBManager - Destroying the ZiB Manager object\n");
	
	::CloseHandle (m_hStopEvent);
	::CloseHandle (m_hResetEvent);
	::CloseHandle (m_hJobsTimerEvent);
}

bool CZiBManager::Install(EMangerInstallComponents eComponents)
{
	SET_TRACE_MASK(DBG_MASK_ALL);
	TRACE (DBG_LVL_DBG4, "CZiBManager::Install - Installing the ZiB Manager components\n");
	bool bRet = false;
	
// Create/Read the registry
	if (m_reg.RefreshRegistrySettings())
	{
		if (eComponents & EMangerInstallComponentsDatabase)
		{
		// Open the databasae
			m_db.Setup(m_reg.Get().strDbServerName, 
				m_reg.Get().strDbUserName, 
				m_reg.Get().strDbPassword, 
				m_reg.Get().strDbPath, 
				m_reg.Get().strDbBackupPath, 
				"speed");

			if (m_db.CreateDatabase()) // false means it already exists or could not create
			{
			// Create the tables
				if (m_db.CreateTables())
				{
					bRet = true;
				}
			}
		}
		else
		{
			bRet = true;
		}
	}

	if (bRet)
	{
		TRACE(DBG_LVL_ERROR, "CZiBManager::Install - Successfully installed the ZiB Manager components\n");
	}
	else
	{
		TRACE(DBG_LVL_ERROR, "CZiBManager::Install - Unable to install the ZiB Manager components\n");
	}
	
	return bRet;
}

bool CZiBManager::Uninstall(EMangerInstallComponents eComponents)
{
	SET_TRACE_MASK(DBG_MASK_ALL);
	TRACE (DBG_LVL_DBG4, "CZiBManager::Uninstall - Uninstalling the ZiB Manager components\n");
	bool bRet = false;
		
// Connect to the database
	if (m_reg.RefreshRegistrySettings())
	{
		if (eComponents & EMangerInstallComponentsDatabase)
		{
			m_db.Setup(m_reg.Get().strDbServerName, 
				m_reg.Get().strDbUserName, 
				m_reg.Get().strDbPassword, 
				m_reg.Get().strDbPath, 
				m_reg.Get().strDbPath, 
				"speed");

			if (m_db.Connect())
			{
			// Remove the database
				if (m_db.DropDatabase())
				{
					bRet = true;
				}
			}
		}
		else
		{
			bRet = true;
		}

		if (bRet)
		{
			if (eComponents & EMangerInstallComponentsRegistry)
			{
				// Remove the registry key
				if (m_reg.DeleteRegistrySettings())
				{
					bRet = true;
				}
				else
				{
					bRet = false;
				}
			}
			else
			{
				bRet = true;
			}
		}
	}
	
	if (bRet)
	{
		TRACE(DBG_LVL_ERROR, "CZiBManager::Uninstall - Successfully uninstalled the ZiB Manager components\n");
	}
	else
	{
		TRACE(DBG_LVL_ERROR, "CZiBManager::Uninstall - Unable to uninstall the ZiB Manager components\n");
	}
	
	return bRet;
}

// THREAD Main Service Thread -> Based on top level decisions, sends gateway messages downwards
CZiBManager::EMangerReturnAction CZiBManager::Run(void)
{
	TRACE (DBG_LVL_DBG4, "CZiBManager::Run - Start operations\n");
	HANDLE hEventArray[3];
	EMangerReturnAction eReturnAction = EMangerReturnActionExit;
	DWORD dwObjectSignaled;
	bool bExit = false;

	SET_TRACE_MASK(DBG_MASK_ALL);

// Read the registry
	if (!m_reg.RefreshRegistrySettings())
	{
		TRACE(DBG_LVL_ERROR, "CZiBManager::Run - Unable to run the ZiB Manager instance because the registry cannot be read\n");
		return EMangerReturnActionError;
	}
	
// Set debug levels
	SET_TRACE_MASK(m_reg.Get().dwManagerDebugMask);

// Open the databasae
	m_db.Setup(m_reg.Get().strDbServerName, 
		m_reg.Get().strDbUserName, 
		m_reg.Get().strDbPassword, 
		m_reg.Get().strDbPath, 
		m_reg.Get().strDbBackupPath, 
		"speed");

	if (!m_db.Connect())
	{
		TRACE(DBG_LVL_ERROR, "CZiBManager::Run - Unable to run the ZiB Manager instance because the connection to the databasae failed\n");
		return EMangerReturnActionError;
	}

// Start the serial port to communicate with the Gateway
	if (IZiBGatewayComm::StartSerialPort(m_reg.Get().strGatewayComPort.c_str(), 
			(CSerial::EBaudrate)m_reg.Get().dwGatewayComRate,
			CSerial::EData8,
			CSerial::EParNone,
			CSerial::EStop1,
			CSerial::EHandshakeOff) != ERROR_SUCCESS)
	{
		TRACE(DBG_LVL_ERROR, "CZiBManager::Run - Unable to run the ZiB Manager instance because the serial port could not be started\n");
		return EMangerReturnActionError;
	}

// Start the modem
	if (m_reg.Get().strAlarmSmsNumber.length())
	{
		if (m_modem.SetupSerialPort(m_reg.Get().strModemComPort, 
				(CSerial::EBaudrate)m_reg.Get().dwModemComRate,
				CSerial::EData8,
				CSerial::EParNone,
				CSerial::EStop1,
				CSerial::EHandshakeOff )
			&& m_modem.TestCommunication() )
		{
			TRACE(DBG_LVL_DEBUG, "CZiBManager::Run - Modem successfully initialized\n");
			m_bModemActivated = true;
		}
		else
		{
			TRACE(DBG_LVL_WARNING, "CZiBManager::Run - Error initializing modem. It will not be used\n");
			m_bModemActivated = false;
		}
	}
	else
	{
		TRACE(DBG_LVL_DEBUG, "CZiBManager::Run - The modem will not be initialized. Registry.AlarmSmsNumber is empty or not defined\n");
		m_bModemActivated = false;
	}

// Connect to the SMTP server and prepare the email pumper
	if (!m_reg.Get().strAlarmEmail.empty() && !m_reg.Get().strAlarmSmtpServer.empty())
	{
		if (m_mail.ConnectServer("localhost")) 
		{
			m_mail.SetSenderName("ZiBManager");
			m_mail.SetSenderEmail("daniel.deschamps@gmail.com");
			m_mail.SetSubject("ZiB Manager Alarm");
			m_mail.AddRecipient(m_reg.Get().strAlarmEmail.c_str());
			//m_mail.AddCCRecipient("test@test.com");
			//m_mail.AddBCCRecipient("test@test.com");
			TRACE(DBG_LVL_DEBUG, "CZiBManager::Run - SMTP connection successfully initialized\n");
			m_bSmtpConnectionActivated = true;
		}
		else
		{
			TRACE(DBG_LVL_DEBUG, "CZiBManager::Run - Error setting up SMTP connection. e-mails will not be sent\n");
			m_bSmtpConnectionActivated = false;
		}
	}
	else
	{
		TRACE(DBG_LVL_DEBUG, "CZiBManager::Run - The SMTP connection will not be setup. Registry.AlarmSmtpServer or Registry.strAlarmEmail are empty or not defined\n");
		m_bSmtpConnectionActivated = false;
	}

// Start the message processor - starts signaling the virtuals (CONF and IND)
	if (IZiBGatewayComm::StartThreadProcessInMsg() != ERROR_SUCCESS)
	{
		TRACE(DBG_LVL_ERROR, "CZiBManager::Run - Unable to run the ZiB Manager instance because the message processor could not be started\n");
		return EMangerReturnActionError;
	}

// Alarm manager
	m_alarms.SetMaxSendCount(m_reg.Get().dwAlarmMaxSendCount);
	m_alarms.SetRepeatLatency(m_reg.Get().dwAlarmRepeatLatency);

// Jobs
	if (!StartJobsTimer())
	{
		TRACE(DBG_LVL_ERROR, "CZiBManager::Run - Error while creating the timer\n");
		return EMangerReturnActionError;
	}

// Event handling
	hEventArray[0] = m_hStopEvent;
	hEventArray[1] = m_hResetEvent;
	hEventArray[2] = m_hJobsTimerEvent;
	
	while (!bExit)
	{
		dwObjectSignaled = WaitForMultipleObjects (3, hEventArray, FALSE, INFINITE);
		switch (dwObjectSignaled)
		{
			case WAIT_OBJECT_0: // m_hStopEvent
				TRACE(DBG_LVL_DEBUG, "CZiBManager::Run - StopEvent was signaled\n");
				eReturnAction = EMangerReturnActionExit;
				bExit = true;
				break;

			case WAIT_OBJECT_0 + 1: // m_hResetEvent
				TRACE(DBG_LVL_DEBUG, "CZiBManager::Run - ResetEvent was signaled\n");
				eReturnAction = EMangerReturnActionReset;
				bExit = true;
				break;

			case WAIT_OBJECT_0 + 2: // m_hTimerEvent
				TRACE(DBG_LVL_DEBUG, "CZiBManager::Run - TimerEvent was signaled\n");
				ProcessJobsTimer();
				break;

			default:
				TRACE(DBG_LVL_DEBUG, "CZiBManager::Run - Unknown object signaled\n");
				break;
		}
	}

	return eReturnAction;
}

void CZiBManager::Stop(void)
{
	TRACE(DBG_LVL_INFO, "CZiBManager::Stop - Sending the StopEvent\n");
	SetEvent(m_hStopEvent); // signal to stop
}

bool CZiBManager::StartJobsTimer(void)
{
	bool bRet = true;
	LARGE_INTEGER lElapse;
	int elapse = m_reg.Get().dwManagerJobTimeBase;
	DWORD dwCurrentTick = ::GetTickCount();
	
	m_mapJobs[EManagerJobDiscovery] = dwCurrentTick;
	m_mapJobs[EManagerJobAttributesHandle] = dwCurrentTick + m_reg.Get().dwManagerJobAttributesHandleLatency;
	m_mapJobs[EManagerJobProcessAlarms] = dwCurrentTick + m_reg.Get().dwManagerJobCheckAlarmLatency;
	
	lElapse.QuadPart = - ((int)elapse * 10000);
	if (! (bRet =	!!::SetWaitableTimer(m_hJobsTimerEvent, 
						&lElapse, // first trigger of the timer in 100ns units
						elapse, // repeat the timer at each elapse ms
						NULL,
						NULL, 
						FALSE)))
	{
		m_lLastError = ::GetLastError();
		TRACE(DBG_LVL_WARNING, "CZiBManager::StartTimer - SetWaitableTimer failed (%d)\n", m_lLastError);
	}
	else
	{
		ProcessJobsTimer(); // Perform the EManagerJobDiscovery
	}

	return bRet;
}

bool CZiBManager::StopJobsTimer(void)
{
	bool bRet = true;
	if (!(bRet = !!::CancelWaitableTimer(m_hJobsTimerEvent)))
	{
		m_lLastError = ::GetLastError();
		TRACE(DBG_LVL_WARNING, "CZiBManager::StartTimer - CancelWaitableTimer failed (%d)\n", m_lLastError);
	}
	return bRet;
}

void CZiBManager::ProcessJobsTimer(void)
{
	DWORD dwCurrentTick = ::GetTickCount();

	if (!m_reg.RefreshRegistrySettings())
	{
		TRACE(DBG_LVL_DEBUG, "CZiBManager::ProcessJobsTimer - Unable to update the registry settings\n");
	}

	SET_TRACE_MASK(m_reg.Get().dwManagerDebugMask);

	if (dwCurrentTick >= m_mapJobs[EManagerJobDiscovery])
	{
		m_mapJobs[EManagerJobDiscovery] = dwCurrentTick + m_reg.Get().dwManagerJobDiscoveryLatency;
		DiscoveryClear();
		DiscoveryStart();
		DiscoveryDumpPhysical();
		DiscoveryDumpLogical();
	}

	if (dwCurrentTick >= m_mapJobs[EManagerJobAttributesHandle])
	{
		m_mapJobs[EManagerJobAttributesHandle] = dwCurrentTick + m_reg.Get().dwManagerJobAttributesHandleLatency;
		AttributeHandleStart();
	}

	if (dwCurrentTick >= m_mapJobs[EManagerJobProcessAlarms])
	{
		m_mapJobs[EManagerJobProcessAlarms] = dwCurrentTick + m_reg.Get().dwManagerJobCheckAlarmLatency;
		m_alarms.ProcessAlarms();
	}
}

void CZiBManager::DiscoveryStart(void)
{
	TRACE(DBG_LVL_DBG4, "CZiBManager::DiscoveryStart\n");

// physical
	PanIdReq(false);
	NodeListReq(false);
	
	m_csNetDiscovery.lock();
	NODE_PHYSICAL_MAP::iterator node_phy_it = m_mapNodesPhysical.begin();
	for (; node_phy_it != m_mapNodesPhysical.end(); node_phy_it++)
	{
	// store in the database
		ZiBNode* pNode = &node_phy_it->second;
		ZiBNodeRow node_row(*(int64_t*)&pNode->mac_addr,
			(int16_t)pNode->nwk_addr,
			(int16_t)pNode->profile_id,
			(int16_t)pNode->device_id, 
			false);
		if (!m_db.FindNode(*(int64_t*)&pNode->mac_addr))
		{
			m_db.InsertNode(node_row);
		}
		else
		{
			m_db.UpdateNode(*(int64_t*)&pNode->mac_addr,
				(int16_t)pNode->nwk_addr,
				(int16_t)pNode->profile_id,
				(int16_t)pNode->device_id); // don't update the online status yet
		}
	}

// logic
	NODE_LOGICAL_MAP::iterator node_logic_it = m_mapNodesLogical.begin();
	for (; node_logic_it != m_mapNodesLogical.end(); node_logic_it++)
	{
		WORD wNwkAddr = node_logic_it->first;

		m_csNetDiscovery.unlock();
		LONG lRet = EpListReq(wNwkAddr, false);
		m_csNetDiscovery.lock();

		// update the online field
		ZiBNode* pNode = &m_mapNodesPhysical[wNwkAddr];
		ZiBNodeRow node_row(*(int64_t*)&pNode->mac_addr,
			(int16_t)pNode->nwk_addr,
			(int16_t)pNode->profile_id,
			(int16_t)pNode->device_id,
			lRet==ERROR_SUCCESS? true: false);
		if (m_db.FindNode(*(int64_t*)&pNode->mac_addr))
		{
			m_db.UpdateNode(node_row);
		}

		EP_MAP::iterator ep_it = m_mapNodesLogical [wNwkAddr].begin();
		for (; ep_it != m_mapNodesLogical [wNwkAddr].end(); ep_it++)
		{
			BYTE byteEp =  ep_it->first;

			if (byteEp != 0x00) // todo: use a define
			{
				m_csNetDiscovery.unlock();
				ClusterListReq(wNwkAddr, byteEp, false);
				m_csNetDiscovery.lock();

				CLUSTER_MAP::iterator cluster_it = m_mapNodesLogical [wNwkAddr] [byteEp].begin();
				for (; cluster_it != m_mapNodesLogical [wNwkAddr] [byteEp].end(); cluster_it++)
				{
					BYTE byteClusterId =  cluster_it->first;
				// store in the database with the hardcoded attributes with unknown values
					// dds: this used to crash
					ZiBAttributeRow attribute_row(
						*(int64_t*)&(m_mapNodesPhysical[wNwkAddr].mac_addr),
						byteEp,
						byteClusterId,
						0,
						string("<unknown>"));
					
					for (int i = 0; i < m_mapClusterAttributes[byteClusterId].size(); i++)
					{
						attribute_row.SetAttributeId(m_mapClusterAttributes[byteClusterId][i]);
						
						if (!m_db.FindAttribute(attribute_row))
						{
							m_db.InsertAttribute(attribute_row);
						}
					}
				}
			}
		}
	}
	m_csNetDiscovery.unlock();
}

void CZiBManager::DiscoveryClear(void)
{
	TRACE(DBG_LVL_DBG4, "CZiBManager::DiscoveryClear\n");
	m_csNetDiscovery.lock();
	m_wPanId = 0;
	m_mapNodesLogical.clear();
	m_mapNodesPhysical.clear();
	m_csNetDiscovery.unlock();
}

void CZiBManager::DiscoveryDumpPhysical(void)
{
	TRACE(DBG_LVL_DBG4, "CZiBManager::DiscoveryDumpPhysical\n");
	
	m_csNetDiscovery.lock();
	TRACE(DBG_LVL_DBG1 | DBG_MASK_NOHEADER, "\n**** START OF PHYSICAL DISCOVERY DUMP ****\n");
	NODE_PHYSICAL_MAP::iterator node_it = m_mapNodesPhysical.begin();
	for (; node_it != m_mapNodesPhysical.end(); node_it++)
	{
		ZiBNode* pNode = &node_it->second;
		TRACE(DBG_LVL_DBG1 | DBG_MASK_NOHEADER, "Node Mac <0x%02X%02X%02X%02X%02X%02X%02X%02X>, Nwk <0x%X>, DeviceId <0x%X>, ProfileId <0x%X>\n",
			pNode->mac_addr[7],
			pNode->mac_addr[6],
			pNode->mac_addr[5],
			pNode->mac_addr[4],
			pNode->mac_addr[3],
			pNode->mac_addr[2],
			pNode->mac_addr[1],
			pNode->mac_addr[0],
			pNode->nwk_addr,
			pNode->device_id,
			pNode->profile_id);
	}
	TRACE(DBG_LVL_DBG1 | DBG_MASK_NOHEADER, "**** END OF PHYSICAL DISCOVERY DUMP ****\n\n");

	m_csNetDiscovery.unlock();
}

void CZiBManager::DiscoveryDumpLogical(void)
{
	int iCounter = 0;
	TRACE(DBG_LVL_DBG4, "CZiBManager::DiscoveryDumpLogical\n");
	
	m_csNetDiscovery.lock();
	TRACE(DBG_LVL_DBG1 | DBG_MASK_NOHEADER, "\n**** START OF LOGICAL DISCOVERY DUMP ****\n");
	TRACE(DBG_LVL_DBG1 | DBG_MASK_NOHEADER, "PanId=<0x%X>\n", m_wPanId);
	NODE_LOGICAL_MAP::iterator node_it = m_mapNodesLogical.begin();
	for (; node_it != m_mapNodesLogical.end(); node_it++)
	{
		WORD wNwkAddr = (*node_it).first;

		EP_MAP::iterator ep_it = m_mapNodesLogical [wNwkAddr].begin();
		for (; ep_it != m_mapNodesLogical [wNwkAddr].end(); ep_it++)
		{
			BYTE byteEp =  (*ep_it).first;

			CLUSTER_MAP::iterator cluster_it = m_mapNodesLogical [wNwkAddr] [byteEp].begin();
			for (; cluster_it != m_mapNodesLogical [wNwkAddr] [byteEp].end(); cluster_it++)
			{
				BYTE byteClusterId =  (*cluster_it).first;
				TRACE(DBG_LVL_DBG1 | DBG_MASK_NOHEADER, "Node <0x%X>, Ep <0x%X>, Cluster <0x%X>\n", 
					wNwkAddr, byteEp, byteClusterId);
				iCounter++;
			}
		}
	}
	TRACE(DBG_LVL_DBG1 | DBG_MASK_NOHEADER, "**** END OF LOGICAL DISCOVERY DUMP ****\n\n");

	m_csNetDiscovery.unlock();
}

void CZiBManager::AttributeHandleStart(void)
{
	m_csNetDiscovery.lock();
	NODE_LOGICAL_MAP::iterator node_it = m_mapNodesLogical.begin();
	for (; node_it != m_mapNodesLogical.end(); node_it++)
	{
		WORD wNwkAddress = (*node_it).first;

		EP_MAP::iterator ep_it = m_mapNodesLogical [wNwkAddress].begin();
		for (; ep_it != m_mapNodesLogical [wNwkAddress].end(); ep_it++)
		{
			BYTE byteEp =  (*ep_it).first;

			CLUSTER_MAP::iterator cluster_it = m_mapNodesLogical [wNwkAddress] [byteEp].begin();
			for (; cluster_it != m_mapNodesLogical [wNwkAddress] [byteEp].end(); cluster_it++)
			{
				BYTE byteClusterId =  (*cluster_it).first;
				
			// sweep through the cluster's attributes
				for (int i = 0; i < m_mapClusterAttributes[byteClusterId].size(); i++)
				{
				// request the attributes
					m_csNetDiscovery.unlock();
					LONG lRet = AttributeValueReq(wNwkAddress, byteEp, byteClusterId, m_mapClusterAttributes[byteClusterId][i], false);
					m_csNetDiscovery.lock();

					if (lRet != ERROR_SUCCESS)
					{
						break;
					}

					string strAttribute = m_mapNodesLogical [wNwkAddress] [byteEp] [byteClusterId] [m_mapClusterAttributes[byteClusterId][i]];
					if (strAttribute.empty())
					{
						TRACE(DBG_LVL_DBG1, "CZiBManager::AttributeHandleStart - Received empty attribute. Ignoring...\n");
						break;
					}
				
				// update the attribute in the database (t_attributes row should already have been added by network discovery)
					m_db.UpdateAttribute(*(int64_t*)&(m_mapNodesPhysical[wNwkAddress].mac_addr),
						byteEp,
						byteClusterId,
						m_mapClusterAttributes[byteClusterId][i],
						strAttribute);

				// insert the history entry
					IBPP::Timestamp time;
					time.Now();
					m_db.InsertAttributeHistory(*(int64_t*)&(m_mapNodesPhysical[wNwkAddress].mac_addr),
						byteEp,
						byteClusterId,
						m_mapClusterAttributes[byteClusterId][i],
						time,
						strAttribute);

				// handle the thresholds
					string strValue;
					string strUpLimit;
					string strDownLimit;
					DWORD dwLimitAction;
					ZiBNodeRow node;

					m_db.GetAttributeAndLimits(*(int64_t*)&(m_mapNodesPhysical[wNwkAddress].mac_addr),
						byteEp,
						byteClusterId,
						m_mapClusterAttributes[byteClusterId][i],
						strValue,
						strUpLimit,
						strDownLimit,
						dwLimitAction);

					m_db.FindNode(*(int64_t*)&(m_mapNodesPhysical[wNwkAddress].mac_addr), &node);
					
					CAlarmEntry alarm(node.GetMacAddr(), 
						byteEp, 
						byteClusterId, 
						m_mapClusterAttributes[byteClusterId][i],
						strValue, 
						time,
						node.GetTag(), 
						strUpLimit, 
						strDownLimit, 
						dwLimitAction);
					
					int iCharIndex = 0;
					for (iCharIndex = strValue.find_first_not_of("0123456789."); iCharIndex != string::npos; iCharIndex = strValue.find_first_not_of("0123456789."))
					{
						strValue.replace(iCharIndex, 1, "");
					}
					for (iCharIndex = strUpLimit.find_first_not_of("0123456789."); iCharIndex != string::npos; iCharIndex = strUpLimit.find_first_not_of("0123456789."))
					{
						strUpLimit.replace(iCharIndex, 1, "");
					}
					for (iCharIndex = strDownLimit.find_first_not_of("0123456789."); iCharIndex != string::npos; iCharIndex = strDownLimit.find_first_not_of("0123456789."))
					{
						strDownLimit.replace(iCharIndex, 1, "");
					}

					float fValue = atof(strValue.c_str());
					float fUpLimit = atof(strUpLimit.c_str());
					float fDownLimit = atof(strDownLimit.c_str());

					if (!strUpLimit.empty() && !strDownLimit.empty())
					{
						if (fUpLimit > fDownLimit)
						{
							if (!(fDownLimit <= fValue && fValue <= fUpLimit))
							{
								m_alarms.SetAlarm(alarm);
							}
							else
							{
								m_alarms.ClearAlarm(alarm);
							}
						}
						else
						{
							// invalid threshold values
							TRACE(DBG_LVL_WARNING, "CZiBManager::AttributeHandleStart - Invalid Threshold set for node <0x%04X>\n", wNwkAddress);
						}
					}
				}
			}
		}
	}
	m_csNetDiscovery.unlock();
}

// INCOMING MESSAGES ///////////////////////////////////////////////////////////
//
void CZiBManager::ResetConf(BYTE byteStatus)
{
	TRACE(DBG_LVL_DEBUG, "CZiBManager::ResetConf - Status=<0x%X>\n", byteStatus);
}

void CZiBManager::PanIdConf(BYTE byteStatus, WORD wPanId)
{
	if (byteStatus == ZIB_ERROR_SUCCESS)
	{
		TRACE(DBG_LVL_DBG1, "CZiBManager::PanIdConf - PanId=<0x%X>\n", wPanId);
		m_csNetDiscovery.lock();
		m_wPanId = wPanId;
		m_csNetDiscovery.unlock();
	}
	else
	{
		TRACE(DBG_LVL_DEBUG, "CZiBManager::PanIdConf - Error. Status=<0x%X>\n", byteStatus);
	}
}

void CZiBManager::NodeListConf(BYTE byteStatus, vector<ZiBNode>& vecNodes)
{
	if (byteStatus == ZIB_ERROR_SUCCESS)
	{
		ZiBNode node;
		TRACE(DBG_LVL_DBG1, "CZiBManager::NodeListConf - Nodes: ");
		
		for (int iIndex = 0; iIndex<vecNodes.size(); iIndex++)
		{
			node = vecNodes[iIndex];
			TRACE(DBG_LVL_DBG1 | DBG_MASK_NOHEADER, "<0x%X> ", node.nwk_addr);
			
			// todo: store the complete 'node' data in the database

			// store the nwk_addr in the discovery struct
			m_csNetDiscovery.lock();
			m_mapNodesLogical [node.nwk_addr] = EP_MAP::map(); // empty map
			m_mapNodesPhysical [node.nwk_addr] = node;
			m_csNetDiscovery.unlock();
		}
		TRACE(DBG_LVL_DBG1 | DBG_MASK_NOHEADER, "\n");
	}
	else
	{
		TRACE(DBG_LVL_DEBUG, "CZiBManager::NodeListConf - Error. Status=<0x%X>\n", byteStatus);
	}
}

void CZiBManager::EpListConf(BYTE byteStatus, WORD wNwkAddress, vector<BYTE>& vecEps)
{
	if (byteStatus == ZIB_ERROR_SUCCESS)
	{
		TRACE(DBG_LVL_DBG1, "CZiBManager::EpListConf - Node=<0x%X>, Eps: ", wNwkAddress);
		for (int iIndex = 0; iIndex<vecEps.size(); iIndex++)
		{
			TRACE(DBG_LVL_DBG1 | DBG_MASK_NOHEADER, "<0x%X> ", vecEps[iIndex]);
			m_csNetDiscovery.lock();
			m_mapNodesLogical[wNwkAddress][vecEps[iIndex]] = CLUSTER_MAP::map(); // create an EP position for each EP
			m_csNetDiscovery.unlock();
		}
		TRACE(DBG_LVL_DBG1 | DBG_MASK_NOHEADER, "\n");
	}
	else
	{
		TRACE(DBG_LVL_DEBUG, "CZiBManager::EpListConf - Error. Status=<0x%X>\n", byteStatus);
	}
}

void CZiBManager::ClusterListConf(BYTE byteStatus, WORD wNwkAddress, BYTE byteEp, vector<BYTE>& vecClusters)
{
	if (byteStatus == ZIB_ERROR_SUCCESS)
	{
		TRACE(DBG_LVL_DBG1, "CZiBManager::ClusterListConf - Node=<0x%X>, Ep=<0x%X>, Clusters: ", wNwkAddress, byteEp);
		for (int iIndex = 0; iIndex<vecClusters.size(); iIndex++)
		{
			TRACE(DBG_LVL_DBG1 | DBG_MASK_NOHEADER, "<0x%X> ", vecClusters[iIndex]);
			m_csNetDiscovery.lock();
			m_mapNodesLogical [wNwkAddress] [byteEp] [vecClusters[iIndex]] = ATTRIBUTE_MAP::map(); // create a CLUSTER position for each CLUSTER
			m_csNetDiscovery.unlock();
		}
		TRACE(DBG_LVL_DBG1 | DBG_MASK_NOHEADER, "\n");
	}
	else
	{
		TRACE(DBG_LVL_DEBUG, "CZiBManager::ClusterListConf - Error. Status=<0x%X>\n", byteStatus);
	}
}

void CZiBManager::AttributeValueConf(BYTE byteStatus, 
	WORD wNwkAddress,
	BYTE byteEp,
	BYTE byteClusterId,
	WORD wAttributeId,
	BYTE byteSize,
	void* pData)
{
	if (byteStatus == ZIB_ERROR_SUCCESS)
	{
		char* pszDataBytes = new char[byteSize+1];

		memcpy(pszDataBytes, pData, byteSize);
		pszDataBytes[byteSize] = '\0';
		

		TRACE(DBG_LVL_DBG1, "CZiBManager::AttributeValueConf - Node=<0x%X>, Ep=<0x%X>, Cluster=<0x%X>, AttributeId=<0x%X>, Value=",
			wNwkAddress, byteEp, byteClusterId, wAttributeId);
		for (int iCount = 0; iCount < byteSize; iCount++)
		{
			TRACE(DBG_LVL_DBG1 | DBG_MASK_NOHEADER, "0x%X ", pszDataBytes[iCount]);
		}

		string strData((char*)pszDataBytes);
		TRACE(DBG_LVL_DBG1 | DBG_MASK_NOHEADER, " - ");
		TRACE(DBG_LVL_DBG1 | DBG_MASK_NOHEADER, strData.c_str());
		TRACE(DBG_LVL_DBG1 | DBG_MASK_NOHEADER, "\n");

		m_csNetDiscovery.lock();
		m_mapNodesLogical [wNwkAddress] [byteEp] [byteClusterId] [wAttributeId] = strData; // store the read value
		m_csNetDiscovery.unlock();

		delete pszDataBytes;
	}
	else
	{
		TRACE(DBG_LVL_DEBUG, "CZiBManager::AttributeValueConf - Error. Status=<0x%X>\n", byteStatus);
	}
}

void CZiBManager::ResetInd(void)
{
	TRACE(DBG_LVL_INFO, "CZiBManager::ResetInd - Sending the ResetEvent\n");
	SetEvent(m_hResetEvent); // signal to reset
}

void CZiBManager::GatewayDebugInd (const string& strTrace)
{
	TRACE(DBG_LVL_DBG4, "CZiBManager::GatewayDebugInd\n");

	string strTraceLocal = strTrace;
	static bool bLastTraceHasEol = true; // store the information about the last trace line... Did it finish with a line break?

// Remove the carriage returns
	int iCharIndex = 0;
	for (iCharIndex = strTraceLocal.find_first_of("\r"); iCharIndex != string::npos; iCharIndex = strTraceLocal.find_first_of("\r"))
	{
		strTraceLocal.replace(iCharIndex, 1, "");
	}
	

// Trace the header, if it is a new trace line
	if (bLastTraceHasEol)
	{
		TRACE(DBG_LVL_DBG3, "*GATEWAY* ");
	}

// Trace
	TRACE(DBG_LVL_DBG3 | DBG_MASK_NOHEADER, strTrace.c_str());

// Is this trace line finished?
	if (strTrace.find("\r\n") != string::npos)
	{
		bLastTraceHasEol = true;
	}
	else
	{
		bLastTraceHasEol = false;
	}

}
