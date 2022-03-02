// ATComm.cpp: implementation of the CATComm class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ATComm.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CATComm::CATComm()
:	m_dwResponseTimeout(DEFAULT_RESPONSE_TIMEOUT_MS),
	m_strLastResponse ("")
{
}

CATComm::~CATComm()
{

}


LONG CATComm::WriteCommand( const string& strCommand, EEndSequence eCmdEndSequence )
{
	string strCommandTmp = strCommand;

	switch(eCmdEndSequence)
	{
		case EEndSequenceNone:
			strCommandTmp += AT_NONE;
			break;
		case EEndSequenceCtrlZ:
			strCommandTmp += AT_CTRL_Z;
			break;
		case EEndSequenceEol:
			strCommandTmp += AT_EOL;
			break;
	}

	TRACE(DBG_LVL_DBG4, "<<< Sending command <<<\n----------------------\n%s\n----------------------\n\n", strCommandTmp.c_str());
	return CSerial::Write(strCommandTmp.c_str());
}

LONG CATComm::ReadResponse(EEndSequence eRespEndSequence)
{
	LONG	lLastError = ERROR_SUCCESS;
	bool	bLeave = false;

	m_strLastResponse = "";

	do
	{
		while ( (lLastError = CSerial::WaitEvent(NULL,m_dwResponseTimeout)) == ERROR_SUCCESS)
		{
			if (CSerial::GetEventType() & CSerial::EEventRecv)
			{
				// TRACE("1. loop\n");
				// Read data, until there is nothing left
				DWORD dwBytesRead, dwBytesCount = 0;
				char szBuffer[101]; // todo: choose this number
				do
				{
					// Read data from the COM-port
					lLastError = CSerial::Read(&szBuffer[dwBytesCount],sizeof(szBuffer)-1-dwBytesCount,&dwBytesRead,NULL/*, 50*/);
					if (lLastError != ERROR_SUCCESS)
						return ::ShowError(CSerial::GetLastError(), "Unable to read from -COMport.");

					if (dwBytesRead > 0)
					{
						// TRACE("2. loop - read %d bytes\n", dwBytesRead);
						dwBytesCount += dwBytesRead;
					}
				}
				while (dwBytesRead != 0);
				
				if( dwBytesCount )
				{
					szBuffer[dwBytesCount] = '\0';
					m_strLastResponse += szBuffer;
				}
			}
		}

		// verify whether the buffer contains the end character at the end

		if (lLastError == ERROR_TIMEOUT)
		{
			if(m_strLastResponse.length()!=0)
			{
				// timed out, but we got chars. let's check if we got an end of sequence, otherwise try to wait a little bit longer.
				string::size_type pos;
				switch(eRespEndSequence)
				{
					case EEndSequenceNone:
						bLeave = true;
						lLastError = ERROR_SUCCESS;
						break;

					case EEndSequenceCtrlZ:
						pos = m_strLastResponse.rfind(AT_CTRL_Z);
						if( pos == m_strLastResponse.length()-1 )
						{
							bLeave = true;
							lLastError = ERROR_SUCCESS;
						}
						break;

					case EEndSequenceEol:
						pos = m_strLastResponse.rfind(AT_EOL);
						if( pos == m_strLastResponse.length()-2 )
						{
							bLeave = true;
							lLastError = ERROR_SUCCESS;
						}
						break;
				}
			}
			else
			{
				bLeave = true; // timed out and no char was received... last error is signaled as timeout
			}
		}
		else
		{
			bLeave = true;
		}
	} while (!bLeave);

	if(lLastError == ERROR_SUCCESS)
	{
		TRACE(DBG_LVL_DBG4, "<<< Received response <<<\n----------------------\n%s\n----------------------\n\n", GetResponseBuffer().c_str());
	}
	return lLastError;
}

string& CATComm::WriteAndRead(	const string& strCommand,
								DWORD dwWaitBeforeAnswer,
								EEndSequence eCmdEndSequence,
								EEndSequence eRespEndSequence )
{
	if( WriteCommand(strCommand, eCmdEndSequence) == ERROR_SUCCESS )
	{
		if(dwWaitBeforeAnswer)
		{
			Sleep(dwWaitBeforeAnswer);
		}
		ReadResponse(eRespEndSequence);
	}

	return GetResponseBuffer();
}

string& CATComm::GetResponseBuffer()
{
	return m_strLastResponse;
}


void CATComm::SetResponseTimeout( DWORD dwResponseTimeout)
{
	m_dwResponseTimeout = dwResponseTimeout;
}

DWORD CATComm::GetResponseTimeout( void )
{
	return m_dwResponseTimeout;
}
