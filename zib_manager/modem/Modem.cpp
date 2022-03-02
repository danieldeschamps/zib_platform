#include "stdafx.h"
#include <modem/Modem.h>
#include <modem/Pdu.h>

CCriticalSection CModem::lock::m_cs;

CModem::CModem()
{

}

CModem::~CModem()
{

}

bool CModem::SetupSerialPort(	const string& strPort,
								CSerial::EBaudrate eBaudrate,
								CSerial::EDataBits eDataBits,
								CSerial::EParity   eParity,
								CSerial::EStopBits eStopBits,
								CSerial::EHandshake eHandshake )
{
	lock locker;

	LONG lLastError = ERROR_SUCCESS;
	
	CSerial::Close();

	// custom
	if( (lLastError = CSerial::Open(strPort.c_str())) == ERROR_SUCCESS )
	{
		if( (lLastError = CSerial::Setup(eBaudrate,eDataBits,eParity,eStopBits)) == ERROR_SUCCESS )
		{
			if( (lLastError = CSerial::SetupHandshaking(eHandshake)) == ERROR_SUCCESS )
			{
				// hardcoded for modem
				if( (lLastError = CSerial::SetMask(CSerial::EEventRecv)) == ERROR_SUCCESS )
				{
					lLastError = CSerial::SetupReadTimeouts(CSerial::EReadTimeoutNonblocking);
				}
			}
		}
	}

	if(lLastError != ERROR_SUCCESS)
	{
		ShowError(lLastError, "Failed to setup the Serial Port.");
		return false;
	}
	else
	{
		TRACE(DBG_LVL_DEBUG, "Serial Port successfully setup.\n");
		return true;
	}
}



bool CModem::TestCommunication( void )
{
	lock locker;

	TRACE(DBG_LVL_DBG4, "CModem::TestCommunication - Sending ATZ\n");

	if( CATComm::WriteAndRead("ATZ",500).find("OK") != string::npos )
	{
		TRACE(DBG_LVL_DBG4, "CModem::TestCommunication - The modem answered OK\n");
		return true;
	}
	else
	{
		TRACE(DBG_LVL_DEBUG, "CModem::TestCommunication - ATZ Error\n");
		return false;
	}
}

bool CModem::InitializationCommands()
{
	lock locker;

	bool bRet = false;
	if( CSerial::IsOpen() )
	{
		string strModemInitCommands = 
			"ATZ\r\nATE0\r\nAT+CPIN?\r\nAT+CPIN=\"1414\"\r\n"
			"AT+CGMI\r\nAT+CGMM\r\nAT+CNMI=0,0,0,0\r\n"
			"AT+CMGF=0\r\nAT+CSMS=0\r\nAT+CSCS=\"GSM\"\r\n";
		string::size_type iPos = strModemInitCommands.find (AT_EOL,0);
		string::size_type iPosBefore = 0;
		string strRead;
		bool bNeedPin = true;

		for( ; iPos < strModemInitCommands.length() ; iPos = strModemInitCommands.find (AT_EOL,iPosBefore) )
		{
			string strCmd = strModemInitCommands.substr(iPosBefore, iPos - iPosBefore);
			iPosBefore = iPos + sizeof(AT_EOL) - 1;

			// TRACE("<<< Sending command <<<\n----------------------\n%s\n----------------------\n\n", strCmd.c_str());

			if( strCmd.substr(0,8) == "AT+CPIN=" )
			{
				if( bNeedPin )
				{
					// Sleep(10000);
					WriteAndRead( strCmd, 10000 );
				}
			}
			else if( strCmd.substr(0,8) == "AT+CPIN?" )
			{
				// Sleep(3000);
				WriteAndRead( strCmd, 3000 );
				if( GetResponseBuffer().find("READY") != string::npos )
				{
					bNeedPin = false;
					continue;
				}
				else
				{
					TRACE(DBG_LVL_WARNING, "CModem::InitializationCommands - Error: SIM Card is not inserted?\n" );
				}
			}
			else if( strCmd.substr(0,8) == "AT+CSCS=" )
			{
				// Sleep(1000);
				WriteAndRead( strCmd, 1000 );
				if( GetResponseBuffer().find("OK") != string::npos ) // last command
				{
					bRet = true;
					m_bInitialized = true;
				}			
			}
			else
			{
				WriteAndRead( strCmd );
			}

			// TRACE("<<< Received response <<<\n----------------------\n%s\n----------------------\n\n", GetResponseBuffer().c_str());
			if( GetResponseBuffer().find("ERROR") != string::npos ) // break out on any error
			{
				TRACE(DBG_LVL_WARNING, "CModem::InitializationCommands - Command error detected.\n" );
				bRet = false;
				break;
			}
		}

		/*if ( !CreateThread( NULL, 0, ThreadReceiveSms, this, NULL, NULL ) )
		{
			iRet = 1;
			TRACE(_T("CSMServer::CSMServer - Error creating thread"));
		}
		else
		{
			iRet = 0;
		}*/
	}
	else
	{
		TRACE(DBG_LVL_DEBUG, "CModem::InitializationCommands - Error: Serial port not initialized.\n" );
		bRet = false;
	}

	return bRet;
}

bool CModem::GetInitialized()
{
	lock locker;

	return m_bInitialized;
}

bool CModem::SendSms( string strDestinationNumber, string strMessage )
{
	lock locker;

	string strPduBuffer, strPreCmd, strCmd;
	char szFmtPreCmd[MAX_MODEM_MESSAGE];
	PduSubmit objPdu( strDestinationNumber, strMessage );

	if( !objPdu.EncodeRawPdu() )
	{
		objPdu.GetRawPdu( strPduBuffer );
		strCmd = strPduBuffer;

		sprintf(	szFmtPreCmd,
					"%s%d%s",
					"AT+CMGS=",
					( ( strCmd.length() ) / 2 ) - 1,
					"\r" ); // some modems only work if I send only ctrl+z <CR>
		strPreCmd = szFmtPreCmd;
		
		CATComm::WriteAndRead( strPreCmd, 0, CATComm::EEndSequenceNone, CATComm::EEndSequenceNone ).c_str(); // todo: create the <CR> ('\r') option
		CATComm::WriteAndRead( strCmd, 3000, CATComm::EEndSequenceCtrlZ ).c_str();

		return true;
	}
	else
	{
		return false;
	}
}