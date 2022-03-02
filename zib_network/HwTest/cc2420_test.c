#include "ZigbeeTasks.h"
#include "zigbee.def"
#include "generic.h"
#include "zPHY.h"
//#include "zPHY_CC2420.h"
#include "zMAC.h"
//#include "zMAC_CC2420.h"
#include "MSPI.h"
#include "sralloc.h"
#include "cc2420_test.h"

#include "console.h"

#if(RF_CHIP == CC2420)

// ******************************************************************************
// Data Structures

/*typedef union _PHYPendingTasks
{
    struct
    {
        unsigned int PLME_SET_TRX_STATE_request_task :1;
        unsigned int PHY_RX :1;
    } bits;
    BYTE Val;
} PHY_PENDING_TASKS;

typedef struct _RX_DATA
{
    unsigned int size :7;
    unsigned int inUse :1;
} RX_DATA;*/


// ******************************************************************************
// Variable Definitions

/*extern CURRENT_PACKET       currentPacket;
static unsigned char        GIEH_backup;
extern MAC_TASKS_PENDING    macTasksPending;
volatile MAC_FRAME_CONTROL  pendingAckFrameControl;
PHY_PIB                     phyPIB;
volatile PHY_PENDING_TASKS  PHYTasksPending;
extern BYTE                 RxBuffer[RX_BUFFER_SIZE];
volatile RX_DATA            RxData;
#if (RX_BUFFER_SIZE > 256)
    extern WORD             RxRead;
    extern volatile WORD    RxWrite;
#else
    extern BYTE             RxRead;
    extern volatile BYTE    RxWrite;
#endif
SAVED_BITS                  savedBits;
BYTE                        TRXCurrentState;
PHY_ERROR                   phyError;*/


// ******************************************************************************
// Function Prototypes
// void UserInterruptHandler(void);


/*********************************************************************
 * Function:        BYTE PHYGet(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          Returns one buffered data byte that was received
 *                  by the transceiver.
 *
 * Side Effects:    None
 *
 * Overview:        This routine returns one byte that was previously
 *                  received and buffered.
 *
 * Note:            None
 ********************************************************************/
/*BYTE PHYGet(void)
{
    BYTE toReturn;

    toReturn = RxBuffer[RxRead++];

    if(RxRead == RX_BUFFER_SIZE)
    {
        RxRead = 0;
    }

    return toReturn;
}*/

/*********************************************************************
 * Function:        BYTE PHYHasBackgroundTasks(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          If the PHY layer has background tasks in progress.
 *
 * Side Effects:    None
 *
 * Overview:        This routine returns an indication of whether or
 *                  not the layer has background tasks in progress.
 *
 * Note:            None
 ********************************************************************/
/*BYTE PHYHasBackgroundTasks(void)
{
    //PHY might want to check to verify that we are not in the middle of
    // a transmission before allowing the user to turn off the TRX
    return PHYTasksPending.Val;
}*/


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
/*void PHYSetOutputPower( BYTE power)
{
    // Select desired TX output power level
    SPIPut(REG_TXCTRL);
    // As defined by Table 9 of CC2420 datasheet.
    SPIPut(0xA0);             // MSB
    SPIPut( power );          // LSB
}*/

/*********************************************************************
 * Function:        void PHYInit(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This routine initializes the PHY layer of the stack.
 *
 * Note:            None
 ********************************************************************/
void PHYInitLog(void)
{
    WORD i;
    CC2420_STATUS d;
    
    TRACE("Initializing the CC2420 using IO pins\r\n");

    PHY_RESETn = 0;

    CLRWDT();
    i = 0xE000;
    PHY_VREG_EN = 1;
    while(++i){}
    CLRWDT();

    PHY_RESETn = 1;
    i = 500;
    while(i--);
    CLRWDT();

    i=0;

    TRACE("Waiting for the oscilator to become stable\r\n");
    while(1)
    {
        PHY_CSn_0();
        SPIPut(STROBE_SXOSCON); // dds: turn on the ocilator
        PHY_CSn_1();

        d.Val = SSPBUF;
        
        if(d.bits.XOSC16M_STABLE == 1) // dds: wait until the oscilator is stable
        {
	        TRACE("Oscilator is stable...\r\nStatusRegister = ");
	        PrintChar(d.Val);
   	        TRACE("\r\ni = ");
   	        PrintChar(i>>8);
   	        PrintChar(i);
    	    TRACE("\r\n");
            break;
        }
        i++;
        
        if (i==0)
        {
        	TRACE("Still Waiting...\r\nStatusRegister = ");
        	PrintChar(CC2420GetStatus());
        	TRACE("\r\n");
        }
    }
    
    TRACE("The oscilator has become stable\r\nStatusRegister = 0x");
    PrintChar(CC2420GetStatus());
    TRACE("\r\n");

    PHY_CSn_0();

    /* set up REG_MDMCTRL0 */
    // Set AUTOACK bit in MDMCTRL0 register.
    // 15:14 = '00' = Reserved
    // 13    = '0'  = Reserved frames are rejected
    // 12    = '?'  = '1' if this is coordinator, '0' if otherwise
    // 11    = '1'  = Address decoding is enabled
    // 10:8  = '010'    = CCA Hysteresis in DB - default value
    // 7:6   = '11' = CCA = 1, when RSSI_VAL < CCA_THR - CCA_HYST and not receiving valid IEEE 802.15.4 data
    // 5     = '1'  = Auto CRC
    // 4     = '1'  = Auto ACK
    // 3:0   = '0010' = 3 leading zero bytes - IEEE compliant.


    SPIPut(REG_MDMCTRL0);
#if defined(I_AM_COORDINATOR)
    // MSB = 0b0001 1010
    SPIPut(0x1A);
    // LSB = 0b1111 0010
    SPIPut(0xF2);
#else
    // MSB = 0b0000 1010
    SPIPut(0x0A);
    // LSB = 0b1111 0010
    SPIPut(0xF2);
#endif

    SPIPut(REG_MDMCTRL1);
    SPIPut(0x05);            // MSB
    SPIPut(0x00);            // LSB

    /* disable the MAC level security */
    SPIPut(REG_SECCTRL0);
    SPIPut(0x01);            // MSB
    SPIPut(0xC4);            // LSB

    TRACE("Setting output power to 0dBm\r\n");
    PHYSetOutputPower( 0xFF );

    // Set FIFOP threshold to full RXFIFO buffer
    SPIPut(REG_IOCFG0);
    SPIPut(0x00);
    SPIPut(0x7F);
    PHY_CSn_1();

    // Flush the TX FIFO
    PHY_CSn_0();
    SPIPut(STROBE_SFLUSHTX);
    PHY_CSn_1();

#if !defined(I_AM_END_DEVICE)
    // set the data pending bit in the auto ack frames transmitted by the coordinator (todo: why?)
    PHY_CSn_0();
    SPIPut(STROBE_SACKPEND);
    PHY_CSn_1();
#endif

    // Flush the RX FIFO
    PHY_CSn_0();
    SPIPut(REG_RXFIFO | CMD_READ);
    SPIGet();
    PHY_CSn_1();
    PHY_CSn_0();
    SPIPut(STROBE_SFLUSHRX);
    SPIPut(STROBE_SFLUSHRX);
    PHY_CSn_1();
    
    d.Val = CC2420GetStatus();
    
    TRACE("End of initialization\r\n");
    TRACE("StatusRegister = 0x");
    PrintChar(d.Val);
    TRACE("\r\n");
    
    if (d.Val==0x40)
    	TRACE(">>> Initialization successfull <<<\r\n");
    else
    	TRACE(">>> Initialization failed <<<\r\n");
    
    return;
}

/*ZIGBEE_PRIMITIVE PHYTasks(ZIGBEE_PRIMITIVE inputPrimitive)
{
    if(inputPrimitive == NO_PRIMITIVE)
    {
        if(RxRead == RxWrite)
        {
            if(PHY_FIFO == 0)
            {
                if(PHY_FIFOP == 1)
                {
                    // Flush the RX FIFO
                    iPHY_CSn_0();
                    SPIPut(REG_RXFIFO | CMD_READ);
                    SPIGet();
                    iPHY_CSn_1();
                    iPHY_CSn_0();
                    SPIPut(STROBE_SFLUSHRX);
                    SPIPut(STROBE_SFLUSHRX);
                    iPHY_CSn_1();
                    RxData.inUse = 0;
                    ZigBeeStatus.flags.bits.bRxBufferOverflow = 0;
                }
            }
        }

        // manage background tasks here
        // phy background tasks
        if(ZigBeeTxUnblocked)   //(TxFIFOControl.bFIFOInUse==0)
        {
            if(PHYTasksPending.bits.PHY_RX) // signaled by the RX ISR
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

                // save the packet head somewhere so that it can be freed later
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

                    // reset the psdu to the head of the alloc-ed RAM, just happens to me CurrentRxPacket 
                    params.PD_DATA_indication.psdu = CurrentRxPacket;

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
            if(ZigBeeStatus.flags.bits.bRxBufferOverflow == 1)
            {
                ZigBeeStatus.flags.bits.bRxBufferOverflow = 0;
                CCP2IF = 1;
            }
        }
        else
        {
            // if tx is already busy sending a packet then we can't RX a packet because of resource control issues
            return NO_PRIMITIVE;
        }
    }
    else
    {
        // handle primitive here
        switch(inputPrimitive)
        {
//            case PD_DATA_request:
//                  This is handled by CC2420 hardware
//                break;
//            case PLME_CCA_request:
//                  This is handled by CC2420 hardware
//                break;
//            case PLME_ED_request:
//                    This is handled by CC2420 hardware, just read variable out of CC2420
//                break;
//            case PLME_SET_request:
//                  User will access variables directly
//                break;
//            case PLME_GET_request:
//                  User will access variables directly
//                break;
            case PLME_SET_TRX_STATE_request:
                if(params.PLME_SET_TRX_STATE_request.state == TRXCurrentState)
                {
                    params.PLME_SET_TRX_STATE_confirm.status = params.PLME_SET_TRX_STATE_request.state;
                    break;
                }

                if(params.PLME_SET_TRX_STATE_request.state == phyRX_ON || params.PLME_SET_TRX_STATE_request.state == phyTRX_OFF)
                {
                    BYTE_VAL status;
                    status.Val = CC2420GetStatus();

                    if(status.bits.b3 == 1)
                    {
                        params.PLME_SET_TRX_STATE_confirm.status = phyBUSY_TX;
                        break;
                    }
                }

                if((params.PLME_SET_TRX_STATE_request.state == phyTX_ON || params.PLME_SET_TRX_STATE_request.state == phyTRX_OFF) && (TRXCurrentState == phyRX_ON) )
                {
                    if(PHY_SFD == 1)
                    {
                        params.PLME_SET_TRX_STATE_confirm.status = phyBUSY_RX;
                        PHYTasksPending.bits.PLME_SET_TRX_STATE_request_task = 1;
                        return PLME_SET_TRX_STATE_confirm;
                    }
                }

                if(params.PLME_SET_TRX_STATE_request.state == phyFORCE_TRX_OFF || params.PLME_SET_TRX_STATE_request.state == phyTRX_OFF)
                {
                    PHY_CSn_0();
                    SPIPut(STROBE_SRFOFF);
                    PHY_CSn_1();
                    TRXCurrentState = phyTRX_OFF;
                    params.PLME_SET_TRX_STATE_confirm.status = phySUCCESS;
                    break;
                }
                else if(params.PLME_SET_TRX_STATE_request.state == phyRX_ON)
                {
                    PHY_CSn_0();
                    SPIPut(STROBE_SRXON);
                    PHY_CSn_1();
                }
//don't use this one because it is automatically handled in the CC2420 when we use the STXONCCA command strobe
//                else if(params.PLME_SET_TRX_STATE_request.state == phyTX_ON)
//                {
//                    PHY_CSn_0();
//                    SPIPut(STROBE_STXON);
//                    PHY_CSn_1();
//                }

                TRXCurrentState = params.PLME_SET_TRX_STATE_request.state;
                params.PLME_SET_TRX_STATE_confirm.status = phySUCCESS;
                break;
        }
    }
    return NO_PRIMITIVE;
}*/

/*BYTE CC2420GetStatus(void)
{
    BYTE GIEHsave;
    BYTE status;

    GIEHsave = GIEH;
    GIEH = 0;
    PHY_CSn_0();
    status = SPIGet();
    PHY_CSn_1();
    GIEH = GIEHsave;
    return status;
}*/

#else
    #error Please link the appropriate PHY file for the selected transceiver.
#endif

