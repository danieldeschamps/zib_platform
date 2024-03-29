/*********************************************************************
 *
 *                  ZigBee APS Layer
 *
 *********************************************************************
 * FileName:        zAPS.c
 * Dependencies:
 * Processor:       PIC18F
 * Complier:        MCC18 v3.00 or higher
 * Company:         Microchip Technology, Inc.
 *
 * Software License Agreement
 *
 * The software supplied herewith by Microchip Technology Incorporated
 * (the �Company�) for its PIC� microcontroller is intended and
 * supplied to you, the Company�s customer, for use solely and
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
 * THIS SOFTWARE IS PROVIDED IN AN �AS IS� CONDITION. NO WARRANTIES,
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

//#include "zigbee.def"
#include "ZigBeeTasks.h"
#include "sralloc.h"
#include "zAPS.h"
#include "zNWK.h"
#include "zMAC.h"
#include "zNVM.h"


#include "console.h"

// ******************************************************************************
// Configuration Definitions and Error Checks

#ifdef I_AM_RFD
    #if !defined(RFD_POLL_RATE)
        #error Please use ZENA(TM) to define the internal stack message poll rate.
    #endif
#endif


// ******************************************************************************
// Constant Definitions

//-----------------------------------------------------------------------------
// Frame Control Bits
#define APS_FRAME_DATA              0x00
#define APS_FRAME_COMMAND           0x01
#define APS_FRAME_ACKNOWLEDGE       0x02

#define APS_DELIVERY_DIRECT         0x00
#define APS_DELIVERY_INDIRECT       0x01
#define APS_DELIVERY_BROADCAST      0x02
#define APS_DELIVERY_RESERVED       0x03

#define APS_INDIRECT_ADDRESS_MODE_TO_COORD      0x01
#define APS_INDIRECT_ADDRESS_MODE_FROM_COORD    0x00

#define APS_SECURITY_ON             0x01
#define APS_SECURITY_OFF            0x00

#define APS_ACK_REQUESTED           0x01
#define APS_ACK_NOT_REQUESTED       0x00
//-----------------------------------------------------------------------------


// This value marks whether or not the binding table contains valid
// values left over from before a loss of power.
#define BINDING_TABLE_VALID        0xC35A

#define END_BINDING_RECORDS         0xff

// ******************************************************************************
// Data Structures

typedef union _APS_FRAME_CONTROL
{
    BYTE    Val;
    struct _APS_FRAME_CONTROL_bits
    {
        BYTE    frameType           : 2;
        BYTE    deliveryMode        : 2;
        BYTE    indirectAddressMode : 1;
        BYTE    security            : 1;
        BYTE    acknowledgeRequest  : 1;
        BYTE                        : 1;
    } bits;
} APS_FRAME_CONTROL;

typedef struct _APL_FRAME_INFO
{
#ifdef I_AM_RFD
    TICK                dataRequestTimer;
#endif
    WORD_VAL            profileID;
    SHORT_ADDR          shortDstAddress;
    BYTE                *message;
    APS_FRAME_CONTROL   apsFrameControl;
    BYTE                messageLength;
    BYTE                radiusCounter;
    BYTE                clusterID;
    BYTE                confirmationIndex;
    union
    {
        BYTE    Val;
        struct
        {
            BYTE    nTransmitAttempts   : 3;
            BYTE    bSendMessage        : 1;
            BYTE    nDiscoverRoute      : 4;
        } bits;
    } flags;
} APL_FRAME_INFO;

#if NUM_BUFFERED_INDIRECT_MESSAGES > 14
    #error Maximum buffered indirect messages is 14.
#endif
#define INVALID_INDIRECT_RELAY_INDEX      15

#if MAX_APL_FRAMES > 14
    #error Maximum APL messages is 14.
#endif
#define INVALID_APL_INDEX      15

typedef struct _APS_FRAMES
{
    TICK    timeStamp;
    ADDR    DstAddress;
    BYTE    nsduHandle;
    BYTE    DstAddrMode;
    BYTE    SrcEndpoint;
    BYTE    DstEndpoint;
    union
    {
        BYTE    Val;
        struct
        {
            BYTE    nIndirectRelayIndex     : 4;
            BYTE    bWaitingForAck          : 1;
            #ifdef I_SUPPORT_BINDINGS
                BYTE    bWeAreOriginator    : 1;
            #endif
        } bits;
    } flags;
} APS_FRAMES;
#define nAPLFrameIndex nIndirectRelayIndex

#if apscMaxFrameRetries > 7
    #error apscMaxFrameRetries too large.
#endif

typedef struct _INDIRECT_MESSAGE_INFO
{
    WORD_VAL            profileID;
    BYTE                *message;
    APS_FRAME_CONTROL   apsFrameControl;
    BYTE                messageLength;
    BYTE                currentBinding;
    BYTE                sourceEndpoint;     // Used only for messages from upper layers
    union
    {
        BYTE    Val;
        struct
        {
            BYTE    nTransmitAttempts   : 3;
            BYTE    bSendMessage        : 1;
            BYTE    bFromMe             : 1;
        } bits;
    } flags;
} INDIRECT_MESSAGE_INFO;


typedef struct _APS_STATUS
{
    union _NWK_STATUS_flags
    {
        BYTE    Val;
        struct _APS_STATUS_bits
        {
            // Background Task Flags
            BYTE    bSendingIndirectMessage     : 1;
            BYTE    bFramesPendingConfirm       : 1;
            BYTE    bDataIndicationPending      : 1;
            BYTE    bFramesAwaitingTransmission : 1;

            // Status Flags
        } bits;
    } flags;

    // If we receive an indirect message and must relay it, or an indirect transmission
    // is sent down from the upper layers, we must buffer it for multiple transmissions
    // from the background.
    #if defined (I_SUPPORT_BINDINGS)
        INDIRECT_MESSAGE_INFO   *indirectMessages[NUM_BUFFERED_INDIRECT_MESSAGES];
    #endif

    // All messages that come in from the APL level are actually sent in the background,
    // because they must be buffered to allow for multiple retries.
    APL_FRAME_INFO  *aplMessages[MAX_APL_FRAMES];

    // If we get a message that requires an APS ACK, we must buffer the message, send the ACK,
    // and then return the APSDE_DATA_indication in the background, using the buffered message info.
    struct _ACK_MESSAGE
    {
        BYTE        asduLength;
        BYTE        SecurityStatus;
        BYTE *      asdu;
        WORD_VAL    ProfileId;
        BYTE        SrcAddrMode;
        BYTE        WasBroadcast;
        SHORT_ADDR  SrcAddress;         // NOTE: This is ADDR in params, but we cannot get a long address from the NWK layer
        BYTE        SrcEndpoint;
        BYTE        DstEndpoint;
        BYTE        ClusterId;
    } ackMessage;

} APS_STATUS;
#define APS_BACKGROUND_TASKS 0x0F


// ******************************************************************************
// Variable Definitions

BYTE            aplTransId;

APS_STATUS      apsStatus;
APS_FRAMES      *apsConfirmationHandles[MAX_APS_FRAMES];

// ******************************************************************************
// Function Prototypes

BOOL LookupAPSAddress( LONG_ADDR *longAddr );

#if defined(I_SUPPORT_BINDINGS)
BYTE LookupSourceBindingInfo( SHORT_ADDR srcAddr, BYTE srcEP, BYTE clusterID );
void RemoveAllBindings( SHORT_ADDR shortAddr );
#endif


/*********************************************************************
 * Function:        BOOL APSHasBackgroundTasks( void )
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          TRUE - APS layer has background tasks to run
 *                  FALSE - APS layer does not have background tasks
 *
 * Side Effects:    None
 *
 * Overview:        Determines if the APS layer has background tasks
 *                  that need to be run.
 *
 * Note:            None
 ********************************************************************/

BOOL APSHasBackgroundTasks( void )
{
    return ((apsStatus.flags.Val & APS_BACKGROUND_TASKS) != 0);
}


/*********************************************************************
 * Function:        void APSInit( void )
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    APS layer data structures are initialized.
 *
 * Overview:        This routine initializes all APS layer data
 *                  structures.
 *
 * Note:            This routine is intended to be called as part of
 *                  a network or power-up initialization.  If called
 *                  after the network has been running, heap space
 *                  may be lost unless the heap is also reinitialized.
 ********************************************************************/

void APSInit( void )
{
    BYTE        i;
    #if defined(I_SUPPORT_BINDINGS)
        WORD    key;
    #endif

    apsStatus.flags.Val = 0;

    // Initialize APS frame handles awaiting confirmation.
    for (i=0; i<MAX_APS_FRAMES; i++)
    {
        apsConfirmationHandles[i] = NULL;
    }

    // Initialize the buffered APL message pointers.
    for (i=0; i<MAX_APL_FRAMES; i++)
    {
        apsStatus.aplMessages[i] = NULL;
    }

    #if defined(I_SUPPORT_BINDINGS)
        for (i=0; i<NUM_BUFFERED_INDIRECT_MESSAGES; i++)
        {
            apsStatus.indirectMessages[i] = NULL;
        }

        GetBindingValidityKey( &key );
        if (key != BINDING_TABLE_VALID)
        {
            key = BINDING_TABLE_VALID;
            PutBindingValidityKey( &key );
            ClearBindingTable();
        }
    #endif

    aplTransId = 0;
}

/*********************************************************************
 * Function:        ZIGBEE_PRIMITIVE APSTasks(ZIGBEE_PRIMITIVE inputPrimitive)
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
 * Note:            If this routine is called with NO_PRIMITIVE, it is
 *                  assumed that the TX and RX paths are not blocked,
 *                  and the background tasks may initiate a transmission.
 *                  It is the responsibility of this task to ensure that
 *                  only one output primitive is generated by any path.
 *                  If multiple output primitives are generated, they
 *                  must be generated one at a time by background processing.
 ********************************************************************/

ZIGBEE_PRIMITIVE APSTasks(ZIGBEE_PRIMITIVE inputPrimitive)
{
    BYTE    i;
    BYTE    j;
    BYTE    *ptr;


    if (inputPrimitive == NO_PRIMITIVE)
    {
        // Perform background tasks

        // ---------------------------------------------------------------------
        // Handle pending data indication from an ACK request
        if (apsStatus.flags.bits.bDataIndicationPending)
        {
            apsStatus.flags.bits.bDataIndicationPending = 0;
            #ifdef I_AM_RFD
                ZigBeeStatus.flags.bits.bRequestingData = 0;
            #endif

            params.APSDE_DATA_indication.asduLength             = apsStatus.ackMessage.asduLength;
            // TODO params.APSDE_DATA_indication.SecurityStatus = apsStatus.ackMessage.SecurityStatus;
            params.APSDE_DATA_indication.asdu                   = apsStatus.ackMessage.asdu;
            params.APSDE_DATA_indication.ProfileId              = apsStatus.ackMessage.ProfileId;
            params.APSDE_DATA_indication.SrcAddrMode            = apsStatus.ackMessage.SrcAddrMode;
            params.APSDE_DATA_indication.WasBroadcast           = apsStatus.ackMessage.WasBroadcast;
            params.APSDE_DATA_indication.SrcAddress.ShortAddr   = apsStatus.ackMessage.SrcAddress;
            params.APSDE_DATA_indication.SrcEndpoint            = apsStatus.ackMessage.SrcEndpoint;
            params.APSDE_DATA_indication.DstEndpoint            = apsStatus.ackMessage.DstEndpoint;
            params.APSDE_DATA_indication.ClusterId              = apsStatus.ackMessage.ClusterId;
            return APSDE_DATA_indication;
        }

        // ---------------------------------------------------------------------
        // Handle frames awaiting sending (or resending) from upper layers
        if (apsStatus.flags.bits.bFramesAwaitingTransmission)
        {
            #ifdef I_AM_RFD
            if (ZigBeeReady() && ZigBeeStatus.flags.bits.bDataRequestComplete)
            #else
            if (ZigBeeReady())
            #endif
            {
                for (i=0; i<MAX_APL_FRAMES; i++)
                {
                    if (apsStatus.aplMessages[i] != NULL)
                    {
                        if (apsStatus.aplMessages[i]->flags.bits.bSendMessage)
                        {
                            BYTE    cIndex;

                            cIndex = apsStatus.aplMessages[i]->confirmationIndex;

                            // If we've run out of retries, destroy everything and send up a confirm.
                            if (apsStatus.aplMessages[i]->flags.bits.nTransmitAttempts == 0)
                            {
							#ifdef DEBUG_RETRIES
								TRACE("APS: apscMaxFrameRetries exceeded\r\n");
							#endif
                                // We have run out of transmit attempts.  Prepare the confirmation primitive.
                                params.APSDE_DATA_confirm.Status        = APS_NO_ACK;
                                params.APSDE_DATA_confirm.DstAddrMode   = apsConfirmationHandles[cIndex]->DstAddrMode;
                                params.APSDE_DATA_confirm.DstAddress    = apsConfirmationHandles[cIndex]->DstAddress;
                                params.APSDE_DATA_confirm.SrcEndpoint   = apsConfirmationHandles[cIndex]->SrcEndpoint;
                                params.APSDE_DATA_confirm.DstEndpoint   = apsConfirmationHandles[cIndex]->DstEndpoint;

                                // Clean up everything.
                                if (apsStatus.aplMessages[i]->message != NULL)
                                {
                                    free( apsStatus.aplMessages[i]->message );
                                }
                                free( apsStatus.aplMessages[i] );
                                free( apsConfirmationHandles[cIndex] );
                                return APSDE_DATA_confirm;
                            }

							#ifdef DEBUG_RETRIES
							TRACE("APS: (Re)trying...\r\n");
							#endif
							                         
                            // We still have retries left.
                            // Load the primitive parameters.
                            params.NLDE_DATA_request.BroadcastRadius = apsStatus.aplMessages[i]->radiusCounter;
                            params.NLDE_DATA_request.DiscoverRoute   = apsStatus.aplMessages[i]->flags.bits.nDiscoverRoute;
                            //TODO params.NLDE_DATA_request.SecurityEnable
                            params.NLDE_DATA_request.NsduLength      = apsStatus.aplMessages[i]->messageLength;
                            params.NLDE_DATA_request.DstAddr         = apsStatus.aplMessages[i]->shortDstAddress;
                            params.NLDE_DATA_request.NsduHandle      = NLME_GET_nwkBCSN();

                            // Update the confirmation queue entry.  The entry already exists from when we received
                            // the original request.  We just need to fill in the nsduHandle and timeStamp.
                            apsConfirmationHandles[cIndex]->nsduHandle = params.NLDE_DATA_request.NsduHandle;
                            apsConfirmationHandles[cIndex]->timeStamp  = TickGet();

                            // Load the APS Payload.
                            for (j=0; j<params.NLDE_DATA_request.NsduLength; j++)
                            {
                                TxBuffer[TxData++] = apsStatus.aplMessages[i]->message[j];
                            }

                            // dds: Zigbee spec pg 54 (Data frame format)
							// Load the APS Header (backwards).
                            TxBuffer[TxHeader--] = apsConfirmationHandles[cIndex]->SrcEndpoint;
                            TxBuffer[TxHeader--] = apsStatus.aplMessages[i]->profileID.byte.MSB;
                            TxBuffer[TxHeader--] = apsStatus.aplMessages[i]->profileID.byte.LSB;
                            TxBuffer[TxHeader--] = apsStatus.aplMessages[i]->clusterID;
                            if (apsStatus.aplMessages[i]->apsFrameControl.bits.deliveryMode != APS_DELIVERY_INDIRECT)
                            {
                                TxBuffer[TxHeader--] = apsConfirmationHandles[cIndex]->DstEndpoint;
                            }
                            TxBuffer[TxHeader--] = apsStatus.aplMessages[i]->apsFrameControl.Val;

                            // Update the message info.
                            apsStatus.aplMessages[i]->flags.bits.nTransmitAttempts--;
                            apsStatus.aplMessages[i]->flags.bits.bSendMessage   = 0;
                            #ifdef I_AM_RFD
                                apsStatus.aplMessages[i]->dataRequestTimer      = TickGet();
                            #endif

                            apsStatus.flags.bits.bFramesPendingConfirm = 1;

                            ZigBeeBlockTx();
                            return NLDE_DATA_request;
                        }
#ifdef I_AM_RFD
                        else if (apsStatus.aplMessages[i]->apsFrameControl.bits.acknowledgeRequest)
                        {
                            TICK    currentTime;

                            currentTime = TickGet();
                            if (TickGetDiff( currentTime, apsStatus.aplMessages[i]->dataRequestTimer ) > (DWORD)RFD_POLL_RATE)
                            {
                                // Send a data request message so we can try to receive our ACK
                                apsStatus.aplMessages[i]->dataRequestTimer = currentTime;
                                params.NLME_SYNC_request.Track = FALSE;
                                return NLME_SYNC_request;
                            }
                        }
#endif
                    }
                }
            }

            for (i=0; (i<MAX_APL_FRAMES) && (apsStatus.aplMessages[i] == NULL); i++) {}
            if (i == MAX_APL_FRAMES)
            {
                //TRACE(APS: No APL frames awaiting transmission\r\n" );
                apsStatus.flags.bits.bFramesAwaitingTransmission = 0;
            }
        }

        // ---------------------------------------------------------------------
        // Handle frames awaiting confirmation
        if (apsStatus.flags.bits.bFramesPendingConfirm)
        {
            // NOTE: Compiler SSR27744, TickGet() output must be assigned to a variable.
            TICK    tempTick;

            tempTick = TickGet();
            for (i=0; i<MAX_APS_FRAMES; i++)
            {
                if (apsConfirmationHandles[i] != NULL)
                {
                    if ((apsConfirmationHandles[i]->nsduHandle != INVALID_NWK_HANDLE) &&
                        (TickGetDiff( tempTick, apsConfirmationHandles[i]->timeStamp ) > apscAckWaitDuration))
//                    if (TickGetDiff( TickGet(), apsConfirmationHandles[i]->timeStamp ) > apscAckWaitDuration)
                    {
                        // The frame has timed out while waiting for an ACK.  See if we can try again.

                        // Get the index to either the indirect message buffer of APL message buffer
                        // (nIndirectRelayIndex is the same as nAPLFrameIndex).
                        j = apsConfirmationHandles[i]->flags.bits.nAPLFrameIndex;

                        #ifdef I_SUPPORT_BINDINGS
                        if (apsConfirmationHandles[i]->flags.bits.bWeAreOriginator)
                        #endif
                        {
                            // We are the originator of the frame, so look in aplMessages.

                            // Try to send the message again.
                            apsStatus.aplMessages[j]->flags.bits.bSendMessage = 1;
                        }
                        #ifdef I_SUPPORT_BINDINGS
                        else
                        {
                            // We are trying to relay an indirect message, so look in indirectMessages.
                            // See if we have any more retries.  Otherwise, get the next destination.

                            if (apsStatus.indirectMessages[j]->flags.bits.nTransmitAttempts == 0)
                            {
                                // Get ready to send to the next destination.
                                pCurrentBindingRecord = &apsBindingTable[apsStatus.indirectMessages[j]->currentBinding];
                                GetBindingRecord(&currentBindingRecord, (ROM void*)pCurrentBindingRecord );
                                apsStatus.indirectMessages[j]->currentBinding = currentBindingRecord.nextBindingRecord;
                                if (apsStatus.indirectMessages[j]->apsFrameControl.bits.acknowledgeRequest)
                                {
                                    apsStatus.indirectMessages[j]->flags.bits.nTransmitAttempts = apscMaxFrameRetries + 1;
                                }
                                else
                                {
                                    apsStatus.indirectMessages[j]->flags.bits.nTransmitAttempts = 1;
                                }
                            }
                            apsStatus.indirectMessages[j]->flags.bits.bSendMessage = 1;
                        }
                        #endif

                        #ifdef I_SUPPORT_BINDINGS
                            if (!apsConfirmationHandles[i]->flags.bits.bWeAreOriginator)
                            {
                                free( apsConfirmationHandles[i] );
                            }
                        #endif
                    }
                }
            }

            for (i=0; (i<MAX_APS_FRAMES) && (apsConfirmationHandles[i]==NULL); i++) {}
            if (i == MAX_APS_FRAMES)
            {
                //TRACE(APS: No APS frames awaiting confirmation\r\n" );
                apsStatus.flags.bits.bFramesPendingConfirm = 0;
            }
        }

        // ---------------------------------------------------------------------
        // Handle relaying indirect messages

        #ifdef I_SUPPORT_BINDINGS
            if (apsStatus.flags.bits.bSendingIndirectMessage)
            {
                if (ZigBeeReady())
                {
                    for (i=0; i<NUM_BUFFERED_INDIRECT_MESSAGES; i++)
                    {
                        if (apsStatus.indirectMessages[i] != NULL)
                        {
                            if (apsStatus.indirectMessages[i]->currentBinding == END_BINDING_RECORDS)
                            {
                                // We have sent to all available destinations.
                                free( apsStatus.indirectMessages[i]->message );
                                free( apsStatus.indirectMessages[i] );
                            }
                            else
                            {
                                if (apsStatus.indirectMessages[i]->flags.bits.bSendMessage)
                                {
                                    // See who is the current destination
                                    pCurrentBindingRecord = &apsBindingTable[apsStatus.indirectMessages[i]->currentBinding];
                                    GetBindingRecord(&currentBindingRecord, (ROM void*)pCurrentBindingRecord );

                                    if (currentBindingRecord.shortAddr.Val == macPIB.macShortAddress.Val)
                                    {
                                        // The destination is me.  Pass this message up through the CurrentRxPacket.
                                        if (CurrentRxPacket != NULL)
                                        {
                                            // We are still processing a received packet, so we have to wait to pass this one up.
                                            goto BailFromSendingIndirect;
                                        }

                                        // Allocate a new buffer for internal message processing. We have to keep the original.
                                        if ((CurrentRxPacket = SRAMalloc(apsStatus.indirectMessages[i]->messageLength)) == NULL)
                                        {
                                            // We don't have room to process the message.  Ignore it and get ready for the next pass.
                                            // TODO is this how we want to handle it?
                                            apsStatus.indirectMessages[i]->currentBinding = currentBindingRecord.nextBindingRecord;
                                            goto BailFromSendingIndirect;
                                        }

                                        params.APSDE_DATA_indication.asdu         = CurrentRxPacket;
                                        params.APSDE_DATA_indication.asduLength   = apsStatus.indirectMessages[i]->messageLength;
                                        // SrcAddress is not known
                                        params.APSDE_DATA_indication.SrcAddrMode  = APS_ADDRESS_NOT_PRESENT;
                                        params.APSDE_DATA_indication.WasBroadcast = (apsStatus.indirectMessages[i]->apsFrameControl.bits.deliveryMode == APS_DELIVERY_BROADCAST);
                                        // TODO params.APSDE_DATA_indication.SecurityStatus
                                        params.APSDE_DATA_indication.DstEndpoint  = currentBindingRecord.endPoint;
                                        params.APSDE_DATA_indication.ClusterId    = currentBindingRecord.clusterID;
                                        params.APSDE_DATA_indication.ProfileId.Val= apsStatus.indirectMessages[i]->profileID.Val;
                                        // SrcEndpoint is not known

                                        // Copy the message.
                                        for (j=0; j<params.APSDE_DATA_indication.asduLength; j++)
                                        {
                                            params.APSDE_DATA_indication.asdu[j] = apsStatus.indirectMessages[i]->message[j];
                                        }

                                        // Get ready for the next pass.  nTransmitAttempts and bSendMessage are still set.
                                        apsStatus.indirectMessages[i]->currentBinding = currentBindingRecord.nextBindingRecord;

                                        #ifdef I_AM_RFD
                                            ZigBeeStatus.flags.bits.bRequestingData = 0;
                                        #endif
                                        if (params.APSDE_DATA_indication.DstEndpoint == 0)
                                        {
                                            return ZDO_DATA_indication;
                                        }
                                        else
                                        {
                                            return APSDE_DATA_indication;
                                        }
                                    }
                                    else
                                    {   // The destination is for someone else.

                                        // TODO We have lost the original values of BroadcastRadius, DiscoverRoute.  What do we use?
                                        params.NLDE_DATA_request.BroadcastRadius = DEFAULT_RADIUS;
                                        params.NLDE_DATA_request.DiscoverRoute = ROUTE_DISCOVERY_ENABLE;
                                        //TODO params.NLDE_DATA_request.SecurityEnable
                                        params.NLDE_DATA_request.NsduLength = apsStatus.indirectMessages[i]->messageLength;
                                        params.NLDE_DATA_request.DstAddr = currentBindingRecord.shortAddr;
                                        params.NLDE_DATA_request.NsduHandle = NLME_GET_nwkBCSN();

                                        // Add the frame to the confirmation queue so we can try to resend the frame if necessary.
                                        for (j=0; (j<MAX_APS_FRAMES) && (apsConfirmationHandles[j]!=NULL); j++) {}
                                        if ((j == MAX_APS_FRAMES) ||
                                            ((apsConfirmationHandles[j] = (APS_FRAMES *)SRAMalloc( sizeof(APS_FRAMES) )) == NULL))
                                        {
                                            // There was no room for another frame, so wait.
                                            goto BailFromSendingIndirect;
                                        }

                                        if (apsStatus.indirectMessages[i]->flags.bits.bFromMe)
                                        {
                                            // It was from me, so I need to convert it to a direct message.
                                            // So now I need to keep the source endpoint.  Great.
                                            TxBuffer[TxHeader--] = apsStatus.indirectMessages[i]->sourceEndpoint;
                                        }

                                        // Load the APS Header (backwards).
                                        TxBuffer[TxHeader--] = apsStatus.indirectMessages[i]->profileID.byte.MSB;
                                        TxBuffer[TxHeader--] = apsStatus.indirectMessages[i]->profileID.byte.LSB;
                                        TxBuffer[TxHeader--] = currentBindingRecord.clusterID;
                                        TxBuffer[TxHeader--] = currentBindingRecord.endPoint;
                                        TxBuffer[TxHeader--] = apsStatus.indirectMessages[i]->apsFrameControl.Val;

                                        // Load the confirmation handle entry.
                                        apsConfirmationHandles[j]->DstAddrMode                   = APS_ADDRESS_16_BIT;
                                        apsConfirmationHandles[j]->SrcEndpoint                   = apsStatus.indirectMessages[i]->sourceEndpoint; // Not always valid
                                        apsConfirmationHandles[j]->DstEndpoint                   = currentBindingRecord.endPoint;
                                        apsConfirmationHandles[j]->DstAddress.ShortAddr          = currentBindingRecord.shortAddr;
                                        apsConfirmationHandles[j]->flags.bits.bWaitingForAck     = 0;
                                        if (apsStatus.indirectMessages[i]->apsFrameControl.bits.acknowledgeRequest == APS_ACK_REQUESTED)
                                        {
                                            apsConfirmationHandles[j]->flags.bits.bWaitingForAck = 1;
                                        }
                                        apsConfirmationHandles[j]->flags.bits.bWeAreOriginator   = 0;
                                        apsConfirmationHandles[j]->timeStamp                     = TickGet();
                                        apsConfirmationHandles[j]->flags.bits.nIndirectRelayIndex= i;
                                        apsConfirmationHandles[j]->nsduHandle                    = params.NLDE_DATA_request.NsduHandle;

                                        // Update the indirect message info.  Leave currentBinding pointing to the
                                        // current destination in case we must try again.
                                        apsStatus.indirectMessages[i]->flags.bits.nTransmitAttempts--;
                                        apsStatus.indirectMessages[i]->flags.bits.bSendMessage  = 0;

                                        // Load the APS Payload.
                                        for (j=0; j<params.NLDE_DATA_request.NsduLength; j++)
                                        {
                                            TxBuffer[TxData++] = apsStatus.indirectMessages[i]->message[j];
                                        }

                                        ZigBeeBlockTx();
                                        return NLDE_DATA_request;
                                    }
                                } // ready to send a message
                            }
                        }
                    }
                }

BailFromSendingIndirect:
                // See if we are done relaying all indirect messages.
                for (i=0; (i<NUM_BUFFERED_INDIRECT_MESSAGES) && (apsStatus.indirectMessages[i]==NULL); i++) {}
                if (i == NUM_BUFFERED_INDIRECT_MESSAGES)
                {
                    //TRACE(APS: No indirect messages awaiting transmission\r\n" );
                    apsStatus.flags.bits.bSendingIndirectMessage = 0;
                }
            }
        #endif
    }
    else
    {
        switch (inputPrimitive)
        {
            case NLDE_DATA_confirm:
                for (i=0; i<MAX_APS_FRAMES; i++)
                {
                    if ((apsConfirmationHandles[i] != NULL) &&
                        (apsConfirmationHandles[i]->nsduHandle == params.NLDE_DATA_confirm.NsduHandle))
                    {
                        if (!apsConfirmationHandles[i]->flags.bits.bWaitingForAck ||
                            (params.NLDE_DATA_confirm.Status != NWK_SUCCESS))
                        {
FinishConfirmation:
                            {
                                // Get the index into the message info buffer
                                j = apsConfirmationHandles[i]->flags.bits.nIndirectRelayIndex;

                                #ifdef I_SUPPORT_BINDINGS
                                if (apsConfirmationHandles[i]->flags.bits.bWeAreOriginator)
                                #endif
                                {
                                    if (params.NLDE_DATA_confirm.Status != NWK_SUCCESS)
                                    {
                                        // The transmission failed, so try again.
                                        apsStatus.aplMessages[j]->flags.bits.bSendMessage = 1;
                                    }
                                    else
                                    {
										#ifdef DEBUG_RETRIES
										TRACE("APS: Received ACK...\r\n");
										#endif
                                        // We have a successful confirmation for a frame we transmitted for the upper layers.
                                        params.APSDE_DATA_confirm.DstAddrMode   = apsConfirmationHandles[i]->DstAddrMode;
                                        params.APSDE_DATA_confirm.DstAddress    = apsConfirmationHandles[i]->DstAddress;
                                        params.APSDE_DATA_confirm.SrcEndpoint   = apsConfirmationHandles[i]->SrcEndpoint;
                                        params.APSDE_DATA_confirm.DstEndpoint   = apsConfirmationHandles[i]->DstEndpoint;

                                        // Clean up everything.
                                        if (apsStatus.aplMessages[j]->message != NULL)
                                        {
                                            free( apsStatus.aplMessages[j]->message );
                                        }
                                        free( apsStatus.aplMessages[j] );
                                        free( apsConfirmationHandles[i] );

                                        return APSDE_DATA_confirm;
                                    }
                                }
                                #ifdef I_SUPPORT_BINDINGS
                                else
                                {
                                    // We're sending indirect messages.  See if we can go on to the next message
                                    // of if we need to retry the current one.  We can go on if either we've run
                                    // out of retries or the confirm came back with SUCCESS.
                                    if ((params.NLDE_DATA_confirm.Status == SUCCESS) ||
                                        (apsStatus.indirectMessages[j]->flags.bits.nTransmitAttempts == 0))
                                    {
                                        pCurrentBindingRecord = &apsBindingTable[apsStatus.indirectMessages[j]->currentBinding];
                                        GetBindingRecord(&currentBindingRecord, (ROM void*)pCurrentBindingRecord );
                                        apsStatus.indirectMessages[j]->currentBinding = currentBindingRecord.nextBindingRecord;
                                        if (apsStatus.indirectMessages[j]->apsFrameControl.bits.acknowledgeRequest)
                                        {
                                            apsStatus.indirectMessages[j]->flags.bits.nTransmitAttempts = apscMaxFrameRetries + 1;
                                        }
                                        else
                                        {
                                            apsStatus.indirectMessages[j]->flags.bits.nTransmitAttempts = 1;
                                        }
                                    }
                                    apsStatus.indirectMessages[j]->flags.bits.bSendMessage = 1;

                                    free( apsConfirmationHandles[i] );
                                }
                                #endif
                            }
                        }
                    }
                }
                break;

            case NLDE_DATA_indication:
                {
                    APS_FRAME_CONTROL   apsFrameControl;

                    // Since we can only get in here if we have not received an APS message that required an ACK waiting
                    // for a chance to be sent up with an APSDE_DATA_indication, we can use the apsStatus.ackMessage
                    // structure members for temporary storage.  We'll redefine them with local labels to avoid confusion.
                    // SHORT_ADDR          ackShortAddress;
                    // BYTE                destinationEP;
                    // BYTE                clusterID;
                    // WORD_VAL            profileID;
                    // BYTE                sourceEP;
                    #define destinationEPL      apsStatus.ackMessage.DstEndpoint
                    #define clusterIDL          apsStatus.ackMessage.ClusterId
                    #define profileIDL          apsStatus.ackMessage.ProfileId
                    #define sourceEPL           apsStatus.ackMessage.SrcEndpoint
                    #define addressModeL        apsStatus.ackMessage.SrcAddrMode
                    #define ackSourceAddressL   apsStatus.ackMessage.SrcAddress

                    // Start extracting the APS header
                    apsFrameControl.Val = APSGet();

                    if ((apsFrameControl.bits.frameType == APS_FRAME_DATA) ||
                        (apsFrameControl.bits.frameType == APS_FRAME_ACKNOWLEDGE))
                    {
                        // Finish reading the APS header
                        if (!((apsFrameControl.bits.deliveryMode == APS_DELIVERY_INDIRECT) &&
                              (apsFrameControl.bits.indirectAddressMode == APS_INDIRECT_ADDRESS_MODE_TO_COORD)))
                        {
                            destinationEPL   = APSGet();
                        }
                        clusterIDL           = APSGet();
                        profileIDL.byte.LSB  = APSGet();
                        profileIDL.byte.MSB  = APSGet();

                        if (!((apsFrameControl.bits.deliveryMode == APS_DELIVERY_INDIRECT) &&
                              (apsFrameControl.bits.indirectAddressMode == APS_INDIRECT_ADDRESS_MODE_FROM_COORD)))
                        {
                            sourceEPL        = APSGet();
                        }
                    }

                    if (apsFrameControl.bits.frameType == APS_FRAME_DATA)
                    {
#ifdef I_SUPPORT_BINDINGS
                        if ((apsFrameControl.bits.deliveryMode == APS_DELIVERY_DIRECT) ||
                            (apsFrameControl.bits.deliveryMode == APS_DELIVERY_BROADCAST) ||
                            ((apsFrameControl.bits.deliveryMode == APS_DELIVERY_INDIRECT) &&
                             (apsFrameControl.bits.indirectAddressMode == APS_INDIRECT_ADDRESS_MODE_FROM_COORD)))
#endif
                        {
                            // The packet is for me, either directly or indirectly

                            // Determine the address mode.
                            if (apsFrameControl.bits.deliveryMode == APS_DELIVERY_INDIRECT)
                            {
                                addressModeL = APS_ADDRESS_NOT_PRESENT;
                            }
                            else
                            {
                                addressModeL = APS_ADDRESS_16_BIT;
                            }

                            // Check for and send APS ACK here.
                            if (apsFrameControl.bits.acknowledgeRequest == APS_ACK_REQUESTED)
                            {
                                // Since we also need to process this message, buffer the info so we can send up an
                                // APSDE_DATA_indication as soon as possible. Set the flag to send the data indication
                                // in the background
                                apsStatus.flags.bits.bDataIndicationPending = 1;

                                // Buffer the old message info
                                apsStatus.ackMessage.asduLength     = params.NLDE_DATA_indication.NsduLength;
                                // TODO apsStatus.ackMessage.SecurityStatus         = ;
                                apsStatus.ackMessage.asdu           = params.NLDE_DATA_indication.Nsdu;
                                //apsStatus.ackMessage.ProfileId      = profileIDL;
                                //apsStatus.ackMessage.SrcAddrMode    = addressModeL;
                                apsStatus.ackMessage.WasBroadcast   = (apsFrameControl.bits.deliveryMode == APS_DELIVERY_BROADCAST);
                                apsStatus.ackMessage.SrcAddress     = params.NLDE_DATA_indication.SrcAddress;
                                //apsStatus.ackMessage.SrcEndpoint    = sourceEPL;
                                //apsStatus.ackMessage.DstEndpoint    = destinationEPL;
                                //apsStatus.ackMessage.ClusterId      = clusterIDL;

                                // Send the acknowledge
                                params.NLDE_DATA_request.DstAddr            = params.NLDE_DATA_indication.SrcAddress;
                                params.NLDE_DATA_request.BroadcastRadius    = DEFAULT_RADIUS;
                                params.NLDE_DATA_request.DiscoverRoute      = ROUTE_DISCOVERY_ENABLE;
                                //TODO params.NLDE_DATA_request.SecurityEnable
                                params.NLDE_DATA_request.NsduLength         = 0;
                                params.NLDE_DATA_request.NsduHandle         = NLME_GET_nwkBCSN();

                                // Load up the APS Header (backwards).  Note that the source and destination EP's get flipped.
                                TxBuffer[TxHeader--] = destinationEPL;
                                TxBuffer[TxHeader--] = profileIDL.byte.MSB;
                                TxBuffer[TxHeader--] = profileIDL.byte.LSB;
                                TxBuffer[TxHeader--] = clusterIDL;

                                if (apsFrameControl.bits.deliveryMode == APS_DELIVERY_INDIRECT)
                                {
                                    apsFrameControl.bits.indirectAddressMode = APS_INDIRECT_ADDRESS_MODE_TO_COORD;
                                }
                                else
                                {
                                    TxBuffer[TxHeader--] = sourceEPL;
                                }
                                apsFrameControl.bits.acknowledgeRequest = APS_ACK_NOT_REQUESTED;
                                apsFrameControl.bits.frameType          = APS_FRAME_ACKNOWLEDGE;

                                TxBuffer[TxHeader--] = apsFrameControl.Val;

                                ZigBeeBlockTx();
                                return NLDE_DATA_request;
                            }

                            // asduLength already in place
                            // *asdu already in place
                            // SrcAddress already in place
                            params.APSDE_DATA_indication.SrcAddrMode    = addressModeL;
                            params.APSDE_DATA_indication.WasBroadcast   = (apsFrameControl.bits.deliveryMode == APS_DELIVERY_BROADCAST);
                            // TODO params.APSDE_DATA_indication.SecurityStatus
                            params.APSDE_DATA_indication.DstEndpoint    = destinationEPL;
                            params.APSDE_DATA_indication.ClusterId      = clusterIDL;
                            params.APSDE_DATA_indication.ProfileId      = profileIDL;
                            params.APSDE_DATA_indication.SrcEndpoint    = sourceEPL;   // May be invalid.  SrcAddrMode will indicate.
                            if (params.APSDE_DATA_indication.DstEndpoint == 0)
                            {
                                return ZDO_DATA_indication;
                            }
                            else
                            {
                                return APSDE_DATA_indication;
                            }
                        }
#if defined(I_SUPPORT_BINDINGS)
                        // When Indirect message is received, look it up binding table
                        // and direct it to intended recipients.
                        else if (apsFrameControl.bits.deliveryMode == APS_DELIVERY_INDIRECT)
                        {
                            // Look for an empty place to buffer the message
                            for (i=0; (i<NUM_BUFFERED_INDIRECT_MESSAGES) && (apsStatus.indirectMessages[i]!=NULL); i++) {}

                            // Make sure we can buffer the message to resend it
                            if (i != NUM_BUFFERED_INDIRECT_MESSAGES)
                            {
                                // Try to buffer the message so we can send it to each destination in the background.
                                if ((apsStatus.indirectMessages[i] = (INDIRECT_MESSAGE_INFO *)SRAMalloc( sizeof(INDIRECT_MESSAGE_INFO) )) != NULL)
                                {
                                    // Make sure there is at least one destination.
                                    if (LookupSourceBindingInfo( params.NLDE_DATA_indication.SrcAddress,
                                                    sourceEPL, clusterIDL) != END_BINDING_RECORDS )
                                    {
                                        // Buffer the message.  Change the indirect address mode for transmission.
                                        apsFrameControl.bits.indirectAddressMode = APS_INDIRECT_ADDRESS_MODE_FROM_COORD;
                                        apsStatus.indirectMessages[i]->apsFrameControl    = apsFrameControl;
                                        apsStatus.indirectMessages[i]->messageLength      = params.NLDE_DATA_indication.NsduLength;
                                        apsStatus.indirectMessages[i]->profileID          = profileIDL;
                                        apsStatus.indirectMessages[i]->flags.bits.bFromMe = 0;

                                        // Point to the first destination binding.
                                        apsStatus.indirectMessages[i]->currentBinding     = currentBindingRecord.nextBindingRecord;

                                        // Set the flag to send the message.
                                        apsStatus.indirectMessages[i]->flags.bits.bSendMessage = 1;

                                        if ((apsStatus.indirectMessages[i]->message = SRAMalloc(params.NLDE_DATA_indication.NsduLength)) != NULL)
                                        {
                                            ptr = apsStatus.indirectMessages[i]->message;
                                            while (params.NLDE_DATA_indication.NsduLength)
                                            {
                                                *ptr++ = APSGet();
                                            }
                                            apsStatus.flags.bits.bSendingIndirectMessage = 1;
                                            APSDiscardRx();

                                            // Check for APS ACK.  Set transmit attempts for forwarded message and send ACK
                                            if (apsFrameControl.bits.acknowledgeRequest == APS_ACK_REQUESTED)
                                            {
                                                // Set the transmission attempts for an acknowledged message.
                                                apsStatus.indirectMessages[i]->flags.bits.nTransmitAttempts = apscMaxFrameRetries + 1;

                                                // Send the acknowledge
                                                params.NLDE_DATA_request.DstAddr            = params.NLDE_DATA_indication.SrcAddress;
                                                params.NLDE_DATA_request.BroadcastRadius    = DEFAULT_RADIUS;
                                                params.NLDE_DATA_request.DiscoverRoute      = ROUTE_DISCOVERY_ENABLE;
                                                //TODO params.NLDE_DATA_request.SecurityEnable
                                                params.NLDE_DATA_request.NsduLength          = 0;
                                                // NsduHandle is TxBuffer

                                                // Load up the APS Header (backwards).  Note that the source and destination EP's get flipped.
                                                // No Source Endpoint
                                                TxBuffer[TxHeader--] = profileIDL.byte.MSB;
                                                TxBuffer[TxHeader--] = profileIDL.byte.LSB;
                                                TxBuffer[TxHeader--] = clusterIDL;
                                                TxBuffer[TxHeader--] = sourceEPL;   // Old source EP goes as destination

                                                apsFrameControl.bits.indirectAddressMode    = APS_INDIRECT_ADDRESS_MODE_FROM_COORD;
                                                apsFrameControl.bits.acknowledgeRequest     = APS_ACK_NOT_REQUESTED;
                                                apsFrameControl.bits.frameType              = APS_FRAME_ACKNOWLEDGE;

                                                TxBuffer[TxHeader--] = apsFrameControl.Val;

                                                ZigBeeBlockTx();
                                                return NLDE_DATA_request;
                                            }
                                            else
                                            {
                                                // Set the transmission attempts for a non-acknowledged message.
                                                apsStatus.indirectMessages[i]->flags.bits.nTransmitAttempts = 1;
                                            }

                                            return NO_PRIMITIVE;
                                        }
                                    }
                                    // If there were no destinations or we couldn't allocate room for the message itself, free the buffer.
                                    free( apsStatus.indirectMessages[i] );
                                }
                            }
                        }
#endif
                    }
                    else if (apsFrameControl.bits.frameType == APS_FRAME_ACKNOWLEDGE)
                    {
                        for (i=0; i<MAX_APS_FRAMES; i++)
                        {
                            if (apsConfirmationHandles[i] != NULL)
                            {
                                if (apsConfirmationHandles[i]->DstAddrMode == APS_ADDRESS_64_BIT)
                                {
                                    #if MAX_APS_ADDRESSES > 0
                                        if (!LookupAPSAddress( &apsConfirmationHandles[i]->DstAddress.LongAddr ))
                                        {
                                            ackSourceAddressL = currentAPSAddress.shortAddr;
                                        }
                                        else
                                        {
                                            continue;
                                        }
                                    #else
                                        continue;
                                    #endif
                                }
                                else
                                {
                                    ackSourceAddressL = apsConfirmationHandles[i]->DstAddress.ShortAddr;
                                }

                                if (ackSourceAddressL.Val == params.NLDE_DATA_indication.SrcAddress.Val)
                                {
                                    if (apsFrameControl.bits.deliveryMode == APS_DELIVERY_DIRECT)
                                    {
                                        // Receiving an ACK for a direct delivery packet.
                                        if ((destinationEPL != apsConfirmationHandles[i]->SrcEndpoint) ||
                                            (sourceEPL != apsConfirmationHandles[i]->DstEndpoint))
                                        {
                                            continue;
                                        }
                                    }
                                    else
                                    {
                                        // Receiving an ACK for an indirect delivery packet.
                                        if (apsFrameControl.bits.indirectAddressMode == APS_INDIRECT_ADDRESS_MODE_FROM_COORD)
                                        {
                                            // sourceEP is not present
                                            if (destinationEPL != apsConfirmationHandles[i]->SrcEndpoint)
                                            {
                                                continue;
                                            }
                                        }
                                        else
                                        {
                                            // destinationEP is not present
                                            if (sourceEPL != apsConfirmationHandles[i]->DstEndpoint)
                                            {
                                                continue;
                                            }
                                        }
                                    }

                                    // If all the above tests pass, the frame has been ACK'd
                                    APSDiscardRx();

                                    params.APSDE_DATA_confirm.Status        = SUCCESS;

                                    // This is the same as when we get an NLDE_DATA_confirm
                                    goto FinishConfirmation;
                                }
                            }
                        }
                    }
                    // There are currently no APS-level commands

                    APSDiscardRx();
                    return NO_PRIMITIVE;
                }
                break;
                #undef destinationEPL
                #undef clusterIDL
                #undef profileIDL
                #undef sourceEPL
                #undef addressModeL
                #undef ackSourceAddressL

            case APSDE_DATA_request:
                {
                    #ifndef I_SUPPORT_BINDINGS
                        // Bindings are not supported, so all messages are buffered in the aplMessages buffer
                        // and all messages have an apsConfirmationHandles entry.

                        // Validate what we can before allocating space.
                        if (params.APSDE_DATA_request.DstAddrMode == APS_ADDRESS_64_BIT)
                        {
#if MAX_APS_ADDRESSES > 0
                            if (!LookupAPSAddress( &params.APSDE_DATA_request.DstAddress.LongAddr ))
#endif
                            {
                                // We do not have a short address for this long address, so return an error.
                                params.APSDE_DATA_confirm.Status = APS_INVALID_REQUEST;
                                ZigBeeUnblockTx();
                                return APSDE_DATA_confirm;
                            }
                        }

                        // Prepare a confirmation handle entry.
                        for (i=0; (i<MAX_APS_FRAMES) && (apsConfirmationHandles[i]!=NULL); i++) {}
                        if ((i == MAX_APS_FRAMES) ||
                            ((apsConfirmationHandles[i] = (APS_FRAMES *)SRAMalloc( sizeof(APS_FRAMES) )) == NULL))
                        {
                            // There was no room for another frame, so return an error.
                            params.APSDE_DATA_confirm.Status = TRANSACTION_OVERFLOW;
                            ZigBeeUnblockTx();
                            return APSDE_DATA_confirm;
                        }

                        // Prepare an APL message buffer
                        for (j=0; (j<MAX_APL_FRAMES) && (apsStatus.aplMessages[j]!=NULL); j++) {}
                        if ((j == MAX_APL_FRAMES) ||
                            ((apsStatus.aplMessages[j] = (APL_FRAME_INFO *)SRAMalloc( sizeof(APL_FRAME_INFO) )) == NULL))
                        {
                            // There was no room for another frame, so return an error.
                            free( apsConfirmationHandles[i] );
                            params.APSDE_DATA_confirm.Status = TRANSACTION_OVERFLOW;
                            ZigBeeUnblockTx();
                            return APSDE_DATA_confirm;
                        }

                        // Load the confirmation handle entry.  Set nsduHandle to INVALID, since we do the actual
                        // transmission from the background.
                        apsConfirmationHandles[i]->nsduHandle                   = INVALID_NWK_HANDLE;
                        apsConfirmationHandles[i]->DstAddrMode                  = params.APSDE_DATA_request.DstAddrMode;
                        apsConfirmationHandles[i]->SrcEndpoint                  = params.APSDE_DATA_request.SrcEndpoint;
                        apsConfirmationHandles[i]->DstEndpoint                  = params.APSDE_DATA_request.DstEndpoint;
                        apsConfirmationHandles[i]->DstAddress                   = params.APSDE_DATA_request.DstAddress; // May change later...
                        apsConfirmationHandles[i]->flags.bits.nAPLFrameIndex    = j;
                        apsConfirmationHandles[i]->flags.bits.bWaitingForAck    = FALSE;    // May change later...
                        apsConfirmationHandles[i]->timeStamp                    = TickGet();

                        // Start loading the APL message info.
                        if ((apsStatus.aplMessages[j]->message = SRAMalloc( TxData )) == NULL)
                        {
                            free( apsStatus.aplMessages[j] );
                            free( apsConfirmationHandles[i] );
                            params.APSDE_DATA_confirm.Status = TRANSACTION_OVERFLOW;
                            ZigBeeUnblockTx();
                            return APSDE_DATA_confirm;
                        }
                        apsStatus.aplMessages[j]->profileID                     = params.APSDE_DATA_request.ProfileId;
                        apsStatus.aplMessages[j]->radiusCounter                 = params.APSDE_DATA_request.RadiusCounter;
                        apsStatus.aplMessages[j]->clusterID                     = params.APSDE_DATA_request.ClusterId;
                        apsStatus.aplMessages[j]->confirmationIndex             = i;
                        apsStatus.aplMessages[j]->flags.bits.nDiscoverRoute     = params.APSDE_DATA_request.DiscoverRoute;
                        apsStatus.aplMessages[j]->shortDstAddress               = params.APSDE_DATA_request.DstAddress.ShortAddr; // May not be correct - fixed later.
                        apsStatus.aplMessages[j]->flags.bits.bSendMessage       = 1;
                        apsStatus.aplMessages[j]->messageLength                 = TxData;

                        // Start building the frame control.
                        apsStatus.aplMessages[j]->apsFrameControl.Val = APS_FRAME_DATA;   // APS_DELIVERY_DIRECT

                        if (params.APSDE_DATA_request.TxOptions.bits.securityEnabled)
                        {
                            apsStatus.aplMessages[j]->apsFrameControl.bits.security = APS_SECURITY_ON;
                        }

                        if (params.APSDE_DATA_request.TxOptions.bits.acknowledged)
                        {
							#ifdef DEBUG_RETRIES
							TRACE("APS: This is an acknowledged transmission\r\n");
							#endif
                            apsConfirmationHandles[i]->flags.bits.bWaitingForAck = TRUE;
                            apsStatus.aplMessages[j]->apsFrameControl.bits.acknowledgeRequest = APS_ACK_REQUESTED;
                            apsStatus.aplMessages[j]->flags.bits.nTransmitAttempts  = apscMaxFrameRetries + 1;
                        }
                        else
                        {
						#ifdef DEBUG_RETRIES
							TRACE("APS: This is NOT an acknowledged transmission\r\n");
						#endif
                            apsStatus.aplMessages[j]->flags.bits.nTransmitAttempts  = 1;
                        }

#if MAX_APS_ADDRESSES > 0
                        if (params.APSDE_DATA_request.DstAddrMode == APS_ADDRESS_64_BIT)
                        {
                            // If we get to here, then currentAPSAddress is waiting for us.
                            apsStatus.aplMessages[j]->shortDstAddress = currentAPSAddress.shortAddr;
                            // apsFrameControl.bits.deliveryMode = APS_DELIVERY_DIRECT, which is 0x00
                        }
                        else
#endif
                        if (params.APSDE_DATA_request.DstAddrMode == APS_ADDRESS_16_BIT)
                        {
                            // NOTE According to the spec, broadcast at this level means the frame
                            // goes to all devices AND all endpoints.  That makes no sense - EP0 Cluster 0
                            // means something very different from Cluster 0 on any other endpoint.  We'll
                            // set it for all devices...
                            if (params.APSDE_DATA_request.DstAddress.ShortAddr.Val == 0xFFFF)
                            {
                                apsStatus.aplMessages[j]->apsFrameControl.bits.deliveryMode = APS_DELIVERY_BROADCAST;
                            }
                            // else apsFrameControl.bits.deliveryMode = APS_DELIVERY_DIRECT, which is 0x00
                        }
                        else
                        {
                            // Since we do not support binding, then we must be sending to the coordinator
                            apsStatus.aplMessages[j]->shortDstAddress.Val                       = 0x0000;
                            apsStatus.aplMessages[j]->apsFrameControl.bits.deliveryMode         = APS_DELIVERY_INDIRECT;
                            apsStatus.aplMessages[j]->apsFrameControl.bits.indirectAddressMode  = APS_INDIRECT_ADDRESS_MODE_TO_COORD;
                            apsConfirmationHandles[i]->DstAddress.ShortAddr.Val                 = 0x0000;
                        }
                        //apsStatus.aplMessages[j]->apsFrameControl = apsFrameControl;

                        // Buffer the message payload.
                        ptr = apsStatus.aplMessages[i]->message;
                        i = 0;
                        while (TxData--)
                        {
                            *ptr++ = TxBuffer[i++];
                        }

                        apsStatus.flags.bits.bFramesAwaitingTransmission = 1;
                        ZigBeeUnblockTx();
                        return NO_PRIMITIVE;

                    #else // #ifndef I_SUPPORT_BINDINGS

                        // Bindings are supported, so we are either a coordinator or a router.  If we are sending
                        // a direct message or an indirect message to the coordinator, we need to create an
                        // aplMessages and an apsConfirmationHandles entry.  If we are trying to send indirect
                        // messages from the coordinator, we need to create an indirectMessages queue entry.

                        APS_FRAME_CONTROL   apsFrameControl;

                        // Start building the frame control.
                        apsFrameControl.Val = APS_FRAME_DATA;   // and APS_DELIVERY_DIRECT
                        if (params.APSDE_DATA_request.TxOptions.bits.securityEnabled)
                        {
                            apsFrameControl.bits.security = APS_SECURITY_ON;
                        }

                        if (params.APSDE_DATA_request.TxOptions.bits.acknowledged)
                        {
	                        #ifdef DEBUG_RETRIES
							TRACE("APS: This is an acknowledged transmission\r\n");
							#endif
                            apsFrameControl.bits.acknowledgeRequest = APS_ACK_REQUESTED;
                        }
                        else
                        {
	                        #ifdef DEBUG_RETRIES
							TRACE("APS: This is NOT an acknowledged transmission\r\n");
							#endif
						}

                        // Validate what we can before allocating space, and determine the addressing mode.
                        if (params.APSDE_DATA_request.DstAddrMode == APS_ADDRESS_64_BIT)
                        {
#if MAX_APS_ADDRESSES > 0
                            if (!LookupAPSAddress( &params.APSDE_DATA_request.DstAddress.LongAddr ))
#endif
                            {
                                // We do not have a short address for this long address, so return an error.
                                params.APSDE_DATA_confirm.Status = APS_INVALID_REQUEST;
                                ZigBeeUnblockTx();
                                return APSDE_DATA_confirm;
                            }
                            // else apsFrameControl.bits.deliveryMode = APS_DELIVERY_DIRECT, which is 0x00
                        }
                        else if (params.APSDE_DATA_request.DstAddrMode == APS_ADDRESS_16_BIT)
                        {
                            // NOTE According to the spec, broadcast at this level means the frame
                            // goes to all devices AND all endpoints.  That makes no sense - EP0 Cluster 0
                            // means something very different from Cluster 0 on any other endpoint.  We'll
                            // set it for all devices...
                            if (params.APSDE_DATA_request.DstAddress.ShortAddr.Val == 0xFFFF)
                            {
                                apsFrameControl.bits.deliveryMode = APS_DELIVERY_BROADCAST;
                            }
                            // else apsFrameControl.bits.deliveryMode = APS_DELIVERY_DIRECT, which is 0x00
                        }
                        else
                        {
                            #if defined (I_AM_ROUTER)
                                if (LookupSourceBindingInfo( macPIB.macShortAddress, params.APSDE_DATA_request.SrcEndpoint, params.APSDE_DATA_request.ClusterId ) == END_BINDING_RECORDS)
                                {
                                    // I don't have a binding for it, so send it to the coordinator.
                                    //later  params.APSDE_DATA_request.DstAddress.ShortAddr.Val = 0x0000;
                                    apsFrameControl.bits.deliveryMode        = APS_DELIVERY_INDIRECT;
                                    apsFrameControl.bits.indirectAddressMode = APS_INDIRECT_ADDRESS_MODE_TO_COORD;
                                    goto BufferAPLMessage;
                                }
                            #else // I_AM_COORDINATOR
                                if (LookupSourceBindingInfo( macPIB.macShortAddress, params.APSDE_DATA_request.SrcEndpoint, params.APSDE_DATA_request.ClusterId ) == END_BINDING_RECORDS)
                                {
                                    // We have no bindings for this message.
                                    params.APSDE_DATA_confirm.Status = APS_NO_BOUND_DEVICE;
                                    ZigBeeUnblockTx();
                                    return APSDE_DATA_confirm;
                                }
                            #endif

                            // I am relaying an indirect message.  Buffer the message in the indirectMessages queue.
                            for (i=0; (i<NUM_BUFFERED_INDIRECT_MESSAGES) && (apsStatus.indirectMessages[i]!=NULL); i++) {}

                            // Make sure we can buffer the message to resend it
                            if (i != NUM_BUFFERED_INDIRECT_MESSAGES)
                            {
                                // Try to buffer the message so we can send it to each destination in the background.
                                if ((apsStatus.indirectMessages[i] = (INDIRECT_MESSAGE_INFO *)SRAMalloc( sizeof(INDIRECT_MESSAGE_INFO) )) != NULL)
                                {
                                    // Buffer the message.  Set the indirect address mode for transmission.
                                    // According to the spec, we need to convert these to direct messages.
                                    //apsFrameControl.bits.deliveryMode               = APS_DELIVERY_INDIRECT;
                                    //apsFrameControl.bits.indirectAddressMode        = APS_INDIRECT_ADDRESS_MODE_FROM_COORD;
                                    apsFrameControl.bits.deliveryMode                 = APS_DELIVERY_DIRECT;
                                    apsStatus.indirectMessages[i]->apsFrameControl    = apsFrameControl;
                                    apsStatus.indirectMessages[i]->messageLength      = TxData;
                                    apsStatus.indirectMessages[i]->profileID          = params.APSDE_DATA_request.ProfileId;
                                    apsStatus.indirectMessages[i]->currentBinding     = currentBindingRecord.nextBindingRecord;
                                    apsStatus.indirectMessages[i]->sourceEndpoint     = params.APSDE_DATA_request.SrcEndpoint;
                                    apsStatus.indirectMessages[i]->flags.bits.bFromMe = 1;

                                    if ((apsStatus.indirectMessages[i]->message = SRAMalloc(TxData)) != NULL)
                                    {
                                        ptr = apsStatus.indirectMessages[i]->message;
                                        j = 0;
                                        while (TxData--)
                                        {
                                            *ptr++ = TxBuffer[j++];
                                        }

                                        // Set up the retry information.
                                        if (apsFrameControl.bits.acknowledgeRequest)
                                        {
                                            apsStatus.indirectMessages[i]->flags.bits.nTransmitAttempts = apscMaxFrameRetries + 1;
                                        }
                                        else
                                        {
                                            apsStatus.indirectMessages[i]->flags.bits.nTransmitAttempts = 1;
                                        }
                                        apsStatus.indirectMessages[i]->flags.bits.bSendMessage = 1;

                                        apsStatus.flags.bits.bSendingIndirectMessage = 1;

                                        // Return SUCCESS that the message is succesfully buffered (as per spec)
                                        params.APSDE_DATA_confirm.Status = SUCCESS;
                                        ZigBeeUnblockTx();
                                        return APSDE_DATA_confirm;
                                    }
                                    // If we couldn't allocate room for the message itself, free the buffer.
                                    free( apsStatus.indirectMessages[i] );
                                }
                            }
                            // There was no room for another frame, so return an error.
                            params.APSDE_DATA_confirm.Status = TRANSACTION_OVERFLOW;
                            ZigBeeUnblockTx();
                            return APSDE_DATA_confirm;

                        }

                        // We are sending either an indirect message to the coordinator or a direct message to someone.
                        // Create both an aplMessages entry and an apsConfirmationHandles entry.
BufferAPLMessage:
                        // Prepare a confirmation handle entry.
                        for (i=0; (i<MAX_APS_FRAMES) && (apsConfirmationHandles[i]!=NULL); i++) {}
                        if ((i == MAX_APS_FRAMES) ||
                            ((apsConfirmationHandles[i] = (APS_FRAMES *)SRAMalloc( sizeof(APS_FRAMES) )) == NULL))
                        {
                            // There was no room for another frame, so return an error.
                            params.APSDE_DATA_confirm.Status = TRANSACTION_OVERFLOW;
                            ZigBeeUnblockTx();
                            return APSDE_DATA_confirm;
                        }

                        // Prepare an APL message buffer
                        for (j=0; (j<MAX_APL_FRAMES) && (apsStatus.aplMessages[j]!=NULL); j++) {} // dds: changed i yo j
                        if ((j == MAX_APL_FRAMES) ||
                            ((apsStatus.aplMessages[j] = (APL_FRAME_INFO *)SRAMalloc( sizeof(APL_FRAME_INFO) )) == NULL))
                        {
                            // There was no room for another frame, so return an error.
                            free( apsConfirmationHandles[i] );
                            params.APSDE_DATA_confirm.Status = TRANSACTION_OVERFLOW;
                            ZigBeeUnblockTx();
                            return APSDE_DATA_confirm;
                        }

                        // Load the confirmation handle entry.  Set nsduHandle to INVALID, and do not load
                        // timeStamp, since we do the actual transmission from the background.
                        apsConfirmationHandles[i]->nsduHandle                   = INVALID_NWK_HANDLE;
                        apsConfirmationHandles[i]->DstAddrMode                  = params.APSDE_DATA_request.DstAddrMode;
                        apsConfirmationHandles[i]->SrcEndpoint                  = params.APSDE_DATA_request.SrcEndpoint;
                        apsConfirmationHandles[i]->DstEndpoint                  = params.APSDE_DATA_request.DstEndpoint;
                        apsConfirmationHandles[i]->DstAddress                   = params.APSDE_DATA_request.DstAddress;
                        apsConfirmationHandles[i]->flags.bits.nAPLFrameIndex    = j;
                        apsConfirmationHandles[i]->flags.bits.bWaitingForAck    = 0;    // May change later...
                        apsConfirmationHandles[i]->flags.bits.bWeAreOriginator  = 1;

                        // Start loading the APL message info.
                        if ((apsStatus.aplMessages[j]->message = SRAMalloc( TxData )) == NULL)
                        {
                            free( apsStatus.aplMessages[j] );
                            free( apsConfirmationHandles[i] );
                            params.APSDE_DATA_confirm.Status = TRANSACTION_OVERFLOW;
                            ZigBeeUnblockTx();
                            return APSDE_DATA_confirm;
                        }
                        apsStatus.aplMessages[j]->profileID                      = params.APSDE_DATA_request.ProfileId;
                        apsStatus.aplMessages[j]->radiusCounter                  = params.APSDE_DATA_request.RadiusCounter;
                        apsStatus.aplMessages[j]->clusterID                      = params.APSDE_DATA_request.ClusterId;
                        apsStatus.aplMessages[j]->confirmationIndex              = i;
                        apsStatus.aplMessages[j]->shortDstAddress                = params.APSDE_DATA_request.DstAddress.ShortAddr; // May not be correct - fixed later.
                        apsStatus.aplMessages[j]->messageLength                  = TxData;
                        apsStatus.aplMessages[j]->flags.bits.nDiscoverRoute      = params.APSDE_DATA_request.DiscoverRoute;
                        apsStatus.aplMessages[j]->flags.bits.bSendMessage        = 1;

                        if (params.APSDE_DATA_request.TxOptions.bits.acknowledged)
                        {
                            apsConfirmationHandles[i]->flags.bits.bWaitingForAck   = 1;
                            apsStatus.aplMessages[j]->flags.bits.nTransmitAttempts = apscMaxFrameRetries + 1;
                        }
                        else
                        {
                            apsStatus.aplMessages[j]->flags.bits.nTransmitAttempts = 1;
                        }

#if MAX_APS_ADDRESSES > 0
                        if (params.APSDE_DATA_request.DstAddrMode == APS_ADDRESS_64_BIT)
                        {
                            // If we get to here, then currentAPSAddress is waiting for us.
                            apsStatus.aplMessages[j]->shortDstAddress = currentAPSAddress.shortAddr;
                        }
                        else
#endif
                        if (params.APSDE_DATA_request.DstAddrMode == APS_ADDRESS_NOT_PRESENT)
                        {
                            // Since we do not support binding, then we must be sending to the coordinator
                            apsStatus.aplMessages[j]->shortDstAddress.Val = 0x0000;
                            apsFrameControl.bits.deliveryMode             = APS_DELIVERY_INDIRECT;
                            apsFrameControl.bits.indirectAddressMode      = APS_INDIRECT_ADDRESS_MODE_TO_COORD;
                        }
                        // else APS_ADDRESS_16_BIT, and everything is already set up

                        apsStatus.aplMessages[j]->apsFrameControl = apsFrameControl;

                        // Buffer the message payload.
                        ptr = apsStatus.aplMessages[i]->message;
                        i = 0;
                        while (TxData--)
                        {
                            *ptr++ = TxBuffer[i++];
                        }

                        apsStatus.flags.bits.bFramesAwaitingTransmission = 1;
                        ZigBeeUnblockTx();
                        return NO_PRIMITIVE;

                    #endif
                }
                break;

            case APSME_BIND_request:
                #ifndef I_SUPPORT_BINDINGS
                    params.APSME_BIND_confirm.Status = BIND_NOT_SUPPORTED;
                    return APSME_BIND_confirm;
                #else
                    // NOTE - The spec allows bindings to be created even if we are not
                    // associated.  However, it doesn't allow us to unbind...
                    {
                        SHORT_ADDR  srcShortAddress;
                        SHORT_ADDR  dstShortAddress;

                        // TODO - How are we supposed to know the short addresses?  Neighbor table,
                        // APS address map, device discovery,...?  Question submitted to ZigBee Alliance.
                        // We'll use the neighbor table for now.
                        if (NWKLookupNodeByLongAddr( &params.APSME_BIND_request.SrcAddr ) == INVALID_NEIGHBOR_KEY)
                        {
                            params.APSME_BIND_confirm.Status = BIND_ILLEGAL_DEVICE;
                            return APSME_BIND_confirm;
                        }
                        else
                        {
                            srcShortAddress = currentNeighborRecord.shortAddr;
                        }

                        if (NWKLookupNodeByLongAddr( &params.APSME_BIND_request.DstAddr ) == INVALID_NEIGHBOR_KEY)
                        {
                            params.APSME_BIND_confirm.Status = BIND_ILLEGAL_DEVICE;
                            return APSME_BIND_confirm;
                        }
                        else
                        {
                            dstShortAddress = currentNeighborRecord.shortAddr;
                        }

                        params.APSME_BIND_confirm.Status = APSAddBindingInfo( srcShortAddress,
                            params.APSME_BIND_request.SrcEndpoint, params.APSME_BIND_request.ClusterId,
                            dstShortAddress, params.APSME_BIND_request.DstEndpoint );
                        return APSME_BIND_confirm;
                    }
                #endif
                break;

            case APSME_UNBIND_request:
                #ifndef I_SUPPORT_BINDINGS
                    // NOTE - This is a deviation from the spec.  The spec does not specify
                    // what to do with this primitive on an end device, only the Bind Request.
                    params.APSME_UNBIND_confirm.Status = BIND_NOT_SUPPORTED;
                    return APSME_UNBIND_confirm;
                #else
                    // NOTE - The spec allows bindings to be created even if we are not
                    // associated.  However, it doesn't allow us to unbind...
                    #ifdef I_AM_COORDINATOR
                    if (!ZigBeeStatus.flags.bits.bNetworkFormed)
                    #else
                    if (!ZigBeeStatus.flags.bits.bNetworkJoined)
                    #endif
                    //if (macPIB.macPANId.Val == 0xFFFF)
                    {
                        params.APSME_UNBIND_confirm.Status = BIND_ILLEGAL_REQUEST;
                        return APSME_UNBIND_confirm;
                    }

                    {
                        SHORT_ADDR  srcShortAddress;
                        SHORT_ADDR  dstShortAddress;

                        // TODO - How are we supposed to know the short addresses?  Neighbor table,
                        // APS address map, device discovery,...?  Question submitted to ZigBee Alliance.
                        // We'll use the neighbor table for now.
                        if (NWKLookupNodeByLongAddr( &params.APSME_UNBIND_request.SrcAddr ) == INVALID_NEIGHBOR_KEY)
                        {
                            params.APSME_UNBIND_confirm.Status = BIND_ILLEGAL_DEVICE;
                            return APSME_UNBIND_confirm;
                        }
                        else
                        {
                            srcShortAddress = currentNeighborRecord.shortAddr;
                        }

                        if (NWKLookupNodeByLongAddr( &params.APSME_UNBIND_request.DstAddr ) == INVALID_NEIGHBOR_KEY)
                        {
                            params.APSME_UNBIND_confirm.Status = BIND_ILLEGAL_DEVICE;
                            return APSME_UNBIND_confirm;
                        }
                        else
                        {
                            dstShortAddress = currentNeighborRecord.shortAddr;
                        }

                        params.APSME_UNBIND_confirm.Status = APSRemoveBindingInfo( srcShortAddress,
                            params.APSME_UNBIND_request.SrcEndpoint, params.APSME_UNBIND_request.ClusterId,
                            dstShortAddress, params.APSME_UNBIND_request.DstEndpoint );
                        return APSME_UNBIND_confirm;
                    }
                #endif
                break;

        }
    }

    return NO_PRIMITIVE;
}



/*********************************************************************
 * Function:        BYTE APSAddBindingInfo(SHORT_ADDR srcAddr,
 *                                      BYTE srcEP,
 *                                      BYTE clusterID,
 *                                      SHORT_ADDR destAddr,
 *                                      BYTE destEP)
 *
 *
 * PreCondition:    srcAddr and destAddr must be valid addresses of
 *                  devices on the network
 *
 * Input:           srcAddr     - source short address
 *                  srcEP       - source end point
 *                  clusterID   - cluster id
 *                  destAddr    - destination short address
 *                  destEP      - destination end point
 *
 * Output:          SUCCESS if an entry was created or already exists
 *                  TABLE_FULL if the binding table is full
 *                  FALSE if illegal binding attempted
 *
 * Side Effects:    None
 *
 * Overview:        Creates/updates a binding entry for given
 *                  set of data.
 *
 * Note:            None
 ********************************************************************/
#if defined(I_SUPPORT_BINDINGS)
BYTE APSAddBindingInfo( SHORT_ADDR srcAddr, BYTE srcEP, BYTE clusterID,
                     SHORT_ADDR destAddr, BYTE destEP )
{
    BYTE            bindingDestIndex    = 0;
    BYTE            bindingSrcIndex     = 0;
    BYTE            bindingMapSourceByte;
    BYTE            bindingMapUsageByte;
    BOOL            Found           = FALSE;
    BYTE            oldBindingLink;
    BINDING_RECORD  tempBindingRecord;

    // See if a list for the source data already exists.
    while ((bindingSrcIndex < MAX_BINDINGS) && !Found)
    {
        GetBindingSourceMap( &bindingMapSourceByte, bindingSrcIndex );
        GetBindingUsageMap( &bindingMapUsageByte, bindingSrcIndex );

        if (BindingIsUsed( bindingMapUsageByte,  bindingSrcIndex ) &&
            BindingIsUsed( bindingMapSourceByte, bindingSrcIndex ))
        {
            pCurrentBindingRecord = &apsBindingTable[bindingSrcIndex];
            GetBindingRecord(&currentBindingRecord, (ROM void*)pCurrentBindingRecord );

            if ((currentBindingRecord.shortAddr.Val == srcAddr.Val) &&
                (currentBindingRecord.endPoint == srcEP) &&
                (currentBindingRecord.clusterID == clusterID))
            {
                Found = TRUE;
                break;
            }
        }
        bindingSrcIndex ++;
    }

    // If there was no source data list, create one.
    if (!Found)
    {
        bindingSrcIndex = 0;
        GetBindingUsageMap( &bindingMapUsageByte, bindingSrcIndex );
        while ((bindingSrcIndex < MAX_BINDINGS) &&
               BindingIsUsed( bindingMapUsageByte,  bindingSrcIndex ))
        {
            bindingSrcIndex ++;
            GetBindingUsageMap( &bindingMapUsageByte, bindingSrcIndex );
        }
        if (bindingSrcIndex == MAX_BINDINGS)
        {
            return BIND_TABLE_FULL;
        }

        pCurrentBindingRecord = &apsBindingTable[bindingSrcIndex];
        currentBindingRecord.shortAddr = srcAddr;
        currentBindingRecord.endPoint = srcEP;
        currentBindingRecord.clusterID = clusterID;
        currentBindingRecord.nextBindingRecord = END_BINDING_RECORDS;
    }
    // If we found the source data, make sure the destination link
    // doesn't already exist.  Leave currentBindingRecord as the source node.
    else
    {
        tempBindingRecord.nextBindingRecord = currentBindingRecord.nextBindingRecord;
        while (tempBindingRecord.nextBindingRecord != END_BINDING_RECORDS)
        {
            GetBindingRecord(&tempBindingRecord, (ROM void*)&apsBindingTable[tempBindingRecord.nextBindingRecord] );
            if ((tempBindingRecord.shortAddr.Val == destAddr.Val) &&
                (tempBindingRecord.endPoint == destEP))
                return SUCCESS;  // already exists
        }
    }

    // Make sure there is room for a new destination node. Make sure we avoid the
    // node that we're trying to use for the source in case it's a new one!
    bindingDestIndex = 0;
    GetBindingUsageMap( &bindingMapUsageByte, bindingDestIndex );
    while (((bindingDestIndex < MAX_BINDINGS) &&
            BindingIsUsed( bindingMapUsageByte,  bindingDestIndex )) ||
           (bindingDestIndex == bindingSrcIndex))
    {
        bindingDestIndex ++;
        GetBindingUsageMap( &bindingMapUsageByte, bindingDestIndex );
    }
    if (bindingDestIndex == MAX_BINDINGS)
    {
        return BIND_TABLE_FULL;
    }

    // Update the source node to point to the new destination node.
    oldBindingLink = currentBindingRecord.nextBindingRecord;
    currentBindingRecord.nextBindingRecord = bindingDestIndex;
    PutBindingRecord((NVM_ADDR*)pCurrentBindingRecord, (BYTE*)&currentBindingRecord );

    // Create the new binding record, inserting it at the head of the destinations.
    currentBindingRecord.shortAddr = destAddr;
    currentBindingRecord.endPoint = destEP;
    currentBindingRecord.nextBindingRecord = oldBindingLink;
    pCurrentBindingRecord = &apsBindingTable[bindingDestIndex];
    PutBindingRecord((NVM_ADDR*)pCurrentBindingRecord, (BYTE*)&currentBindingRecord );

    // Mark the source node as used.  Is redundant if it already existed, but if there
    // was room for the source but not the destination, we don't want to take the
    // source node.
    GetBindingUsageMap( &bindingMapUsageByte, bindingSrcIndex );
    MarkBindingUsed( bindingMapUsageByte, bindingSrcIndex );
    PutBindingUsageMap( &bindingMapUsageByte, bindingSrcIndex );
    GetBindingSourceMap( &bindingMapSourceByte, bindingSrcIndex );
    MarkBindingUsed( bindingMapSourceByte, bindingSrcIndex );
    PutBindingSourceMap( &bindingMapSourceByte, bindingSrcIndex );

    // Mark the destination node as used, but not a source node
    GetBindingUsageMap( &bindingMapUsageByte, bindingDestIndex );
    MarkBindingUsed( bindingMapUsageByte, bindingDestIndex );
    PutBindingUsageMap( &bindingMapUsageByte, bindingDestIndex );
    GetBindingSourceMap( &bindingMapSourceByte, bindingDestIndex );
    MarkBindingUnused( bindingMapSourceByte, bindingDestIndex );
    PutBindingSourceMap( &bindingMapSourceByte, bindingDestIndex );

    return SUCCESS;
}
#endif


/*********************************************************************
 * Function:        void APSClearAPSAddressTable( void )
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    currentAPSAddress is destroyed
 *
 * Overview:        This function sets the entire APS address map table
 *                  to all 0xFF.
 *
 * Note:            None
 ********************************************************************/
#if MAX_APS_ADDRESSES > 0
void APSClearAPSAddressTable( void )
{
    BYTE    i;

    memset( &currentAPSAddress, 0xFF, sizeof(APS_ADDRESS_MAP) );
    for (i=0; i<MAX_APS_ADDRESSES; i++)
    {
        PutAPSAddress( &(apsAddressMap[i]), &currentAPSAddress );
    }
}
#endif

/*********************************************************************
 * Function:        BYTE APSGet( void )
 *
 * PreCondition:    Must be called from the NLDE_DATA_indication
 *                  primitive.
 *
 * Input:           none
 *
 * Output:          One byte from the msdu if the length is greater
 *                  than 0; otherwise, 0.
 *
 * Side Effects:    The msdu pointer is incremented to point to the
 *                  next byte, and msduLength is decremented.
 *
 * Overview:        This function returns the next byte from the
 *                  msdu.
 *
 * Note:            None
 ********************************************************************/

#if 1

BYTE APSGet( void )
{
    if (params.NLDE_DATA_indication.NsduLength == 0)
    {
        return 0;
    }
    else
    {
        params.NLDE_DATA_indication.NsduLength--;
        return *params.NLDE_DATA_indication.Nsdu++;
    }
}

#else

BYTE APSGet( void )
{
    if (params.NLDE_DATA_indication.NsduLength == 0)
    {
        return 0;
    }
    else
    {
	    BYTE byteReturn;
        params.NLDE_DATA_indication.NsduLength--;
        byteReturn = *params.NLDE_DATA_indication.Nsdu++;
        TRACE_CHAR(byteReturn);
        TRACE( "" );
        return  byteReturn;
    }
}

#endif

/*********************************************************************
 * Function:        BYTE APSRemoveBindingInfo(SHORT_ADDR srcAddr,
 *                                      BYTE srcEP,
 *                                      BYTE clusterID,
 *                                      SHORT_ADDR destAddr,
 *                                      BYTE destEP)
 *
 *
 * PreCondition:    srcAddr and destAddr must be valid addresses of
 *                  devices on the network
 *
 * Input:           srcAddr     - source short address
 *                  srcEP       - source end point
 *                  clusterID   - cluster id
 *                  destAddr    - destination short address
 *                  destEP      - destination EP
 *
 * Output:          SUCCESS if the entry was removed
 *                  INVALID_BINDING if the binding did not exist
 *
 * Side Effects:    None
 *
 * Overview:        Removes a binding entry for given set of data.
 *
 * Note:            None
 ********************************************************************/
#if defined(I_SUPPORT_BINDINGS)
BYTE APSRemoveBindingInfo( SHORT_ADDR srcAddr, BYTE srcEP, BYTE clusterID,
                    SHORT_ADDR destAddr, BYTE destEP )
{
    BYTE    bindingMapUsageByte;
    BYTE    currentKey;
    BYTE    nextKey;
    BYTE    previousKey;
    BYTE    sourceKey;

    if ((sourceKey = LookupSourceBindingInfo( srcAddr, srcEP, clusterID)) ==
            END_BINDING_RECORDS)
    {
        return BIND_INVALID_BINDING;
    }

    previousKey = sourceKey;
    pCurrentBindingRecord = &apsBindingTable[previousKey];
    GetBindingRecord(&currentBindingRecord, (ROM void*)pCurrentBindingRecord );
    while (currentBindingRecord.nextBindingRecord != END_BINDING_RECORDS)
    {
        currentKey = currentBindingRecord.nextBindingRecord;
        pCurrentBindingRecord = &apsBindingTable[currentKey];
        GetBindingRecord(&currentBindingRecord, (ROM void*)pCurrentBindingRecord );
        if ((currentBindingRecord.shortAddr.Val == destAddr.Val) &&
                (currentBindingRecord.endPoint == destEP))
        {
            nextKey = currentBindingRecord.nextBindingRecord;

            // Go back and get the previous record, and point it to the record
            // that the deleted record was pointing to.
            pCurrentBindingRecord = &apsBindingTable[previousKey];
            GetBindingRecord(&currentBindingRecord, (ROM void*)pCurrentBindingRecord );
            currentBindingRecord.nextBindingRecord = nextKey;
            PutBindingRecord((NVM_ADDR*)pCurrentBindingRecord, (BYTE*)&currentBindingRecord);

            // Update the usage map to delete the current node.
            GetBindingUsageMap( &bindingMapUsageByte, currentKey );
            MarkBindingUnused( bindingMapUsageByte, currentKey );
            PutBindingUsageMap( &bindingMapUsageByte, currentKey );

            // If we just deleted the only destination, delete the source as well.
            if ((sourceKey == previousKey) && (nextKey == END_BINDING_RECORDS))
            {
                GetBindingUsageMap( &bindingMapUsageByte, sourceKey );
                MarkBindingUnused( bindingMapUsageByte, sourceKey );
                PutBindingUsageMap( &bindingMapUsageByte, sourceKey );
            }

            return SUCCESS;
         }
         else
         {
             previousKey = currentKey;
         }
     }
     return BIND_INVALID_BINDING;
}
#endif

/*********************************************************************
 * Function:        BYTE LookupAPSAddress( LONG_ADDR *longAddr )
 *
 * PreCondition:    None
 *
 * Input:           longAddr - pointer to long address to match
 *
 * Output:          TRUE - a corresponding short address was found and
 *                      is held in currentAPSAddress.shortAddr
 *                  FALSE - a corresponding short address was not found
 *
 * Side Effects:    currentAPSAddress is destroyed and set to the
 *                  matching entry if found
 *
 * Overview:        Searches the APS address map for the short address
 *                  of a given long address.
 *
 * Note:            The end application is responsible for populating
 *                  this table.
 ********************************************************************/
BOOL LookupAPSAddress( LONG_ADDR *longAddr )
{
#if apscMaxAddrMapEntries > 0
    BYTE    i;

    for (i=0; i<apscMaxAddrMapEntries; i++)
    {
        GetAPSAddress( &currentAPSAddress,  &apsAddressMap[i] );
        if (currentAPSAddress.shortAddr.Val != 0xFFFF)
        {
            if ( !memcmp((void*)longAddr, (void*)&currentAPSAddress.longAddr, (size_t)(sizeof(LONG_ADDR))) )
            {
                return TRUE;
            }
        }
    }
#endif
    return FALSE;
}
/*********************************************************************
 * Function:        BYTE LookupSourceBindingInfo( SHORT_ADDR srcAddr,
 *                                          BYTE srcEP,
 *                                          BYTE clusterID)
 *
 * PreCondition:    None
 *
 * Input:           srcAddr     - short address of source node
 *                  srcEP       - source end point
 *                  clusterID   - cluster id
 *
 * Output:          key to the source binding record if matching record found
 *                  END_BINDING_RECORDS otherwise
 *
 * Side Effects:    pCurrentBindingRecord and currentBindingRecord
 *                  are set to the source binding record
 *
 * Overview:        Searches binding table for matching source binding.
 *
 * Note:            None
 ********************************************************************/
#if defined(I_SUPPORT_BINDINGS)
BYTE LookupSourceBindingInfo( SHORT_ADDR srcAddr, BYTE srcEP, BYTE clusterID )
{
    BYTE            bindingIndex    = 0;
    BYTE            bindingMapSourceByte;
    BYTE            bindingMapUsageByte;

    while (bindingIndex < MAX_BINDINGS)
    {
        GetBindingSourceMap( &bindingMapSourceByte, bindingIndex );
        GetBindingUsageMap( &bindingMapUsageByte, bindingIndex );

        if (BindingIsUsed( bindingMapUsageByte,  bindingIndex ) &&
            BindingIsUsed( bindingMapSourceByte, bindingIndex ))
        {
            pCurrentBindingRecord = &apsBindingTable[bindingIndex];
            GetBindingRecord(&currentBindingRecord, (ROM void*)pCurrentBindingRecord );

            // If we find the matching source, we're done
            if ((currentBindingRecord.shortAddr.Val == srcAddr.Val) &&
                (currentBindingRecord.endPoint == srcEP) &&
                (currentBindingRecord.clusterID == clusterID))
            {
                return bindingIndex;
            }
        }
        bindingIndex ++;
    }

    // We didn't find a match, so return an error condition
    return END_BINDING_RECORDS;
}
#endif

/*********************************************************************
 * Function:        void RemoveAllBindings(SHORT_ADDR shortAddr)
 *
 * PreCondition:    none
 *
 * Input:           shortAddr - Address of a node on the network
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Searches binding table and removes all entries
 *                  associated with the address shortAddr.  Nodes are
 *                  removed by clearing the usage flag in the usage
 *                  map and fixing up any destination links.  If the
 *                  node is a source node, then all associated
 *                  destination nodes are also removed.
 *
 * Note:            None
 ********************************************************************/
#if defined(I_SUPPORT_BINDINGS)
void RemoveAllBindings(SHORT_ADDR shortAddr)
{
    BYTE                bindingIndex    = 0;
    BYTE                bindingMapSourceByte;
    BYTE                bindingMapUsageByte;
    BINDING_RECORD      nextRecord;
    ROM BINDING_RECORD  *pPreviousRecord;

    while (bindingIndex < MAX_BINDINGS)
    {
        // Get the maps for each check, in case we overwrote them removing other entries
        GetBindingSourceMap( &bindingMapSourceByte, bindingIndex );
        GetBindingUsageMap( &bindingMapUsageByte, bindingIndex );

        if (BindingIsUsed( bindingMapUsageByte,  bindingIndex ) &&
            BindingIsUsed( bindingMapSourceByte, bindingIndex ))
        {
            // Read the source node record into RAM.
            pCurrentBindingRecord = &apsBindingTable[bindingIndex];
            GetBindingRecord(&currentBindingRecord, (ROM void*)pCurrentBindingRecord );

            // See if the source node address matches the address we are trying to find
            if (currentBindingRecord.shortAddr.Val == shortAddr.Val)
            {
                // Remove this source node and all destination nodes
                MarkBindingUnused( bindingMapUsageByte, bindingIndex );
                PutBindingUsageMap( &bindingMapUsageByte, bindingIndex );
                MarkBindingUnused( bindingMapSourceByte, bindingIndex );
                PutBindingSourceMap( &bindingMapSourceByte, bindingIndex );
                while (currentBindingRecord.nextBindingRecord != END_BINDING_RECORDS)
                {
                    // Read the destination node record into RAM.
                    GetBindingUsageMap( &bindingMapUsageByte, currentBindingRecord.nextBindingRecord );
                    MarkBindingUnused( bindingMapUsageByte, currentBindingRecord.nextBindingRecord );
                    PutBindingUsageMap( &bindingMapUsageByte, currentBindingRecord.nextBindingRecord );
                    GetBindingRecord(&currentBindingRecord, (ROM void*)&apsBindingTable[currentBindingRecord.nextBindingRecord] );
                }
            }
            else
            {
                pPreviousRecord = &apsBindingTable[bindingIndex];

                // See if any of the destination nodes in this list match the address
                while (currentBindingRecord.nextBindingRecord != END_BINDING_RECORDS)
                {
                    // Read the destination node record into RAM.
                    pCurrentBindingRecord = &apsBindingTable[currentBindingRecord.nextBindingRecord];
                    GetBindingRecord(&nextRecord, (ROM void*)pCurrentBindingRecord );
                    if (nextRecord.shortAddr.Val == shortAddr.Val)
                    {
                        // Remove the destination node and patch up the list
                        GetBindingUsageMap( &bindingMapUsageByte, currentBindingRecord.nextBindingRecord );
                        MarkBindingUnused( bindingMapUsageByte, currentBindingRecord.nextBindingRecord );
                        PutBindingUsageMap( &bindingMapUsageByte, currentBindingRecord.nextBindingRecord );
                        currentBindingRecord.nextBindingRecord = nextRecord.nextBindingRecord;
                        PutBindingRecord((NVM_ADDR*)pPreviousRecord, (BYTE*)&currentBindingRecord );
                    }
                    else
                    {
                        // Read the next destination node record into RAM.
                        pPreviousRecord = pCurrentBindingRecord;
                        pCurrentBindingRecord = &apsBindingTable[currentBindingRecord.nextBindingRecord];
                        GetBindingRecord(&currentBindingRecord, (ROM void*)pCurrentBindingRecord );
                    }
                }

                // If we just deleted the only destination, delete the source as well.
                // Read the source node record into RAM.
                pCurrentBindingRecord = &apsBindingTable[bindingIndex];
                GetBindingRecord( &currentBindingRecord, (ROM void*)pCurrentBindingRecord );
                if (currentBindingRecord.nextBindingRecord == END_BINDING_RECORDS)
                {
                    GetBindingUsageMap( &bindingMapUsageByte, bindingIndex );
                    MarkBindingUnused( bindingMapUsageByte, bindingIndex );
                    PutBindingUsageMap( &bindingMapUsageByte, bindingIndex );
                    GetBindingSourceMap( &bindingMapSourceByte, bindingIndex );
                    MarkBindingUnused( bindingMapSourceByte, bindingIndex );
                    PutBindingSourceMap( &bindingMapSourceByte, bindingIndex );
                }
            }
        }
        bindingIndex ++;
    }
}
#endif


