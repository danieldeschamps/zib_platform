//////////////////////////////////////////////////////////////////////////
// Pdu.cpp
// Author:	DDS
// Purpose:	Implement pdu conversion object
// History:
// Date		Who	Comment
// 20050815	DDS	Created

//////////////////////////////////////////////////////////////////////////
// Includes
#include "StdAfx.h"
#include "Pdu.h"
#include "PduConversion.h"

//////////////////////////////////////////////////////////////////////////
// Definitions

//////////////////////////////////////////////////////////////////////////
// Core stuff

PduSubmit::PduSubmit(void)
: m_bEncoded(false)
{
}

PduSubmit::PduSubmit(	const Address& stServiceCenterAdress,
						const FirstOctetSubmit& stFirstOctet,
						const Address& stDestinationAddress,
						string& strUserData )
: m_bEncoded(false)
, m_strUserData( strUserData )
{
	m_stServiceCenterAdress = stServiceCenterAdress;
	m_stFirstOctet = stFirstOctet;
	m_stDestinationAddress = stDestinationAddress;
	m_strUserData = strUserData;
}

PduSubmit::PduSubmit(	const string& strDestinationNumber,
						const string& strMessage )
:	m_strUserData( strMessage )
,	m_bEncoded(false)

{
	// m_stServiceCenterAdress = NULL; // TODO - Inicializar com valores padrão (que funcionam!) e alterar a funcao de codificacao para ler estes valores
	// m_stFirstOctect = NULL;
	m_stDestinationAddress.strAddress = strDestinationNumber;
}

int PduSubmit::EncodeRawPdu()
{
	//D2: 49 17 26 26 38 97 -> Swap nibbles -> 947162628379 em international format
	//D2: 01 72 62 63 89 7F -> Swap nibbles -> 1027263698F7 em unknown format
	//0011000C919471626283790000AA0AE8329BFD4697D9EC37
	char szCmd[ MAX_PDU_LENGHT ];

//////////////////////////////////////////////////////////////////////////
//Prepare szRecipientNumber
	char szRecipientNumber[ MAX_ADDRESS_LENGHT ];
	int iNumLen = 0;
	char cBuffer;

	if( m_strUserData.length() > 160 )
	{
		TRACE(DBG_LVL_DEBUG, "PduSubmit::EncodeRawPdu - Error encoding pdu. Message is grater then 160 characters\n" );
		return 1;
	}

	strcpy( szRecipientNumber, m_stDestinationAddress.strAddress.c_str() );

	if( ( strlen( szRecipientNumber ) % 2 ) != 0 )
	{
		strcat( szRecipientNumber, "F" );
	}

	iNumLen = ( int ) strlen( szRecipientNumber );
	
	for( int i = 0 ; i < iNumLen / 2 ; i++  ) //Swap Nibbles
	{
		cBuffer = szRecipientNumber[2*i];
		szRecipientNumber[2*i] = szRecipientNumber[2*i + 1];
		szRecipientNumber[2*i + 1] = cBuffer;
	}
///////////////////////////////////////////////////////////////////////////	
//Prepare User Data ( szMsg ) and iSeptetsLen
	char szMsg[ MAX_UD_LENGHT + 1 ];
	int iPduLen = 0;
	int iSeptetsLen = 0;
	unsigned char *pdu;

	strncpy( szMsg, m_strUserData.c_str(), MAX_SMSMESSAGE_LENGHT )[MAX_SMSMESSAGE_LENGHT] = '\0';

	iPduLen = ascii_to_pdu( szMsg, &pdu, iSeptetsLen );
	//pdu[8] = 0x37;
	//strncpy( (char*)msg, (const char*)pdu, pdulen );
	for( i = 0 ; i < iPduLen ; i++ ) //Convert PDU byte information into String coded bytes
	{
		if( pdu[i] <= 0xF  )
		{
			szMsg[2*i] = '0';
			_itoa( pdu[i], & szMsg[2*i + 1], 16 ); //When o itoa results in 0x0E, the 0 is not written, only the E
		}
		else
		{
			_itoa( pdu[i], & szMsg[2*i], 16 );
		}
	}
//////////////////////////////////////////////////////////////////////////////
//Build Raw PDU
	char	szSMSCinformation[ 100 ]; //First Array of octets coded as chars
	char	szFirstOctet[ 3 ];
	char	szMessageReference[ 3 ];
	//char	szAddressLen[ 3 ];
	int		iRecipientNumberLen;
	char	szTypeOfAddress[ 3 ];
	//char	szNumber already declared
	char	szProtocolIdentifier[ 3 ];
	char	szDataCodingScheme[ 3 ];
	char	szValidityPeriod[ 3 ];

//First array
	/*if( strlen( m_stDestinationAddress.szAddress ) == 0 )
	{
		strcpy( szSMSCinformation, "00" );
	}
	else
	{
		// Lenght|TypeOfAddress|DecimalSemiOctets|
		stServiceCenterAdress.byteTypeOfNumber 
		stServiceCenterAdress.byteNumberingPlanId
		stServiceCenterAdress.szAddress
		strcpy( szSMSCinformation, "00" );
	}*/
	strcpy( szSMSCinformation, "00" );

//First octet of SMS submit
	/*Use stFirstOctet.bMMS stFirstOctet.bMTI stFirstOctet.bRP stFirstOctet.bSRI stFirstOctet.bUDHI
	to mount the e.g "11"*/
	strcpy( szFirstOctet, "11" );

//TP-Message reference = "00"
	strcpy( szMessageReference, "00" );

//Phone number
	/*Use stSenderAddress.byteTypeOfNumber and	stSenderAddress.byteNumberingPlanId to mount e.g "91"
	strcat with szNumber which is already built*/
	iRecipientNumberLen = ( int ) strlen( szRecipientNumber );
	strcpy( szTypeOfAddress, "91" );

//Protocol identifier = "00"
	strcpy( szProtocolIdentifier, "00" );

//Data coding scheme = "00"
	strcpy( szDataCodingScheme, "00" );

//Validity Period = "AA"
	strcpy( szValidityPeriod, "AA" );


//SMS Message Len + body
	//Format this: iSeptetsLen already built, szMsg already built

	//szCmd.Format( "%s%02X%s%s%s%02X%s%s", "001100", iNumLen, "91", szNumber, "0000AA", iSeptetsLen, szMsg, "\x1A"  ); //alguns caracteres em msg podem representar fim de string, entao nao pode usar string functions para isso
	sprintf( szCmd, "%s%s%s%02X%s%s%s%s%s%02X%s", szSMSCinformation, szFirstOctet, szMessageReference, iRecipientNumberLen, szTypeOfAddress, szRecipientNumber, szProtocolIdentifier, szDataCodingScheme, szValidityPeriod, iSeptetsLen, szMsg );

	_strupr( szCmd );
	free(pdu);

	m_strRawPdu = szCmd;
	m_bEncoded = TRUE;

	return 0;
}

int PduSubmit::GetRawPdu( string& strRawPdu )
{
	strRawPdu = m_strRawPdu;
	return 0;
}
//////////////////////////////////////////////////////////////////////////
PduDeliver::PduDeliver( void )
: m_bDecoded(false)
{
}

PduDeliver::PduDeliver(	const string& strRawPdu )
: m_strRawPdu( strRawPdu )
, m_bDecoded(false)
{
}
	
int PduDeliver::DecodeRawPdu()
{
	if( !m_strRawPdu.empty() )
	{
		char szCharPdu[MAX_PDU_LENGHT*2]; // Each semi-octect corresponds to 1 byte
		unsigned char szHexPdu[MAX_PDU_LENGHT];

		// |07| |91551006000006| - SMSC - 7 octects
		// |04| - First octect
		// |0B| |A1|(Type of addres) |4081045601F2|(84065102) - Sender number -> 11 semi-octects
		// |00| - protocol id
		// |00| - data coding scheme
		// |50809141905229| - time stamp
		// |0A| |E8329BFD4697D9EC37| - message "hello hello" with size 10 (0xA)

	// Convert 'ASC Char PDU' to 'Hexadecimal PDU'
		strcpy( szCharPdu, m_strRawPdu.c_str() );

		int i = 0;
		for( ; i < m_strRawPdu.length() ; i++ ) // Convert String coded bytes into PDU byte information
		{
			char szCharOctect[3]; // This is a character representation of one octect
			strncpy( szCharOctect, szCharPdu+(i*2), 2 )[2] = '\0';
			szHexPdu[i] = (unsigned char)strtoul( szCharOctect, 0, 16 );
		}

	// Parse the Hexadecimal PDU
		unsigned char* pHexParser = szHexPdu;
		char* pCharParser = szCharPdu;

		m_stServiceCenterAdress.byteTypeOfNumber = (*(pHexParser+1) & NTYPE_MASK) >> NTYPE_MASK_SHIFT;
		m_stServiceCenterAdress.byteNumberingPlanId = (*(pHexParser+1) & NPLAN_MASK) >> NPLAN_MASK_SHIFT;
		
		char szServiceCenterNumber[MAX_NUMBER_LENGHT];
		strncpy( szServiceCenterNumber, pCharParser+4, ((*pHexParser)-1)*2 )[((*pHexParser)-1)*2] = '\0';
		decode_address_number( szServiceCenterNumber );
		m_stServiceCenterAdress.strAddress = szServiceCenterNumber;

		int iOffset = (*pHexParser) + 1;
		pHexParser += iOffset; // Now I'm pointing to First octect of the PDU-DELIVER
		pCharParser += iOffset*2;

		m_stFirstOctet.bMTI = (*pHexParser & FOD_MTI_MASK) >> FOD_MTI_SHIFT;
		m_stFirstOctet.bMMS = (*pHexParser & FOD_MMS_MASK) >> FOD_MMS_SHIFT;
		m_stFirstOctet.bSRI = (*pHexParser & FOD_SRI_MASK) >> FOD_SRI_SHIFT;
		m_stFirstOctet.bUDHI = (*pHexParser & FOD_UDHI_MASK) >> FOD_UDHI_SHIFT;
		m_stFirstOctet.bRP = (*pHexParser & FOD_RP_MASK) >> FOD_RP_SHIFT;

		pHexParser++; // Now I'm pointing to Address-Length of the sender number
		pCharParser += 2;

		m_stSenderAddress.byteTypeOfNumber = (*(pHexParser+1) & NTYPE_MASK) >> NTYPE_MASK_SHIFT;
		m_stSenderAddress.byteNumberingPlanId = (*(pHexParser+1) & NPLAN_MASK) >> NPLAN_MASK_SHIFT;

		char szSenderNumber[MAX_NUMBER_LENGHT];
		iOffset = (*pHexParser)%2 ? (*pHexParser)+1 : (*pHexParser); // Even or odd? In case of Odd, we must add one because of the F coming in the last octect
		strncpy(szSenderNumber, pCharParser+4, iOffset)[iOffset] = '\0';
		decode_address_number( szSenderNumber );
		m_stSenderAddress.strAddress = szSenderNumber;

		iOffset = 1 + iOffset/2+1 + 2;
		pHexParser += iOffset; // Let's jump PID and DCS, now I'm pointing to SCTS
		pCharParser += iOffset * 2;

		char szSCTS[15];
		strncpy( szSCTS, pCharParser, 14 )[14] = '\0';
		decode_address_number( szSCTS );
		/*m_stTimeStamp.byteYear = atoi(szSCTS[0]);
		m_stTimeStamp.byteMonth = atoi(szSCTS[0]);
		m_stTimeStamp.byteDay = atoi(szSCTS[0]);
		m_stTimeStamp.byteHour = atoi(szSCTS[0]);
		m_stTimeStamp.byteMinute = atoi(szSCTS[0]);
		m_stTimeStamp.byteSecond= atoi(szSCTS[0]);
		m_stTimeStamp.byteTimeZone = atoi(szSCTS[0]);*/

		pHexParser += 7; // Now I'm pointing to the UD length in septets
		pCharParser += 14;

		char szMessageBody[MAX_PDU_LENGHT];
		strcpy( szMessageBody, pCharParser+2 ); // copy until the end of the pdu deliver
		char* sz8bitConversion = 0;
		pdu_to_ascii(	pHexParser+1,
						(*pHexParser),
						&sz8bitConversion );
		sz8bitConversion[*pHexParser] = '\0';
		m_strUserData = sz8bitConversion;
		free( sz8bitConversion );

		m_bDecoded = TRUE;
	}

	return 0;

}

int PduDeliver::GetDecodedInfo( /*Address& stServiceCenterAdress,
								FirstOctetDeliver& stFirstOctet,
								Address& stSenderAddress,
								TimeStamp& m_stTimeStamp,*/
								string& strUserData )
{
	strUserData = m_strUserData;
	return 0;
}
