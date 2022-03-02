#ifndef _PDU_H_
#define _PDU_H_

int ascii_to_pdu(char *ascii, unsigned char **pdu, int & septetslen);
int pdu_to_ascii(unsigned char *pdu, int pdulength, char **ascii);
int decode_address_number( char* szAddress );

#endif
