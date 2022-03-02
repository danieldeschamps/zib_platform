#ifndef _MODEM_H_
#define _MODEM_H_

#include <serial/ATComm.h>
#include <misc/CriticalSection.h>

class CModem : public CATComm 
{
public:
	CModem();
	virtual ~CModem();

	bool SetupSerialPort(	const string& strPort,
							CSerial::EBaudrate eBaudrate = CSerial::EBaud9600,
							CSerial::EDataBits eDataBits = CSerial::EData8,
							CSerial::EParity   eParity   = CSerial::EParNone,
							CSerial::EStopBits eStopBits = CSerial::EStop1,
							CSerial::EHandshake eHandshake = CSerial::EHandshakeOff );
	
	virtual bool TestCommunication( void );
	virtual bool InitializationCommands();
	virtual bool SendSms( string strDestinationNumber, string strMessage );

	bool GetInitialized();

protected:
	bool m_bInitialized;

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

#endif // _MODEM_H_
