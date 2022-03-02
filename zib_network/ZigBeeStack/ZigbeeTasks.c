/*********************************************************************
 *
 *                  ZigBee Tasks File
 *
 *********************************************************************
 * FileName:        ZigBeeTasks.c
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

#include "Compiler.h"
#include "generic.h"
#include "zigbee.def"
#include "ZigbeeTasks.h"
#include "zPHY.h"
#include "zMAC.h"
#include "zNWK.h"
#include "zAPS.h"
#include "zZDO.h"
#include "MSPI.h"
#include "sralloc.h"
#include "SymbolTime.h"

//#include "console.h"

//#pragma udata TX_BUFFER=TX_BUFFER_LOCATION
BYTE TxBuffer[TX_BUFFER_SIZE];
//#pragma udata

#pragma udata RX_BUFFER=RX_BUFFER_LOCATION
volatile BYTE RxBuffer[RX_BUFFER_SIZE];
#pragma udata

BYTE                    * CurrentRxPacket;
PARAMS                  params;
BYTE                    RxRead;
volatile BYTE           RxWrite;
BYTE                    TxData;
BYTE                    TxHeader;
volatile ZIGBEE_STATUS  ZigBeeStatus;

void ZigBeeInit(void)
{
    SRAMInitHeap();

    MACInit();
    NWKInit();
    APSInit();
    ZDOInit();

    TxHeader = 127;
    TxData = 0;
    RxWrite = 0;
    RxRead = 0;

    // Set up the interrupt to read in a data packet.
    // set to capture on falling edge
#if (RF_CHIP == UZ2400) || (RF_CHIP == MRF24J40)
    CCP2CON = 0b00000100;
#elif (RF_CHIP == CC2420)
    CCP2CON = 0b00000101;
#endif

    // Set up the interrupt to read in a data packet.
#if (RF_CHIP==UZ2400) || (RF_CHIP == MRF24J40)
    INT0IF = 0;
    INT0IE = 1;
#elif (RF_CHIP==CC2420)
    CCP2IF = 0;
    CCP2IP = 1;
    CCP2IE = 1;
#endif
    InitSymbolTimer();

    ZigBeeStatus.nextZigBeeState = NO_PRIMITIVE;
    CurrentRxPacket = NULL;
}
#pragma

BOOL ZigBeeTasks( ZIGBEE_PRIMITIVE *command )
{
    ZigBeeStatus.nextZigBeeState = *command;

    do        /* need to determine/modify the exit conditions */
    {
        CLRWDT();

        if(ZigBeeStatus.nextZigBeeState == NO_PRIMITIVE)
        {
            ZigBeeStatus.nextZigBeeState = PHYTasks(ZigBeeStatus.nextZigBeeState);
        }
        if(ZigBeeStatus.nextZigBeeState == NO_PRIMITIVE)
        {
            ZigBeeStatus.nextZigBeeState = MACTasks(ZigBeeStatus.nextZigBeeState);
        }
        if(ZigBeeStatus.nextZigBeeState == NO_PRIMITIVE)
        {
            ZigBeeStatus.nextZigBeeState = NWKTasks(ZigBeeStatus.nextZigBeeState);
        }
        if(ZigBeeStatus.nextZigBeeState == NO_PRIMITIVE)
        {
            ZigBeeStatus.nextZigBeeState = APSTasks(ZigBeeStatus.nextZigBeeState);
        }
        if(ZigBeeStatus.nextZigBeeState == NO_PRIMITIVE)
        {
            ZigBeeStatus.nextZigBeeState = ZDOTasks(ZigBeeStatus.nextZigBeeState);
        }

        switch(ZigBeeStatus.nextZigBeeState)
        {
            // Check for the primitives that are handled by the PHY.
            case PD_DATA_request:
            case PLME_CCA_request:
            case PLME_ED_request:
            case PLME_SET_request:
            case PLME_GET_request:
            case PLME_SET_TRX_STATE_request:
                ZigBeeStatus.nextZigBeeState = PHYTasks(ZigBeeStatus.nextZigBeeState);
                break;

            // Check for the primitives that are handled by the MAC.
            case PD_DATA_indication:
            case PD_DATA_confirm:
            case PLME_ED_confirm:
            case PLME_GET_confirm:
            case PLME_CCA_confirm:
            case PLME_SET_TRX_STATE_confirm:
            case PLME_SET_confirm:
            case MCPS_DATA_request:
            case MCPS_PURGE_request:
            case MLME_ASSOCIATE_request:
            case MLME_ASSOCIATE_response:
            case MLME_DISASSOCIATE_request:
            case MLME_GET_request:
            case MLME_GTS_request:
            case MLME_ORPHAN_response:
            case MLME_RESET_request:
            case MLME_RX_ENABLE_request:
            case MLME_SCAN_request:
            case MLME_SET_request:
            case MLME_START_request:
            case MLME_SYNC_request:
            case MLME_POLL_request:
                ZigBeeStatus.nextZigBeeState = MACTasks(ZigBeeStatus.nextZigBeeState);
                break;

            // Check for the primitives that are handled by the NWK.
            case MCPS_DATA_confirm:
            case MCPS_DATA_indication:
            case MCPS_PURGE_confirm:
            case MLME_ASSOCIATE_indication:
            case MLME_ASSOCIATE_confirm:
            case MLME_DISASSOCIATE_indication:
            case MLME_DISASSOCIATE_confirm:
            case MLME_BEACON_NOTIFY_indication:
            case MLME_GET_confirm:
            case MLME_GTS_confirm:
            case MLME_GTS_indication:
            case MLME_ORPHAN_indication:
            case MLME_RESET_confirm:
            case MLME_RX_ENABLE_confirm:
            case MLME_SCAN_confirm:
            case MLME_COMM_STATUS_indication:
            case MLME_SET_confirm:
            case MLME_START_confirm:
            case MLME_SYNC_LOSS_indication:
            case MLME_POLL_confirm:
            case NLDE_DATA_request:
            case NLME_NETWORK_DISCOVERY_request:
            case NLME_NETWORK_FORMATION_request:
            case NLME_PERMIT_JOINING_request:
            case NLME_START_ROUTER_request:
            case NLME_JOIN_request:
            case NLME_DIRECT_JOIN_request:
            case NLME_LEAVE_request:
            case NLME_RESET_request:
            case NLME_SYNC_request:
            case NLME_GET_request:
            case NLME_SET_request:
                ZigBeeStatus.nextZigBeeState = NWKTasks( ZigBeeStatus.nextZigBeeState );
                break;

            // Check for the primitives that are handled by the APS.
            case NLDE_DATA_confirm:
            case NLDE_DATA_indication:
            case APSDE_DATA_request:
            case APSME_BIND_request:
            case APSME_UNBIND_request:
                ZigBeeStatus.nextZigBeeState = APSTasks( ZigBeeStatus.nextZigBeeState );
                break;

            case ZDO_DATA_indication:
            case ZDO_BIND_req:
            case ZDO_UNBIND_req:
            case ZDO_END_DEVICE_BIND_req:
                ZigBeeStatus.nextZigBeeState = ZDOTasks( ZigBeeStatus.nextZigBeeState );
                break;


            // Check for the primitives that are returned to the user.
            case APSDE_DATA_confirm:
            case NLME_NETWORK_DISCOVERY_confirm:
            case NLME_NETWORK_FORMATION_confirm:
            case NLME_PERMIT_JOINING_confirm:
            case NLME_START_ROUTER_confirm:
            case NLME_JOIN_confirm:
            case NLME_DIRECT_JOIN_confirm:
            case NLME_LEAVE_confirm:
            case NLME_RESET_confirm:
            case NLME_SYNC_confirm:
            case NLME_GET_confirm:
            case NLME_SET_confirm:
            case NLME_JOIN_indication:
            case NLME_LEAVE_indication:
            case NLME_SYNC_indication:
            case APSDE_DATA_indication:
            case APSME_BIND_confirm:
            case APSME_UNBIND_confirm:
                *command = ZigBeeStatus.nextZigBeeState;
                if (ZDOHasBackgroundTasks() || APSHasBackgroundTasks() || NWKHasBackgroundTasks() ||
                    MACHasBackgroundTasks() || PHYHasBackgroundTasks())
                {
                    ZigBeeStatus.flags.bits.bHasBackgroundTasks = 1;
                    return TRUE;
                }
                else
                {
                    ZigBeeStatus.flags.bits.bHasBackgroundTasks = 0;
                    return FALSE;
                }
                break;
        }
    } while (ZigBeeStatus.nextZigBeeState != NO_PRIMITIVE);

    *command = ZigBeeStatus.nextZigBeeState;
    if (ZDOHasBackgroundTasks() || APSHasBackgroundTasks() || NWKHasBackgroundTasks() ||
        MACHasBackgroundTasks() || PHYHasBackgroundTasks())
    {
        ZigBeeStatus.flags.bits.bHasBackgroundTasks = 1;
        return TRUE;
    }
    else
    {
        // TRACE("Exiting ZigbeeTasks() with tasks remaining\r\n");
        ZigBeeStatus.flags.bits.bHasBackgroundTasks = 0;
        return FALSE;
    }
}
