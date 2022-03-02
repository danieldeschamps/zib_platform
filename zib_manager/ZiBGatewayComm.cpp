#include "Stdafx.h"
#include "ZiBGatewayComm.h"

#define WAIT_FOR_CONF_TIMEOUT_MS			3000
#define WAIT_FOR_OUT_MSG_SEND_TIMEOUT_MS	50

IZiBGatewayComm::IZiBGatewayComm ()
:	m_smRx (ERxState::ERxStateWaitingStartByte),
	m_hThreadProcessInMsg (0)
{
	m_hNewInMsgEvent = ::CreateEvent (NULL, FALSE, FALSE, NULL);
	m_hStopEvent = ::CreateEvent (NULL, FALSE, FALSE, NULL);
	m_hConfEvent = ::CreateEvent (NULL, FALSE, FALSE, NULL);
}

IZiBGatewayComm::~IZiBGatewayComm ()
{
	// Check if the thread handle is still there. If so, then we
	// didn't close the serial port. We cannot depend on the
	// CSerial destructor, because if it calls Close then it
	// won't call our overridden Close.
	if (m_hThread)
	{
		// Display a warning
		// _RPTF0(_CRT_WARN, "IZiBGatewayComm::~IZiBGatewayComm - Serial port not closed\n");

		// Close implicitly
		StopSerialPort();
	}

	if (m_hThreadProcessInMsg) 
	{
		if (StopThreadProcessInMsg(5000) != ERROR_SUCCESS)
		{
			TRACE (DBG_LVL_WARNING, "IZiBGatewayComm::~IZiBGatewayComm - ThreadProcessInMsg is probably stuck\n");
		}
	}

	::CloseHandle (m_hNewInMsgEvent);
	::CloseHandle (m_hStopEvent);
	::CloseHandle (m_hConfEvent);
}

LONG IZiBGatewayComm::StartSerialPort (const string& strPort,
	CSerial::EBaudrate eBaudrate,
	CSerial::EDataBits eDataBits,
	CSerial::EParity   eParity,
	CSerial::EStopBits eStopBits,
	CSerial::EHandshake eHandshake )
{
	LONG lLastError = ERROR_SUCCESS;
	
	StopSerialPort ();

	// custom
	if ((lLastError = CSerialEx::Open (strPort.c_str())) == ERROR_SUCCESS)
	{
		if ((lLastError = CSerialEx::Setup (eBaudrate,eDataBits,eParity,eStopBits)) == ERROR_SUCCESS)
		{
			if ((lLastError = CSerialEx::SetupHandshaking (eHandshake)) == ERROR_SUCCESS)
			{
				// hardcoded for modem
				if ((lLastError = CSerialEx::SetMask (CSerial::EEventRecv | CSerial::EEventSend)) == ERROR_SUCCESS) // todo: remove this eventsend from here... I dont want it
				{
					CSerial::Purge();
					// Start the listener thread
					lLastError = StartListener ();
				}
			}
		}
	}

	if (lLastError != ERROR_SUCCESS)
	{
		// Close the serial port
		StopSerialPort ();

		// Return the error-code
		m_lLastError = lLastError;
	}

	// Return the error
	return m_lLastError;
}

LONG IZiBGatewayComm::StopSerialPort (void)
{
	// Do your stuff

	// Call the base class
	return CSerialEx::Close ();
}

LONG IZiBGatewayComm::SendMessage (ZiBMessage& msg)
{
	BYTE byteStart = ZIB_COMM_START_BYTE;
	BYTE* pData = (BYTE*)&msg;
	LONG lError;
	
	if ( (lError = Write (&byteStart, 1)) == ERROR_SUCCESS )
		lError = Write (pData, msg.length);

	// Sleep(WAIT_FOR_OUT_MSG_SEND_TIMEOUT_MS); // dds: Let's give a breath for the PC USART... This sleep is needed for massive communication

	return lError;
}

// Protocol REQUESTS /////////////////////////////////
//

//	ZIB_MSG_CODE_RESET_REQ,
LONG IZiBGatewayComm::ResetReq (bool bAsynchronous)
{
	TRACE(DBG_LVL_DEBUG, "IZiBGatewayComm::ResetReq\n");
	ZiBMessage msg = {0};
	msg.code = ZIB_MSG_CODE_RESET_REQ;
	msg.length = sizeof msg.type.reset_req + ZIB_MSG_HEADER_SIZE;
	msg.type.reset_req.reserved = 0;

	::ResetEvent (m_hConfEvent);
	m_lLastError = SendMessage (msg);
	if (m_lLastError == ERROR_SUCCESS && !bAsynchronous)
	{
		if (::WaitForSingleObject (m_hConfEvent, WAIT_FOR_CONF_TIMEOUT_MS) == WAIT_TIMEOUT)
		{
			TRACE(DBG_LVL_DBG1, "IZiBGatewayComm::ResetReq - Timeout while waiting for the CONFIRMATION in synchronous mode\n");
			m_lLastError = ERROR_TIMEOUT;
		}
	}
	TRACE(DBG_LVL_DBG4, "IZiBGatewayComm::ResetReq - Returning\n");
	return m_lLastError;
}

//	ZIB_MSG_CODE_PAN_ID_REQ,
LONG IZiBGatewayComm::PanIdReq (bool bAsynchronous)
{
	TRACE(DBG_LVL_DBG1, "IZiBGatewayComm::PanIdReq\n");
	ZiBMessage msg = {0};
	msg.code = ZIB_MSG_CODE_PAN_ID_REQ;
	msg.length = sizeof msg.type.pan_id_req + ZIB_MSG_HEADER_SIZE;
	msg.type.pan_id_req.reserved = 0;
	
	::ResetEvent (m_hConfEvent);
	m_lLastError = SendMessage (msg);
	if (m_lLastError == ERROR_SUCCESS && !bAsynchronous)
	{
		if (::WaitForSingleObject (m_hConfEvent, WAIT_FOR_CONF_TIMEOUT_MS) == WAIT_TIMEOUT)
		{
			TRACE(DBG_LVL_DBG1, "IZiBGatewayComm::PanIdReq - Timeout while waiting for the CONFIRMATION in synchronous mode\n");
			m_lLastError = ERROR_TIMEOUT;
		}
	}
	TRACE(DBG_LVL_DBG4, "IZiBGatewayComm::PanIdReq - Returning\n");
	return m_lLastError;
}

//	ZIB_MSG_CODE_NODE_LIST_REQ
LONG IZiBGatewayComm::NodeListReq (bool bAsynchronous)
{
	TRACE(DBG_LVL_DBG1, "IZiBGatewayComm::NodeListReq\n");
	ZiBMessage msg = {0};
	msg.code = ZIB_MSG_CODE_NODE_LIST_REQ;
	msg.length = sizeof msg.type.node_list_req + ZIB_MSG_HEADER_SIZE;
	msg.type.node_list_req.reserved = 0;

	::ResetEvent (m_hConfEvent);
	m_lLastError = SendMessage (msg);
	if (m_lLastError == ERROR_SUCCESS && !bAsynchronous)
	{
		if (::WaitForSingleObject (m_hConfEvent, WAIT_FOR_CONF_TIMEOUT_MS) == WAIT_TIMEOUT)
		{
			TRACE(DBG_LVL_DBG1, "IZiBGatewayComm::NodeListReq - Timeout while waiting for the CONFIRMATION in synchronous mode\n");
			m_lLastError = ERROR_TIMEOUT;
		}
	}
	TRACE(DBG_LVL_DBG4, "IZiBGatewayComm::NodeListReq - Returning\n");
	return m_lLastError;
}

//	ZIB_MSG_CODE_EP_LIST_REQ
LONG IZiBGatewayComm::EpListReq (WORD wNwkAddress, bool bAsynchronous)
{
	TRACE(DBG_LVL_DBG1, "IZiBGatewayComm::EpListReq - Node=<0x%X>\n", wNwkAddress);
	ZiBMessage msg = {0};
	msg.code = ZIB_MSG_CODE_EP_LIST_REQ;
	msg.length = sizeof msg.type.ep_list_req + ZIB_MSG_HEADER_SIZE;
	msg.type.ep_list_req.nwk_addr = wNwkAddress;

	::ResetEvent (m_hConfEvent);
	m_lLastError = SendMessage (msg);
	if (m_lLastError == ERROR_SUCCESS && !bAsynchronous)
	{
		if (::WaitForSingleObject (m_hConfEvent, WAIT_FOR_CONF_TIMEOUT_MS) == WAIT_TIMEOUT)
		{
			TRACE(DBG_LVL_DBG1, "IZiBGatewayComm::EpListReq - Timeout while waiting for the CONFIRMATION in synchronous mode\n");
			m_lLastError = ERROR_TIMEOUT;
		}
	}
	TRACE(DBG_LVL_DBG4, "IZiBGatewayComm::EpListReq - Returning\n");
	return m_lLastError;
}

//	ZIB_MSG_CODE_CLUSTER_LIST_REQ
LONG IZiBGatewayComm::ClusterListReq (WORD wNwkAddress, BYTE byteEp, bool bAsynchronous)
{
	TRACE(DBG_LVL_DBG1, "IZiBGatewayComm::ClusterListReq - Node=<0x%X>, Ep=<0x%X>\n", wNwkAddress, byteEp);
	ZiBMessage msg = {0};
	msg.code = ZIB_MSG_CODE_CLUSTER_LIST_REQ;
	msg.length = sizeof msg.type.cluster_list_req + ZIB_MSG_HEADER_SIZE;
	msg.type.cluster_list_req.nwk_addr = wNwkAddress;
	msg.type.cluster_list_req.ep = byteEp;

	::ResetEvent (m_hConfEvent);
	m_lLastError = SendMessage (msg);
	if (m_lLastError == ERROR_SUCCESS && !bAsynchronous)
	{
		if (::WaitForSingleObject (m_hConfEvent, WAIT_FOR_CONF_TIMEOUT_MS) == WAIT_TIMEOUT)
		{
			TRACE(DBG_LVL_DBG1, "IZiBGatewayComm::ClusterListReq - Timeout while waiting for the CONFIRMATION in synchronous mode\n");
			m_lLastError = ERROR_TIMEOUT;
		}
	}
	TRACE(DBG_LVL_DBG4, "IZiBGatewayComm::ClusterListReq - Returning\n");
	return m_lLastError;
}

//	ZIB_MSG_CODE_GET_ATTRIBUTE_VALUE_REQ
LONG IZiBGatewayComm::AttributeValueReq (WORD wNwkAddress, 
	BYTE byteEp, 
	BYTE byteClusterId, 
	WORD wAttributeId,
	bool bAsynchronous)
{
	TRACE(DBG_LVL_DBG1, "IZiBGatewayComm::AttributeValueReq - Node=<0x%X>, Ep=<0x%X>, Cluster=<0x%X>, Attribute=<0x%X>\n", wNwkAddress, byteEp, byteClusterId, wAttributeId);
	ZiBMessage msg = {0};
	msg.code = ZIB_MSG_CODE_GET_ATTRIBUTE_VALUE_REQ;
	msg.length = sizeof msg.type.get_attribute_value_req + ZIB_MSG_HEADER_SIZE;
	msg.type.get_attribute_value_req.nwk_addr = wNwkAddress;
	msg.type.get_attribute_value_req.ep = byteEp;
	msg.type.get_attribute_value_req.cluster = byteClusterId;
	msg.type.get_attribute_value_req.attribute_id = wAttributeId;

	::ResetEvent (m_hConfEvent);
	m_lLastError = SendMessage (msg);
	if (m_lLastError == ERROR_SUCCESS && !bAsynchronous)
	{
		if (::WaitForSingleObject (m_hConfEvent, WAIT_FOR_CONF_TIMEOUT_MS) == WAIT_TIMEOUT)
		{
			TRACE(DBG_LVL_DBG1, "IZiBGatewayComm::AttributeValueReq - Timeout while waiting for the CONFIRMATION in synchronous mode\n");
			m_lLastError = ERROR_TIMEOUT;
		}
	}
	TRACE(DBG_LVL_DBG4, "IZiBGatewayComm::AttributeValueReq - Returning\n");
	return m_lLastError;
}

//	ZIB_MSG_CODE_SET_ATTRIBUTE_VALUE_REQ
// todo

// Protocol RESPONSES /////////////////////////////////
LONG IZiBGatewayComm::ResetResp (BYTE byteStatus)
{
	TRACE(DBG_LVL_DEBUG, "IZiBGatewayComm::ResetResp\n");
	ZiBMessage msg = {0};
	msg.code = ZIB_MSG_CODE_RESET_RESP;
	msg.length = sizeof msg.type.reset_resp + ZIB_MSG_HEADER_SIZE;
	msg.type.reset_resp.status = byteStatus;

	LONG lRet = SendMessage (msg);
	TRACE(DBG_LVL_DBG4, "IZiBGatewayComm::ResetResp - Returning\n");
	return lRet;
}

// THREAD Serial comm thread -> puts gateway messages in queue
void IZiBGatewayComm::OnEvent (EEvent eEvent, EError eError)
{
	BYTE pData[256];
	DWORD dwByteCount, dwIndex=0;

	// Do your stuff
	// switch (eEvent)
	{
		if ( eEvent & CSerial::EEvent::EEventRecv )
		{
			Read (pData, sizeof pData, &dwByteCount);
			
			while (dwByteCount--)
			{
				switch (m_smRx)
				{
					case ERxState::ERxStateWaitingStartByte:
					{
						if (pData[dwIndex] == ZIB_COMM_START_BYTE)
						{
							m_smRx = ERxState::ERxStateWaitingEndOfMessage;
							// todo: start a timer (ZIB_COMM_TIMEOUT_AFTER_START_BYTE_MS)
						}
						break;
					}

					case ERxState::ERxStateWaitingEndOfMessage:
					{
						m_deqRxBuffer.push_back(pData[dwIndex]);

						// todo: clear the SM and the buffer if the timer expires
						if (m_deqRxBuffer.size() == m_deqRxBuffer[0]) // did we receive all data?
						{
							if (m_deqRxBuffer.size() > ZIB_MSG_HEADER_SIZE) // Verify minimum message size
							{
								TRACE (DBG_LVL_DBG4, "IZiBGatewayComm::OnEvent - Pushing a new message into the stack. Length=<%u bytes>\n", m_deqRxBuffer[0]);
								PushInMsgStack (m_deqRxBuffer);
								::SetEvent (m_hNewInMsgEvent);
							}
							else
							{
								TRACE (DBG_LVL_DEBUG, "Discarding an invalid message\n");
							}

							m_deqRxBuffer.clear();
							m_smRx = ERxState::ERxStateWaitingStartByte;
						}
						else if (m_deqRxBuffer.size() > m_deqRxBuffer[0])
						{
							TRACE(DBG_LVL_ERROR, "IZiBGatewayComm::OnEvent - Got more data in the buffer then the message size! Discarding buffer\n");
							m_deqRxBuffer.clear();
							m_smRx = ERxState::ERxStateWaitingStartByte;
						}
						break;
					}
				}

				dwIndex++;
			}
		}

		if (eEvent & CSerial::EEvent::EEventSend)
		{
			TRACE(DBG_LVL_DBG4, "IZiBGatewayComm::OnEvent - CSerial::EEventSend -> All data has been sent!\n");
		}
	}
}

// de-serializes into a newly allocated memory struct
void IZiBGatewayComm::PushInMsgStack (deque<BYTE>& deqMsg)
{
	m_csInMsgStack.lock ();
	deque<BYTE>* pMsg = new deque<BYTE>;
	*pMsg = deqMsg;
	m_deqInMsgStack.push_back (pMsg);
	m_csInMsgStack.unlock ();
}


bool IZiBGatewayComm::PopInMsgStack (deque<BYTE>& deqMsg)
{
	bool bRet = false;
	deque<BYTE>* pMsg;

	m_csInMsgStack.lock();
	if(m_deqInMsgStack.size ()!=0)
	{
		pMsg = m_deqInMsgStack.front ();
		deqMsg = *pMsg;
		m_deqInMsgStack.pop_front ();
		delete pMsg;
		bRet = true;
	}
	else
	{
		bRet = false;
	}
	m_csInMsgStack.unlock ();
	return bRet;
}

LONG IZiBGatewayComm::StartThreadProcessInMsg ()
{
	// Check if the watcher thread was already running
	if (!m_hThreadProcessInMsg)
	{
		// Start the watcher thread
		DWORD dwThreadId = 0;
		m_hThreadProcessInMsg = ::CreateThread (0,0,ThreadProcessInMsg,LPVOID(this),0,&dwThreadId);
		if (m_hThreadProcessInMsg == 0)
		{
			// Display a warning
			TRACE (DBG_LVL_WARNING, "IZiBGatewayComm::StartThreadProcessInMsg - Unable to start thread\n");

			// Set the error code and exit
			m_lLastError = ::GetLastError ();
			return m_lLastError;
		}
	}

	// Return the error
	m_lLastError = ERROR_SUCCESS;
	return m_lLastError;
}

LONG IZiBGatewayComm::StopThreadProcessInMsg (DWORD dwTimeout)
{
	TRACE(DBG_LVL_DEBUG, "IZiBGatewayComm::StopThreadProcessInMsg - Trying to stop ProcessInMsg Thread\n");
	// Check if the thread is running
	if (m_hThreadProcessInMsg)
	{
		// Set the flag that the thread must be stopped
		::SetEvent (m_hStopEvent);
		
		// Wait until the watcher thread has stopped
		if (::WaitForSingleObject (m_hThreadProcessInMsg,dwTimeout) == WAIT_TIMEOUT)
		{
			TRACE (DBG_LVL_DBG1, "IZiBGatewayComm::StopThreadProcessInMsg - TIMEOUT while waiting\n");
			// todo: use brute force in case dwTimeout expires
		}

		// Close handle to the thread
		::CloseHandle (m_hThreadProcessInMsg);
		m_hThreadProcessInMsg = 0;
	}
	else
	{
		TRACE (DBG_LVL_DEBUG, "IZiBGatewayComm::StopThreadProcessInMsg - The thread was already stoped\n");
	}

	// Return the error
	m_lLastError = ERROR_SUCCESS;
	return m_lLastError;
}

DWORD WINAPI IZiBGatewayComm::ThreadProcessInMsg (LPVOID lpArg)
{
	// Route the method to the actual object
	IZiBGatewayComm* pThis = reinterpret_cast<IZiBGatewayComm*> (lpArg);
	return pThis->ThreadProcessInMsg ();
}

// THREAD Collect gateway messages thread -> takes gateway messages from queue and processes upwards
DWORD IZiBGatewayComm::ThreadProcessInMsg (void)
{
	DWORD dwEvent;
	HANDLE m_hThreadProcessInMsgEventList[2];

	m_hThreadProcessInMsgEventList[0] = m_hNewInMsgEvent;
	m_hThreadProcessInMsgEventList[1] = m_hStopEvent;
	
	while ((dwEvent = WaitForMultipleObjects (2, m_hThreadProcessInMsgEventList, FALSE, INFINITE)) != WAIT_TIMEOUT)
	{
		if (dwEvent == WAIT_OBJECT_0) // NewInMsgEvent
		{
			TRACE(DBG_LVL_DBG4, "IZiBGatewayComm::ThreadProcessInMsg - Received a NewInMsgEvent\n");

			deque<BYTE> deqMsg;
			while (PopInMsgStack (deqMsg))
			{
				switch (deqMsg[1]) // message code
				{
				// INDICATIONS
					case ZIB_MSG_CODE_RESET_IND:
						ResetIndParser (deqMsg);
						break;
					case ZIB_MSG_CODE_GATEWAY_DEBUG_IND:
						GatewayDebugIndParser (deqMsg);
						break;
				
				// CONFIRMATIONS
					case ZIB_MSG_CODE_RESET_CONF:
						ResetConfParser (deqMsg);
						::SetEvent (m_hConfEvent);
						break;
					case ZIB_MSG_CODE_PAN_ID_CONF:
						PanIdConfParser (deqMsg);
						::SetEvent (m_hConfEvent);
						break;
					case ZIB_MSG_CODE_NODE_LIST_CONF:
						NodeListConfParser (deqMsg);
						::SetEvent (m_hConfEvent);
						break;
					case ZIB_MSG_CODE_EP_LIST_CONF:
						EpListConfParser (deqMsg);
						::SetEvent (m_hConfEvent);
						break;
					case ZIB_MSG_CODE_CLUSTER_LIST_CONF:
						ClusterListConfParser (deqMsg);
						::SetEvent (m_hConfEvent);
						break;
					case ZIB_MSG_CODE_GET_ATTRIBUTE_VALUE_CONF:
						AttributeValueConfParser (deqMsg);
						::SetEvent (m_hConfEvent);
						break;
					/*case ZIB_MSG_CODE_SET_ATTRIBUTE_VALUE_CONF:
						::SetEvent (m_hConfEvent);
						break;*/

					default:
						TRACE (DBG_LVL_DEBUG, "IZiBGatewayComm::ThreadProcessInMsg - Invalid message code <0x%X>\n", deqMsg[1]);
						break;
				}				
			}
		}
		else if (dwEvent == WAIT_OBJECT_0 + 1) // StopEvent
		{
			TRACE(DBG_LVL_DEBUG, "IZiBGatewayComm::ThreadProcessInMsg - Received a StopEvent\n");
			break;
		}

	}

	TRACE (DBG_LVL_DEBUG, "IZiBGatewayComm::ThreadProcessInMsg - Exiting\n", dwEvent);

	// Bye bye
	return 0;
}

// Parsers ////////////////////////////////////////////
void IZiBGatewayComm::ResetConfParser (deque<BYTE>& deqMsg)
{
	ResetConf (deqMsg[2]);
}

void IZiBGatewayComm::PanIdConfParser (deque<BYTE>& deqMsg)
{
	ZiBMessage* msg = (ZiBMessage*)ParserSerializeMsg (deqMsg);

	PanIdConf (msg->type.pan_id_conf.status, msg->type.pan_id_conf.pan_id);
	ParserSerializeMsgFree ();
}

void IZiBGatewayComm::NodeListConfParser (deque<BYTE>& deqMsg)
{
	ZiBMessage* msg = (ZiBMessage*)ParserSerializeMsg (deqMsg);
	ZiBNode* pNode = (ZiBNode*)&msg->type.node_list_conf.node_list;	
	vector<ZiBNode> vecNodes;
	
	for (int iCount = 0; iCount < msg->type.node_list_conf.count; iCount++)
	{
		vecNodes.push_back (pNode[iCount]);
	}
	
	NodeListConf (msg->type.node_list_conf.status, vecNodes);
	ParserSerializeMsgFree ();
}

void IZiBGatewayComm::EpListConfParser (deque<BYTE>& deqMsg)
{
	ZiBMessage* msg = (ZiBMessage*)ParserSerializeMsg (deqMsg);
	BYTE* pEp = (BYTE*)&msg->type.ep_list_conf.ep_list;	
	vector<BYTE> vecEps;
	
	for (int iCount = 0; iCount < msg->type.ep_list_conf.count; iCount++)
	{
		vecEps.push_back (pEp[iCount]);
	}
	
	EpListConf (msg->type.ep_list_conf.status, msg->type.ep_list_conf.nwk_addr, vecEps);
	ParserSerializeMsgFree ();
}

void IZiBGatewayComm::ClusterListConfParser (deque<BYTE>& deqMsg)
{
	ZiBMessage* msg = (ZiBMessage*)ParserSerializeMsg (deqMsg);
	BYTE* pCluster = (BYTE*)&msg->type.cluster_list_conf.cluster_list;	
	vector<BYTE> vecClusters;;
	
	for (int iCount = 0; iCount < msg->type.cluster_list_conf.count; iCount++)
	{
		vecClusters.push_back (pCluster[iCount]);
	}
	
	ClusterListConf (msg->type.cluster_list_conf.status, 
		msg->type.cluster_list_conf.nwk_addr, 
		msg->type.cluster_list_conf.ep, 
		vecClusters);
	ParserSerializeMsgFree ();
}

void IZiBGatewayComm::AttributeValueConfParser (deque<BYTE>& deqMsg)
{
	ZiBMessage* msg = (ZiBMessage*)ParserSerializeMsg (deqMsg);

	AttributeValueConf (msg->type.get_attribute_value_conf.status,
		msg->type.get_attribute_value_conf.nwk_addr,
		msg->type.get_attribute_value_conf.ep,
		msg->type.get_attribute_value_conf.cluster,
		msg->type.get_attribute_value_conf.attribute_id,
		msg->type.get_attribute_value_conf.size,
		&msg->type.get_attribute_value_conf.value);
	ParserSerializeMsgFree ();
}

void IZiBGatewayComm::ResetIndParser (deque<BYTE>& deqMsg)
{
	ResetInd ();
}

void IZiBGatewayComm::GatewayDebugIndParser (deque<BYTE>& deqMsg)
{
	ZiBMessage* msg = (ZiBMessage*)ParserSerializeMsg (deqMsg);

	string strTrace = (const char*) &msg->type.gateway_debug_ind.trace;

	GatewayDebugInd (strTrace);
	ParserSerializeMsgFree ();
}

BYTE* IZiBGatewayComm::ParserSerializeMsg (deque<BYTE>& deqMsg)
{
	m_pSerializedMsg = (BYTE*)new char[deqMsg.size()];
	
	for (int i=0; i<deqMsg.size(); i++)
	{
		m_pSerializedMsg[i] = deqMsg[i];
	}

	return m_pSerializedMsg;
}

void IZiBGatewayComm::ParserSerializeMsgFree (void)
{
	delete m_pSerializedMsg;
}


