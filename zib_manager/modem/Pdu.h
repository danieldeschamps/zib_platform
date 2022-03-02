//////////////////////////////////////////////////////////////////////////
// Pdu.h
// Author:	DDS
// Purpose:	Implement pdu conversion object
// History:
// Date		Who	Comment
// 20050815	DDS	Created

//////////////////////////////////////////////////////////////////////////
// Includes

//////////////////////////////////////////////////////////////////////////
// Definitions
// General defines
#define MAX_PDU_LENGHT 512 //Max lenght of the Raw PDU. (TODO)
#define MAX_UD_LENGHT 512 //Max User Data Lenght (TODO)
#define MAX_ADDRESS_LENGHT 80 //Max User Address Lenght (TODO)
#define MAX_SMSMESSAGE_LENGHT	160 // Multiples of 4 because they are inside a struct

// Type of Number defines
#define NTYPE_MASK 0x70
#define NTYPE_MASK_SHIFT 4

#define NTYPE_UNKNOWN 0
#define NTYPE_INTERNATIONAL 1
#define NTYPE_NATIONAL 2
#define NTYPE_NETWORKSPECIFIC 3
#define NTYPE_SUBSCRIBER 4
#define NTYPE_ALPHANUMERIC 5
#define NTYPE_ABBREVIATED 6

// Numbering Plan Identification defines
#define NPLAN_MASK 0x0F
#define NPLAN_MASK_SHIFT 0

#define NPLAN_UNKNOWN 0
#define NPLAN_ISDN 1
#define NPLAN_DATA 3
#define NPLAN_TELEX 4
#define NPLAN_NATIONAL 8
#define NPLAN_PRIVATE 9
#define NPLAN_HERMES 10

// First Octect Deliver defines
#define FOD_MTI_MASK 0x03
#define FOD_MTI_SHIFT 0
#define FOD_MMS_MASK 0x04
#define FOD_MMS_SHIFT 2
#define FOD_SRI_MASK 0x20
#define FOD_SRI_SHIFT 5
#define FOD_UDHI_MASK 0x40
#define FOD_UDHI_SHIFT 6
#define FOD_RP_MASK 0x80
#define FOD_RP_SHIFT 7

#define MAX_NUMBER_LENGHT		64
#define MAX_MODEM_MESSAGE		512

using namespace std;

//////////////////////////////////////////////////////////////////////////
// Core stuff

struct Address
{
	//Bit no	7					6 5	4				3 2 1 0 
	//Name		Always set to 1		Type-of-number		Numbering Plan Identification

	BYTE	byteTypeOfNumber /*=NTYPE_INTERNATIONAL*/;	//0 0 0 Unknown. This is used when the user or network has no a priori information about the numbering plan. In this case, the Address-Value field is organized according to the network dialling plan, e.g. prefix or escape digits might be present. 
														//0 0 1 International number.  
														//0 1 0 National number. Prefix or escape digits shall not be included. 
														//0 1 1 Network specific number. This is used to indicate administration/service number specific to the serving network, e.g. used to access an operator. 
														//1 0 0 Subscriber number. This is used when a specific short number representation is stored in one or more SCs as part of a higher layer application. (Note that "Subscriber number" shall only be used in connection with the proper PID referring to this application).  
														//1 0 1 Alphanumeric, (coded according to GSM TS 03.38 7-bit default alphabet)  
														//1 1 0 Abbreviated number  
	BYTE	byteNumberingPlanId /*=	NPLAN_ISDN*/;	//0 0 0 0 Unknown. 
													//0 0 0 1 ISDN/telephone numbering plan (E.164/E.163). 
													//0 0 1 1 Data numbering plan (X.121). 
													//0 1 0 0 Telex numbering plan  
													//1 0 0 0 National numbering plan  
													//1 0 0 1 Private numbering plan 
													//1 0 1 0 ERMES numbering plan (ETSI DE/PS 3 01-3)  
	string	strAddress; // Tel. Number in decimal semi-octets with a trailing F to make even 
};

struct FirstOctetDeliver
{
	//Bit no	7		6		5		4			3			2		1		0 
	//Name		TP-RP	TP-UDHI	TP-SRI	(unused)	(unused)	TP-MMS	TP-MTI	TP-MTI

	BOOL	bRP/* =	FALSE*/;	//Reply path. Parameter indicating that reply path exists.
	BOOL	bUDHI/* =	FALSE*/;	//User data header indicator. This bit is set to 1 if the User Data field starts with a header
	BOOL	bSRI/* =	FALSE*/;	//Status report indication. This bit is set to 1 if a status report is going to be returned to the SME
	BOOL	bMMS/* =	TRUE*/;	//More messages to send. This bit is set to 0 if there are more messages to send 
	BOOL	bMTI/* =	FALSE*/;	//Message type indicator. Bits no 1 and 0 are both set to 0 to indicate that this PDU is a SMS-DELIVER
};

struct FirstOctetSubmit // Class for incoming SMS
{
	//Bit no	7		6		5		4 3		2		1 0 
	//Name		TP-RP	TP-UDHI	TP-SRR	TP-VPF	TP-RD	TP-MTI

	BOOL	bRP /*= FALSE*/;	//Reply path. Parameter indicating that reply path exists.
	BOOL	bUDHI /*= FALSE*/;	//User data header indicator. This bit is set to 1 if the User Data field starts with a header
	BOOL	bSRR /*= FALSE*/;	//This bit is set to 1 if a status report is requested
	BOOL	byteVPF /*=	2*/;	//Validity Period Format. Bit4 and Bit3 specify the TP-VP field
								/*0 0 : TP-VP field not present
								1 0 : TP-VP field present. Relative format (one octet)
								0 1 : TP-VP field present. Enhanced format (7 octets)
								1 1 : TP-VP field present. Absolute format (7 octets)*/
	BOOL	bRD /*= FALSE*/;	//Reject duplicates. Parameter indicating whether or not the SC shall accept an SMS-SUBMIT for an SM still held in the SC which has the same TP-MR and the same TP-DA as a previously submitted SM from the same OA. 
	BYTE	byteMTI /*= 1*/;	//Message type indicator. Bits no 1 and 0 are set to 0 and 1 respectively to indicate that this PDU is an SMS-SUBMIT 
};

struct TimeStamp	//These semi-octets are in "Swapped Nibble" mode on the Raw PDU(in the same order
{					//E.g.: 0x99 0x20 0x21 0x50 0x75 0x03 0x21 means 12. Feb 1999 05:57:30 GMT+3
	BYTE	byteYear;
	BYTE	byteMonth;
	BYTE	byteDay;
	BYTE	byteHour;
	BYTE	byteMinute;
	BYTE	byteSecond;
	BYTE	byteTimeZone; //Relation to GMT. One unit is 15min. If MSB=1, value is negative.
};

//Raw Pdu Submit struct (Outgoing SMS):
//(Byte)	Length of the SMSC information ( Optional = 0x00 )
//(Byte)	Type of address 7=1 654=Type of Number 3210=Numbering Plan Identification
//(Array)	Service Center Number (decimal semi-octets), with a trailing F to make even
//(Byte)	First octet of the SMS-SUBMIT message. ( = 0x11 )
//(Byte)	TP-Message-Reference. The "00" value here lets the phone set the message reference number itself. ( = 0x00 )
//(Byte)	Address-Length. Length of phone number 
//(Byte)	Type-of-Address. (91 indicates international format of the phone number).  
//(Array)	The phone number in semi octets (46708251358). The length of the phone number is odd (11), therefore a trailing F has been added, as if the phone number were "46708251358F". Using the unknown format (i.e. the Type-of-Address 81 instead of 91) would yield the phone number octet sequence 7080523185 (0708251358). Note that this has the length 10 (A), which is even.  
//(Byte)	TP-PID. Protocol identifier ( = 0x00 )
//(Byte)	TP-DCS. Data coding scheme.This message is coded according to the 7bit default alphabet. Having "04" instead of "00" here, would indicate that the TP-User-Data field of this message should be interpreted as 8bit rather than 7bit (used in e.g. smart messaging, OTA provisioning etc).  ( = 0x00 )
//(Byte)	TP-Validity-Period. "AA" means 4 days. Note: This octet is optional, see bits 4 and 3 of the first octet   ( = 0xAA )
//(Byte)	TP-User-Data-Length. Length of message. The TP-DCS field indicated 7-bit data, so the length here is the number of septets (10). If the TP-DCS field were set to 8-bit data or Unicode, the length would be the number of octets.  
//(Array)	TP-User-Data. These octets represent the message "hellohello". How to do the transformation from 7bit septets into octets is shown here  
class PduSubmit // Class for outgoing SMS
{
public:
// Methods
	PduSubmit(void);
	PduSubmit(	const Address& stServiceCenterAdress,
				const FirstOctetSubmit& stFirstOctet,
				const Address& stDestinationAddress,
				string& strUserData );
	PduSubmit(	const string& strDestinationNumber,
				const string& strMessage );
	int EncodeRawPdu();
	int GetRawPdu( string& strRawPdu );

protected:
// Members
	string				m_strRawPdu; // Raw PDU built by CreateRawPdu
	BOOL				m_bEncoded; // Flag indicating that the pdu is coded at m_strRawPdu

	Address				m_stServiceCenterAdress;
	FirstOctetSubmit	m_stFirstOctet; // Flags for the SMS to be considered by the SMSC
	Address				m_stDestinationAddress; // Destination number (cell phone)
	string				m_strUserData; // SmsText

};

//Raw Pdu Deliver struct (Incoming SMS):
//(Byte)	Length of the SMSC information
//(Byte)	Type of address 7=1 654=Type of Number 3210=Numbering Plan Identification
//(Array)	Service Center Number (decimal semi-octets), with a trailing F to make even
//(Byte)	First octet of the SMS-DELIVER message
//(Byte)	Address length of the sender number 
//(Byte)	Type-of-address of the sender number 
//(Array)	Sender number (decimal semi-octets), with a trailing F to make even
//(Byte)	TP-PID. Protocol identifier. (= 0x00)
//(Byte)	TP-DCS Data coding scheme (= 0x00)
//(Array)	TP-SCTS. Time stamp (semi-octets) 
//(Byte)	TP-UDL. User data length, length of message. The TP-DCS field indicated 7-bit data, so the length here is the number of septets (10). If the TP-DCS field were set to indicate 8-bit data or Unicode, the length would be the number of octets (9).  
//(Array)	TP-UD. Message "hellohello" , 8-bit octets representing 7-bit data.
class PduDeliver // Clas for incoming SMS
{
public:
// Methods
	PduDeliver(void);
	PduDeliver(	const string& strRawPdu );
	
	int DecodeRawPdu();
	int GetDecodedInfo( /*Address& stServiceCenterAdress,
						FirstOctetDeliver& stFirstOctet,
						Address& stSenderAddress,
						TimeStamp& m_stTimeStamp,*/
						string& strUserData );
	Address				m_stSenderAddress;

protected:
// Members
	string				m_strRawPdu; // Buffer for the Raw PDU.
	BOOL				m_bDecoded; // Flag indicating that fields below contain decoded info.

	Address				m_stServiceCenterAdress;
	FirstOctetDeliver	m_stFirstOctet;
	TimeStamp			m_stTimeStamp;
	string				m_strUserData;
};