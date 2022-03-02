#ifndef _ZIB_GATEWAY_COMM_H_
#define _ZIB_GATEWAY_COMM_H_

#include <serial/SerialEx.h>
#include <misc/CriticalSection.h>
#include <deque>
#include <vector>
#include <common/ZiBManagerGatewayProtocol.h>

class IZiBGatewayComm : public CSerialEx
{
public:
	typedef enum
	{
		ERxStateWaitingStartByte,
		ERxStateWaitingEndOfMessage
	} 
	ERxState;

// Construction
	IZiBGatewayComm ();
	virtual ~IZiBGatewayComm ();

// Operations
public:
	// Initialize the serial port before anything (starts the RX thread)
	LONG StartSerialPort (const string& strPort,
		CSerial::EBaudrate eBaudrate,
		CSerial::EDataBits eDataBits,
		CSerial::EParity   eParity,
		CSerial::EStopBits eStopBits,
		CSerial::EHandshake eHandshake);

	// Close the serial port.
	LONG StopSerialPort (void);

	// Start / Stop the thread which processes the incoming messages
	LONG StartThreadProcessInMsg (void);
	LONG StopThreadProcessInMsg (DWORD dwTimeout);

	// Appends the start byte and transmits the bytes to the ZiB Gateway
	LONG SendMessage (ZiBMessage& msg);
	
	// REQUESTS
	LONG ResetReq (bool bAsynchronous = true);
	LONG PanIdReq (bool bAsynchronous = true);
	LONG NodeListReq (bool bAsynchronous = true);
	LONG EpListReq (WORD wNwkAddress, bool bAsynchronous = true);
	LONG ClusterListReq (WORD wNwkAddress, BYTE byteEp, bool bAsynchronous = true);
	LONG AttributeValueReq (WORD wNwkAddress,
		BYTE byteEp, 
		BYTE byteClusterId, 
		WORD wAttributeId, 
		bool bAsynchronous = true);

	// CONFIRMATIONS
	virtual void ResetConf (BYTE byteStatus) = 0;
	virtual void PanIdConf (BYTE byteStatus, WORD wPanId) = 0;
	virtual void NodeListConf (BYTE byteStatus, vector<ZiBNode>& vecNodes) = 0;
	virtual void EpListConf (BYTE byteStatus, WORD wNwkAddress, vector<BYTE>& vecEps) = 0;
	virtual void ClusterListConf (BYTE byteStatus, WORD wNwkAddress, BYTE byteEp, vector<BYTE>& vecClusters) = 0;
	virtual void AttributeValueConf (BYTE byteStatus,
		WORD wNwkAddress,
		BYTE byteEp,
		BYTE byteClusterId,
		WORD wAttributeId,
		BYTE byteSize,
		void* pData) = 0;
	
	// INDICATIONS
	virtual void ResetInd (void) = 0;
	virtual void GatewayDebugInd (const string& strTrace) = 0;
	
	// RESPONSES
	LONG ResetResp (BYTE byteStatus);
	
protected:
	// Event handler that is called when a serial event occurs
	void OnEvent (EEvent eEvent, EError eError);

	// Incoming message storage
	deque<BYTE> m_deqRxBuffer;
	void PushInMsgStack (deque<BYTE>& deqMsg);
	bool PopInMsgStack (deque<BYTE>& deqMsg);
	CCriticalSection m_csInMsgStack;
	deque<deque<BYTE>*> m_deqInMsgStack; // shared object has to be critical section protected
	
	// Incoming message processor
	static DWORD WINAPI ThreadProcessInMsg (LPVOID lpArg);
	DWORD ThreadProcessInMsg (void);
	HANDLE m_hThreadProcessInMsg;
	HANDLE m_hNewInMsgEvent;
	HANDLE m_hStopEvent;

	// Communication state machine
	ERxState m_smRx;

	// Incoming message parsers
	void ResetConfParser (deque<BYTE>& deqMsg);
	void PanIdConfParser (deque<BYTE>& deqMsg);
	void NodeListConfParser (deque<BYTE>& deqMsg);
	void EpListConfParser (deque<BYTE>& deqMsg);
	void ClusterListConfParser (deque<BYTE>& deqMsg);
	void AttributeValueConfParser (deque<BYTE>& deqMsg);
	void ResetIndParser (deque<BYTE>& deqMsg);
	void GatewayDebugIndParser (deque<BYTE>& deqMsg);
	HANDLE m_hConfEvent;

	// Message serializer. Don't forget to free
	BYTE* ParserSerializeMsg (deque<BYTE>& deqMsg);
	void ParserSerializeMsgFree (void);
	BYTE* m_pSerializedMsg;
};

#endif	// _ZIB_GATEWAY_COMM_H_
