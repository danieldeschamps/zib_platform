/*********************************************************************
 *
 *                  PHY for the MRF24J40
 *
 *********************************************************************
 * FileName:        zPHY_MRF24J40.c
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
 * DF/KO                08/31/06 Microchip ZigBee Stack v1.0-3.6
 ********************************************************************/

#include "ZigbeeTasks.h"
#include "Zigbee.def"
#include "zigbee.h"
#include "zPHY.h"
#include "zMAC.h"
#include "MSPI.h"
#include "generic.h"
#include "sralloc.h"
//#include "console.h"

#if (RF_CHIP == MRF24J40) || (RF_CHIP == UZ2400)


// ******************************************************************************
// Data Structures

typedef struct _RX_DATA
{
    unsigned int size :7;
    unsigned int inUse :1;
} RX_DATA;


// ******************************************************************************
// Variable Definitions

extern CURRENT_PACKET       currentPacket;
static unsigned char        GIEH_backup;
volatile INT_SAVE           IntStatus;
extern MAC_TASKS_PENDING    macTasksPending;
volatile MAC_FRAME_CONTROL  pendingAckFrameControl;
PHY_ERROR                   phyError;
PHY_PIB                     phyPIB;
volatile PHY_PENDING_TASKS  PHYTasksPending;
extern BYTE                 RxBuffer[RX_BUFFER_SIZE];
volatile RX_DATA            RxData;
BYTE                        TRXCurrentState;
volatile TX_STAT            TxStat;
SAVED_BITS                  savedBits;

#if (RX_BUFFER_SIZE > 256)
    extern volatile WORD    RxWrite;
    extern WORD             RxRead;
#else
    extern volatile BYTE    RxWrite;
    extern BYTE             RxRead;
#endif


// ******************************************************************************
// Function Prototypes

void UserInterruptHandler(void);

/*********************************************************************
 * Function:        BYTE PHYGet(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          The next byte from the transceiver read buffer
 *
 * Side Effects:    Read buffer pointer is incremented
 *
 * Overview:        This function returns the next byte from the
 *                  transceiver read buffer.
 *
 * Note:            None
 ********************************************************************/

BYTE PHYGet(void)
{
    BYTE toReturn;

    toReturn = RxBuffer[RxRead++];

    if(RxRead == RX_BUFFER_SIZE)
    {
        RxRead = 0;
    }

    return toReturn;
}

/*********************************************************************
 * Function:        BOOL PHYHasBackgroundTasks( void )
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          TRUE - PHY layer has background tasks to run
 *                  FALSE - PHY layer does not have background tasks
 *
 * Side Effects:    None
 *
 * Overview:        Determines if the PHY layer has background tasks
 *                  that need to be run.
 *
 * Note:            None
 ********************************************************************/

BYTE PHYHasBackgroundTasks(void)
{
    //PHY might want to check to verify that we are not in the middle of
    // a transmission before allowing the user to turn off the TRX
    return PHYTasksPending.Val;
}

/*********************************************************************
 * Function:        void PHYInit( void )
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    Timer 1 is turned on; the interrupt is disabled.
 *
 * Overview:        This routine initializes all PHY layer data
 *                  constants.  It also initializes the transceiver.
 *
 * Note:            None
 ********************************************************************/

void PHYInit(void)
{
        BYTE i;
        PHY_RESETn_TRIS = 0;

        RxData.inUse = 0;
        PHYTasksPending.Val = 0;

        CLRWDT();

        PHY_RESETn = 0;
        for(i=0;i<255;i++){}
        PHY_RESETn = 1;
        for(i=0;i<255;i++){}

        /* flush the RX fifo */
        PHYSetShortRAMAddr(RXFLUSH,0x01);

        /* Program the short MAC Address, 0xffff */
        //<TODO> this step may not be required
        PHYSetShortRAMAddr(SADRL,0xFF);
        PHYSetShortRAMAddr(SADRH,0xFF);
        PHYSetShortRAMAddr(PANIDL,0xFF);
        PHYSetShortRAMAddr(PANIDH,0xFF);

        /* Program Long MAC Address, 0xFFFFFFFFFFFFFFFF*/
        PHYSetShortRAMAddr(EADR0, MAC_LONG_ADDR_BYTE0);
        PHYSetShortRAMAddr(EADR1, MAC_LONG_ADDR_BYTE1);
        PHYSetShortRAMAddr(EADR2, MAC_LONG_ADDR_BYTE2);
        PHYSetShortRAMAddr(EADR3, MAC_LONG_ADDR_BYTE3);
        PHYSetShortRAMAddr(EADR4, MAC_LONG_ADDR_BYTE4);
        PHYSetShortRAMAddr(EADR5, MAC_LONG_ADDR_BYTE5);
        PHYSetShortRAMAddr(EADR6, MAC_LONG_ADDR_BYTE6);
        PHYSetShortRAMAddr(EADR7, MAC_LONG_ADDR_BYTE7);

        /* program the RF and Baseband Register */
        PHYSetLongRAMAddr(RFCTRL4,0x02);

        /* Enable the RX */
        PHYSetLongRAMAddr(RFRXCTRL,0x01);

        /* setup */
        //PHYSetLongRAMAddr(RFCTRL1,0x00);
        PHYSetLongRAMAddr(RFCTRL2,0x80);

        PHYSetOutputPower( PA_LEVEL );

        /* program RSSI ADC with 2.5 MHz clock */
        PHYSetLongRAMAddr(RFCTRL6,0x04);
        PHYSetLongRAMAddr(RFCTRL7,0b00000000);

        /* Program CCA mode using RSSI */
        PHYSetShortRAMAddr(BBREG2,0x80);
        /* Enable the packet RSSI */
        PHYSetShortRAMAddr(BBREG6,0x40);
        /* Program CCA, RSSI threshold values */
        PHYSetShortRAMAddr(RSSITHCCA,0x60);

        #ifdef I_AM_FFD
            PHYSetShortRAMAddr(ACKTMOUT,0xB9);
        #endif

        // Set interrupt mask
        PHYSetShortRAMAddr(0x32, 0xF6);

        do
        {
            i = PHYGetLongRAMAddr(RFSTATE);
        }
        while((i&0xA0) != 0xA0);
}

/*********************************************************************
 * Function:        ZIGBEE_PRIMITIVE PHYTasks(ZIGBEE_PRIMITIVE inputPrimitive)
 *
 * PreCondition:    None
 *
 * Input:           inputPrimitive - the next primitive to run
 *
 * Output:          The next primitive to run.
 *
 * Side Effects:    Numerous
 *
 * Overview:        This routine executes the indicated ZigBee primitive.
 *                  If no primitive is specified, then background
 *                  tasks are executed.
 *
 * Note:            If a message is waiting in the receive buffer, it
 *                  can only be sent to the upper layers if the input
 *                  primitive is NO_PRIMITIVE and the Tx path is not
 *                  blocked.
 ********************************************************************/

ZIGBEE_PRIMITIVE PHYTasks(ZIGBEE_PRIMITIVE inputPrimitive)
{
    if(inputPrimitive == NO_PRIMITIVE)
    {
        /* manage background tasks here */
//        if(ZigBeeStatus.flags.bits.bRxBufferOverflow == 1)
//        {
//            ConsolePutROMString((rom char*)"RxBufferOverflow!\r\n");
//        }
        if(ZigBeeTxUnblocked)   //(TxFIFOControl.bFIFOInUse==0)
        {

            if(PHYTasksPending.bits.PHY_RX)
            {
                BYTE packetSize;

                if(CurrentRxPacket != NULL)
                {

                    return NO_PRIMITIVE;
                }

                packetSize = RxBuffer[RxRead];
                params.PD_DATA_indication.psdu = SRAMalloc(packetSize);

                if(params.PD_DATA_indication.psdu == NULL)
                {
                    phyError.failedToMallocRx = 1;
                    PHYTasksPending.bits.PHY_RX = 0;
                    return NO_PRIMITIVE;
                }

                /* save the packet head somewhere so that it can be freed later */
                if(CurrentRxPacket == NULL)
                {
                    CurrentRxPacket = params.PD_DATA_indication.psdu;

                    params.PD_DATA_indication.psduLength = packetSize;
                    RxRead++;
                    if(RxRead == RX_BUFFER_SIZE)
                    {
                        RxRead = 0;
                    }

                    while(packetSize--)
                    {
                        *params.PD_DATA_indication.psdu++ = PHYGet();
                    }

                    /* reset the psdu to the head of the alloc-ed RAM, just happens to me CurrentRxPacket */
                    params.PD_DATA_indication.psdu = CurrentRxPacket;

                    /* disable interrupts before checking to see if this was the
                        last packet in the FIFO so that if we get a packet(interrupt) after the check,
                        but before the clearing of the bit then the new indication will not
                        get cleared */

                    savedBits.bGIEH = GIEH;
                    GIEH = 0;

                    if(RxRead == RxWrite)
                    {
                        PHYTasksPending.bits.PHY_RX = 0;
                    }

                    GIEH = savedBits.bGIEH;

                    return PD_DATA_indication;
                }
            }
        }
        else
        {
            return NO_PRIMITIVE;
        }

    }
    else
    {
        /* handle primitive here */
        switch(inputPrimitive)
        {
// Not necessary for operation
//            case PD_DATA_request:
//                break;
//CSMA-CA will be automatic
//           case PLME_CCA_request:
//                break;
//            case PLME_ED_request:
  //              break;
/*          User will modify these directly
            case PLME_SET_request:
                break;
            case PLME_GET_request:
                break;  */
/*          The MRF24J40 part switches states automatically
            case PLME_SET_TRX_STATE_request:
                    */
        }
    }
}

/*********************************************************************
 * Function:        void PHYSetLongRAMAddr(WORD address, BYTE value)
 ********************************************************************/

void PHYSetLongRAMAddr(WORD address, BYTE value)
{
    IntStatus.CCP2IntF = INT0IE;
    INT0IE = 0;
    PHY_CSn = 0;
    NOP();
    SPIPut((((BYTE)(address>>3))&0b01111111)|0x80);
    SPIPut((((BYTE)(address<<5))&0b11100000)|0x10);
    SPIPut(value);
    NOP();
    PHY_CSn = 1;
    INT0IE = IntStatus.CCP2IntF;
}

/*********************************************************************
 * Function:        void PHYSetShortRAMAddr(WORD address, BYTE value)
 ********************************************************************/

void PHYSetShortRAMAddr(BYTE address, BYTE value)
{
    IntStatus.CCP2IntF = INT0IE;
    INT0IE = 0;
    PHY_CSn = 0;
    NOP();
    SPIPut(((address<<1)&0b01111111)|0x01);
    SPIPut(value);
    NOP();
    PHY_CSn = 1;
    INT0IE = IntStatus.CCP2IntF;
}

/*********************************************************************
 * Function:        void PHYGetShortRAMAddr(WORD address, BYTE value)
 ********************************************************************/

BYTE PHYGetShortRAMAddr(BYTE address)
{
    BYTE toReturn;
    IntStatus.CCP2IntF = INT0IE;
    INT0IE = 0;
    PHY_CSn = 0;
    NOP();
    SPIPut((address<<1)&0b01111110);
    toReturn = SPIGet();
    NOP();
    PHY_CSn = 1;
    INT0IE = IntStatus.CCP2IntF;
    return toReturn;
}

/*********************************************************************
 * Function:        void PHYGetLongRAMAddr(WORD address, BYTE value)
 ********************************************************************/

BYTE PHYGetLongRAMAddr(WORD address)
{
    BYTE toReturn;
    IntStatus.CCP2IntF = INT0IE;
    INT0IE = 0;
    PHY_CSn = 0;
    NOP();
    SPIPut(((address>>3)&0b01111111)|0x80);
    SPIPut(((address<<5)&0b11100000));
    toReturn = SPIGet();
    NOP();
    PHY_CSn = 1;
    INT0IE = IntStatus.CCP2IntF;
    return toReturn;
}

/*********************************************************************
 * Function:        void PHYSetOutputPower( BYTE power)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This routine sets the output power of the transceiver.
 *                  Refer to the transceiver datasheet for the value
 *                  to pass to this function for the desired power level.
 *
 * Note:            None
 ********************************************************************/
void PHYSetOutputPower( BYTE power)
{
/*   PA_LEVEL determiens output power of transciever
        Default output power is 0 dBm. Summation of “large” and “small” tuning decreases
        output power
    PA_LEVEL:
        [7:6] -> large scale tuning
              00: 0 dB
              01: -10 dB
              10: -20 dB
              11: -30 dB
        [5:3] -> small scale tuning
              000: 0 dB
              001: -1.25 dB
              010: -2.5 dB
              011: -3.75 dB
              100: -5 dB
              101: -6.25 dB
              110: -7.5 dB
              111: -8.75 dB
        [2:0] -> 000
*/
        PHYSetLongRAMAddr( RFCTRL3, power );
}

//******************************************************************************
// ISR
//******************************************************************************

#pragma interruptlow HighISR
void HighISR (void)
{
    BYTE    CheckInterrupt;
    BYTE    TxStatus;

    // Check if our interrupt came from the INT pin on the MRF24J40
    if(INT0IF)
    {
        if(INT0IE)
        {
            // Clear interrupt
            INT0IF = 0;

            // Reading this interrupt register will clear the interrupts
            CheckInterrupt = PHYGetShortRAMAddr(0x31);

            if (CheckInterrupt & 0x01)
            {
                TxStat.finished = 1;
                //Transmit FIFO release interrupt
                // The release status will be set to ok if an ACK was required and
                // was received successfully

                TxStatus = PHYGetShortRAMAddr(0x24);
                if (TxStatus & 0x01)
                {
                    //Failure- No ack back
                    TxStat.success = 0;
                }
                else
                {
                    //success
                    TxStat.success = 1;
                }
            }

            if (CheckInterrupt & 0x08)
            {
                BYTE_VAL ack_status;
                BYTE count;
                BYTE counter;
                BYTE_VAL w;

                #if (RX_BUFFER_SIZE > 256)
                        #error "Rx buffer must be <= 256"
                #else
                        #define BUFFER_CAST BYTE
                        BYTE RxBytesRemaining;
                        BYTE OldRxWrite;
                #endif

                ack_status.Val = 0;
                count = 0;
                counter = 0x00;

                // Receive ok interrupt
                if(RxData.inUse == 0)
                {
                    RxData.size = PHYGetLongRAMAddr ((WORD)(0x300 + counter++));
                    RxData.inUse=1;
                }

                OldRxWrite = RxWrite;
                if(RxWrite < RxRead)
                {
                    RxBytesRemaining = (BUFFER_CAST)(RxRead - RxWrite - 1);
                }
                else
                {
                    RxBytesRemaining = (BUFFER_CAST)(RX_BUFFER_SIZE - 1 - RxWrite + RxRead);
                }

                w.Val = RxData.size;


                /* this is less then because we need one extra byte for the length (which worst case would make it equivent to less then or equal to )*/
                if(w.Val < RxBytesRemaining)
                {
                    MAC_FRAME_CONTROL mfc;

                    /* there is room in the buffer */
                    RxData.inUse = 0;

                    /* add the packet */
                    RxBuffer[RxWrite++]=w.Val;

                    if(RxWrite==RX_BUFFER_SIZE)
                    {
                        RxWrite = 0;
                    }

                    while(w.Val--)
                    {
                        //Note: I am counting on the fact that RxWrite doesn't get incremented here anymore such that the ACK packet doesn't get written into the Buffer and the RxWrite doesn't get modified.
                        if (w.Val == 1)
                        {
                            // The last two bytes will be the 16-bit CRC.  Instead, we will read out the RSSI value
                            // for the link quality and stick it in the second to last byte.  We still need to read the
                            // last byte of the CRC to trigger the device to flush the packet.
                            RxBuffer[RxWrite] = PHYGetLongRAMAddr ((WORD)(0x300 + counter + 2));
                            counter++;
                        }
                        else
                        {
                            RxBuffer[RxWrite] = PHYGetLongRAMAddr ((WORD)(0x300 + counter++));
                        }

                        if(count==0)
                        {
                            //if the frame control indicates that this packet is an ack

                            mfc.word.byte.LSB=RxBuffer[RxWrite];

                            if(mfc.bits.FrameType == 0b010)
                            {
                                //it was an ack then set the ack_status.bits.b0 to 1 showing it was an ack
                                ack_status.bits.b0 = 1;
                                //ConsolePut('@');
                            }
                        }
                        else if(count==2)
                        {
                            //if we are reading the sequence number and the packet was an ack
                            if(ack_status.bits.b0)
                            {

                                if ((macTasksPending.bits.packetPendingAck) &&
                                    (RxBuffer[RxWrite] == currentPacket.sequenceNumber))
                                {
                                    // If this is the ACK we've been waiting for, set the flag to
                                    // send up the confirm and save the Frame Control.
                                    macTasksPending.bits.bSendUpMACConfirm = 1;
                                    pendingAckFrameControl = mfc;
                                }
                                RxWrite = OldRxWrite;
                                goto DoneReceivingPacket;
                            }
                        }

                        count++;
                        RxWrite++;
                        //roll buffer if required
                        if(RxWrite==RX_BUFFER_SIZE)
                        {
                            RxWrite = 0;
                        }
                    }

                    if(RxWrite==0)
                    {
                        w.Val = RxBuffer[RX_BUFFER_SIZE-1];
                    }
                    else
                    {
                        w.Val = RxBuffer[RxWrite - 1];
                    }

                    if(PHYGetShortRAMAddr(RXSR) & 0x08)
                    {
                        /* crc failed.  Erase packet from the array */
                        RxWrite = OldRxWrite;
                        // Flush the RX FIFO
                        PHYSetShortRAMAddr (RXFLUSH, 0x01);
                    }
                    else
                    {
                        ZigBeeStatus.flags.bits.bHasBackgroundTasks = 1;
                        PHYTasksPending.bits.PHY_RX = 1;
                    }
                }
                else
                {
                    RxWrite = OldRxWrite;
                    ZigBeeStatus.flags.bits.bRxBufferOverflow = 1;
                }

DoneReceivingPacket:
                Nop();
            }

        }

    }
    if(TMR0IF)
    {
        if(TMR0IE)
        {
            /* there was a timer overflow */
            TMR0IF = 0;
            timerExtension1++;

            if(timerExtension1 == 0)
            {
                timerExtension2++;
            }
        }
    }

    UserInterruptHandler();

}

/************************************/
/*        Interrupt Vectors         */
/************************************/

#if defined(MCHP_C18)
#pragma code highVector=0x08
void HighVector (void)
{
    _asm goto HighISR _endasm
}
#pragma code /* return to default code section */
#endif

#if defined(MCHP_C18)
#pragma code lowhVector=0x18
void LowVector (void)
{
    _asm goto HighISR _endasm
}
#pragma code /* return to default code section */
#endif


#else
    #error Please link the appropriate PHY file for the selected transceiver.
#endif      // RF_CHIP == MRF24J40
