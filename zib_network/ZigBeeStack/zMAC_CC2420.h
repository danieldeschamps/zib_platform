/*********************************************************************
 *
 *                  MAC Header File for the Chipcon 2420
 *
 *********************************************************************
 * FileName:        zMAC_CC2420.h
 * Dependencies:
 * Processor:       PIC18F
 * Complier:        MCC18 v3.00 or higher
 * Company:         Microchip Technology, Inc.
 *
 * Software License Agreement
 *
 * The software supplied herewith by Microchip Technology Incorporated
 * (the “Company”) for its PIC® microcontroller is intended and
 * supplied to you, the Company’s customer, for use solely and
 * exclusively on Microchip PIC microcontroller products.
 *
 * You may not modify or create derivatives works of the Software.
 *
 * If you intend to use the software in a product for sale, then you must
 * be a member of the ZigBee Alliance and obtain any licenses required by
 * them.  For more information, go to www.zigbee.org.
 *
 * The software is owned by the Company and/or its licensor, and is
 * protected under applicable copyright laws. All rights are reserved.
 *
 * Any use in violation of the foregoing restrictions may subject the
 * user to criminal sanctions under applicable laws, as well as to
 * civil liability for the breach of the terms and conditions of this
 * license.
 *
 * THIS SOFTWARE IS PROVIDED IN AN “AS IS” CONDITION. NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 * TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 *
 * Author               Date    Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * DF/KO                01/09/06 Microchip ZigBee Stack v1.0-3.5
 * DF/KO                08/31/06 Microchip ZigBee Stack v1.0-3.6
 ********************************************************************/

typedef struct _currentPacket
{
    BYTE sequenceNumber;
    TICK startTime;
    union _currentPacket_info
    {
        struct _currentPacket_info_bits
        {
            unsigned int retries :3;
            unsigned int association :1;   //this bit indicates if the packet was an association packet (results go to COMM_STATUS) or a normal packet (results go to DATA_confirm)
            unsigned int data_request :1;   //this bit indicates if the packet was an data request packet (results go to MLME_POLL_confirm)
            unsigned int expecting_data :1; //this bit indicates that a POLL_request got an ack back and the framePending bit was set
            unsigned int RX_association :1;
            unsigned int disassociation :1;  //this bit indicates that a MLME_DISASSOCIATION_confirm is required
        } bits;
        BYTE Val;
    } info;
    LONG_ADDR DstAddr;  //the destination of current packet if it was an association request or coordinator realignment
} CURRENT_PACKET;

typedef union _MAC_BACKGROUND_TASKS_PENDING
{
    BYTE Val;
    struct _background_bits
    {
        unsigned int indirectPackets :1;
        unsigned int packetPendingAck :1;
        unsigned int scanInProgress :1;
        unsigned int associationPending :1;
        unsigned int dataInBuffer :1;
        unsigned int bSendUpMACConfirm : 1;
        unsigned int channelScanning :1;
    } bits;
} MAC_TASKS_PENDING;

#define POSSIBLE_CHANNEL_MASK 0x07FFF800

void MACEnable(void);
void MACDisable(void);

