#ifndef _MODEM_ERICSSON_H_
#define _MODEM_ERICSSON_H_

#include <modem/Modem.h>

class CModemEricsson : public CModem 
{
public:
	CModemEricsson ();
	virtual ~CModemEricsson ();
	
	bool InitializationCommands ();
	bool SendSms (string strDestinationNumber, string strMessage);
};

#endif // _MODEM_H_
