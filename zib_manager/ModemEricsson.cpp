// Modem.cpp: implementation of the CModemEricsson class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ModemEricsson.h"
#include <modem/Pdu.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CModemEricsson::CModemEricsson ()
{

}

CModemEricsson::~CModemEricsson ()
{

}

bool CModemEricsson::InitializationCommands ()
{
	lock locker;

	m_bInitialized = true;
	return true;
}

bool CModemEricsson::SendSms (string strDestinationNumber, string strMessage)
{
	lock locker;

	string strPduBuffer, strPreCmd, strCmd;
	char szFmtPreCmd[MAX_MODEM_MESSAGE];
	PduSubmit objPdu (strDestinationNumber, strMessage);

	if (!objPdu.EncodeRawPdu())
	{
		objPdu.GetRawPdu (strPduBuffer);
		strCmd = strPduBuffer;

		sprintf(szFmtPreCmd,
			"%s%d",
			"AT+CMGS=",
			( ( strCmd.length () ) / 2 ) - 1 );
		strPreCmd = szFmtPreCmd;
		
		CATComm::WriteAndRead (strPreCmd, 0, CATComm::EEndSequenceEol, CATComm::EEndSequenceNone).c_str (); // todo: create the <CR> ('\r') option
		CATComm::WriteAndRead (strCmd, 3000, CATComm::EEndSequenceCtrlZ).c_str ();

		return true;
	}
	else
	{
		return false;
	}
}