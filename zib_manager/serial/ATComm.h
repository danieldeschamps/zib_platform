#ifndef _AT_COMM_H_
#define _AT_COMM_H_

#include "Serial.h"

#define AT_NONE		""
#define AT_CTRL_Z	"\x1A"
#define AT_EOL		"\r\n"

#define DEFAULT_RESPONSE_TIMEOUT_MS		100

class CATComm : public CSerial  
{
public:
	CATComm();
	virtual ~CATComm();

	typedef enum
	{
		EEndSequenceNone,	// nothing
		EEndSequenceCtrlZ,	// ctrl+Z (0x1A)
		EEndSequenceEol,		// "\r\n"
	} 
	EEndSequence;

	// sends a command through the serial port
	LONG WriteCommand(	const string& strCommand, 
						EEndSequence eCmdEndSequence );

	// waits 'm_dwResponseTimeout' ms for blocks of response characters, appending them to the Response Buffer
	// if eRespEndSequence is defined, then this function blocks until the last received chars is eRespEndSequence
	LONG ReadResponse( EEndSequence eRespEndSequence = EEndSequenceEol );

	// easy write and read method
	// use GetLastError to poll the success
	string& WriteAndRead(	const string& strCommand, 
							DWORD dwWaitBeforeAnswer = 0,
							EEndSequence eCmdEndSequence = EEndSequenceEol,
							EEndSequence eRespEndSequence = EEndSequenceEol);
	
	// retrieves the last response, updated by ReadResponse or WriteAndRead methods
	string& GetResponseBuffer();

	// Set the timeout used by ReadResponse method.
	void SetResponseTimeout( DWORD dwResponseTimeout = DEFAULT_RESPONSE_TIMEOUT_MS );
	DWORD GetResponseTimeout( void );


protected:
	// internal variables
	string m_strLastResponse;
	DWORD m_dwResponseTimeout;
};

#endif // _AT_COMM_H_
