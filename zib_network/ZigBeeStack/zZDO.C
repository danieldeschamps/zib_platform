/*********************************************************************
 *
 *                  ZigBee ZDO Layer
 *
 *********************************************************************
 * FileName:        zZDO.c
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

// TODO fix zdo processing to handle transaction count and sequence number!!!



#include "ZigBeeTasks.h"
#include "sralloc.h"
#include "zNVM.h"

#include "zMAC.h"
#include "zNWK.h"
#include "zAPS.h"
#include "zZDO.h"
#include "zAPL.h"

//#include "console.h"

// ******************************************************************************
// Constant Definitions

#define MAX_APS_PACKET_SIZE                 (127-11-8-6-2)  // Max size - MAC, NWK, APS, and AF headers
#define MAX_LONGADDS_TO_SEND                ((MAX_APS_PACKET_SIZE-13)/8)
#define MAX_SHORTADDS_TO_SEND               ((MAX_APS_PACKET_SIZE-13)/2)
#define MSG_HEADER_SIZE                     3

#define BIND_SOURCE_MASK            0x02
#define BIND_FROM_UPPER_LAYERS      0x02
#define UNBIND_FROM_UPPER_LAYERS    0x02
#define BIND_FROM_EXTERNAL          0x00
#define UNBIND_FROM_EXTERNAL        0x00

#define BIND_DIRECTION_MASK         0x01
#define BIND_NOT_DETERMINED         0x02    // NOTE - does not need mask
#define BIND_NODES                  0x01
#define UNBIND_NODES                0x00

// ******************************************************************************
// Macros

// Since the APSDE_DATA_indication and ZDO_DATA_indication parameters align,
// we can use the APSGet function.
#define ZDOGet()        APSGet()
#define ZDODiscardRx()  APSDiscardRx()

// ******************************************************************************
// Data Structures

#if defined(SUPPORT_END_DEVICE_BINDING)
typedef struct _END_DEVICE_BIND_INFO
{
    TICK        lastTick;
    SHORT_ADDR  shortAddr;
    WORD_VAL    profileID;
    BYTE        *inClusterList;
    BYTE        *outClusterList;
    BYTE        sequenceNumber;
//    BYTE        sourceEP; will always be EP0
    BYTE        bindEP;
    BYTE        numInClusters;
    BYTE        numOutClusters;
    union
    {
        struct
        {
            unsigned int    fResponse       : 4;
            unsigned int    bFromSelf       : 1;
            unsigned int    bSendResponse   : 1;
        } bits;
        BYTE    Val;
    } flags;
} END_DEVICE_BIND_INFO;
#endif


typedef struct _ZDO_STATUS
{
    union _ZDO_STATUS_flags
    {
        BYTE    Val;
        struct _ZDO_STATUS_bits
        {
            // Background Task Flags
            BYTE    bEndDeviceBinding   : 1;

            // Status Flags
        } bits;
    } flags;
} ZDO_STATUS;
#define ZDO_BACKGROUND_TASKS    0x01


// ******************************************************************************
// Variable Definitions

#ifdef SUPPORT_END_DEVICE_BINDING
    static END_DEVICE_BIND_INFO *pFirstEndBindRequest;
#endif
BYTE        sequenceNumber;            // Received sequence number, for sending response back
//BYTE        sourceEP; should always be EP0
ZDO_STATUS  zdoStatus;

// ******************************************************************************
// Function Prototypes

void FinishAddressResponses( BYTE clusterID );
BOOL IsThisMyShortAddr( void );
void PrepareMessageResponse( BYTE clusterID );

#if defined(I_AM_COORDINATOR) || defined(I_AM_ROUTER)
    BOOL ProcessBindAndUnbind( BYTE bindInfo, LONG_ADDR *sourceAddress, LONG_ADDR *destinationAddress );
#endif

#ifdef SUPPORT_END_DEVICE_BINDING
    ZIGBEE_PRIMITIVE ProcessEndDeviceBind( END_DEVICE_BIND_INFO *pRequestInfo );
    ZIGBEE_PRIMITIVE Send_END_DEVICE_BIND_rsp( END_DEVICE_BIND_INFO *pBindRequest, BYTE status );
#endif

/*********************************************************************
 * Function:        BOOL ZDOHasBackgroundTasks( void )
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          TRUE - ZDO layer has background tasks to run
 *                  FALSE - ZDO layer does not have background tasks
 *
 * Side Effects:    None
 *
 * Overview:        Determines if the ZDO layer has background tasks
 *                  that need to be run.
 *
 * Note:            None
 ********************************************************************/

BOOL ZDOHasBackgroundTasks( void )
{
    return ((zdoStatus.flags.Val & ZDO_BACKGROUND_TASKS) != 0);
}

/*********************************************************************
 * Function:        void ZDOInit( void )
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    ZDO layer data structures are initialized.
 *
 * Overview:        This routine initializes all ZDO layer data
 *                  structures.
 *
 * Note:            This routine is intended to be called as part of
 *                  a network or power-up initialization.  If called
 *                  after the network has been running, heap space
 *                  may be lost unless the heap is also reinitialized.
 *                  End device binding in progress will be lost.
 ********************************************************************/

void ZDOInit( void )
{
    zdoStatus.flags.Val = 0;

    #ifdef SUPPORT_END_DEVICE_BINDING
        pFirstEndBindRequest  = NULL;
    #endif
}

/*********************************************************************
 * Function:        ZIGBEE_PRIMITIVE ZDOTasks(ZIGBEE_PRIMITIVE inputPrimitive)
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
 * Note:            This routine may be called while the TX path is blocked.
 *                  Therefore, we must check the Tx path before doing any
 *                  processing that may generate a message.
 *                  It is the responsibility of this task to ensure that
 *                  only one output primitive is generated by any path.
 *                  If multiple output primitives are generated, they
 *                  must be generated one at a time by background processing.
 ********************************************************************/

ZIGBEE_PRIMITIVE ZDOTasks(ZIGBEE_PRIMITIVE inputPrimitive)
{
    ZIGBEE_PRIMITIVE    nextPrimitive;


    nextPrimitive = NO_PRIMITIVE;

    // Manage primitive- and Tx-independent tasks here.  These tasks CANNOT
    // produce a primitive or send a message.


    // Handle other tasks and primitives that may require a message to be sent.
    if (inputPrimitive == NO_PRIMITIVE)
    {
        // If Tx is blocked, we cannot generate a message or send back another primitive.
        if (!ZigBeeReady())
            return NO_PRIMITIVE;

        // Manage background tasks here

        // ---------------------------------------------------------------------
        // Handle pending end device bind request
        #ifdef SUPPORT_END_DEVICE_BINDING
            if (zdoStatus.flags.bits.bEndDeviceBinding)
            {
                if (pFirstEndBindRequest)
                {
                    // NOTE: Compiler SSR27744, TickGet() output must be assigned to a variable.
                    TICK tempTick;

                    tempTick = TickGet();
                    if ( !pFirstEndBindRequest->flags.bits.bSendResponse  &&
                        (TickGetDiff(tempTick, pFirstEndBindRequest->lastTick) >= CONFIG_ENDDEV_BIND_TIMEOUT) )
//                    if ( !pFirstEndBindRequest->flags.bits.bSendResponse  &&
//                        (TickGetDiff(TickGet(), pFirstEndBindRequest->lastTick) >= CONFIG_ENDDEV_BIND_TIMEOUT) )
                    {
                        pFirstEndBindRequest->flags.bits.fResponse = ZDO_TIMEOUT;
                        pFirstEndBindRequest->flags.bits.bSendResponse = 1;
                    }
                    if (pFirstEndBindRequest->flags.bits.bSendResponse)
                    {
                        nextPrimitive = Send_END_DEVICE_BIND_rsp( pFirstEndBindRequest, pFirstEndBindRequest->flags.bits.fResponse );
                        free( pFirstEndBindRequest );
                        zdoStatus.flags.bits.bEndDeviceBinding = 0;
                        return nextPrimitive;
                    }
                }
                else
                {
                    zdoStatus.flags.bits.bEndDeviceBinding = 0;
                }
            }
        #endif


    }
    else
    {
        switch( inputPrimitive )
        {
            case ZDO_DATA_indication:
                {
                    BYTE        dataLength;
                    BYTE        frameHeader;

                    // TODO Limitation - the AF command frame allows more than one transaction to be included
                    // in a frame, but we can only generate one response.  Therefore, we will support only
                    // one transaction per ZDO frame.
                    if ((params.ZDO_DATA_indication.ClusterId & 0x80) != 0x00)
                    {
                        // These are ZDO responses that the application has requested.
                        // Send them back to the user.  The parameters are all in place.
                        return APSDE_DATA_indication;
                    }

                    frameHeader = ZDOGet();
                    if ((frameHeader & APL_FRAME_TYPE_MASK) != APL_FRAME_TYPE_MSG)
                    {
                        ZDODiscardRx();
                        break;
                    }

                    // should always be EP0 sourceEP                = params.ZDO_DATA_indication.SrcEndpoint;
                    sequenceNumber          = ZDOGet();
                    dataLength              = ZDOGet();

                    switch (params.ZDO_DATA_indication.ClusterId)
                    {
                        case NWK_ADDR_req:
                            {
                                BYTE    i;
                                BYTE    tempMACAddrByte;
                                BYTE    oneByte;

                                // Get the first parameter (IEEEAddr) and make sure that it is our address
                                for (i=0; i<8; i++)
                                {
                                    GetMACAddressByte( i, &tempMACAddrByte );
                                    oneByte = ZDOGet();
                                    if (tempMACAddrByte != oneByte)
                                    {
                                        goto Finished_NWK_ADDR_req;
                                    }
                                }
                                FinishAddressResponses( NWK_ADDR_rsp );

                                // This request is broadcast, so we must request an APS ACK on the response.
                                params.APSDE_DATA_request.TxOptions.bits.acknowledged = 1;
                                nextPrimitive = APSDE_DATA_request;

Finished_NWK_ADDR_req:
                                ;
                            }
                            break;

                        case IEEE_ADDR_req:
                            if (IsThisMyShortAddr())
                            {
                                FinishAddressResponses( IEEE_ADDR_rsp );
                                nextPrimitive = APSDE_DATA_request;
                            }
                            break;

                        case NODE_DESC_req:
                            // Get NWKAddrOfInterest and make sure that it is ours
                            if (IsThisMyShortAddr())
                            {
                                // Skip over the header info
                                TxData = TX_DATA_START + MSG_HEADER_SIZE;

                                TxBuffer[TxData++] = SUCCESS;
                                TxBuffer[TxData++] = macPIB.macShortAddress.byte.LSB;
                                TxBuffer[TxData++] = macPIB.macShortAddress.byte.MSB;

                                ProfileGetNodeDesc( &TxBuffer[TxData] );
                                TxData += sizeof(NODE_DESCRIPTOR);

                                PrepareMessageResponse( NODE_DESC_rsp );
                                nextPrimitive = APSDE_DATA_request;
                            }
                            break;

                        case POWER_DESC_req:
                            // Get NWKAddrOfInterest and make sure that it is ours
                            if (IsThisMyShortAddr())
                            {
                                // Skip over the header info
                                TxData = TX_DATA_START + MSG_HEADER_SIZE;

                                TxBuffer[TxData++] = SUCCESS;
                                TxBuffer[TxData++] = macPIB.macShortAddress.byte.LSB;
                                TxBuffer[TxData++] = macPIB.macShortAddress.byte.MSB;

                                ProfileGetNodePowerDesc( &TxBuffer[TxData] );
                                TxData += sizeof(NODE_POWER_DESCRIPTOR);

                                PrepareMessageResponse( POWER_DESC_rsp );
                                nextPrimitive = APSDE_DATA_request;
                            }

                            break;

                        case SIMPLE_DESC_req:
                            // Get NWKAddrOfInterest and make sure that it is ours
                            if ( IsThisMyShortAddr())
                            {
                                BYTE                    endPoint;
                                BYTE                    i;
                                NODE_SIMPLE_DESCRIPTOR  simpleDescriptor;

                                endPoint = ZDOGet();

                                // Skip over the header info
                                TxData = TX_DATA_START + MSG_HEADER_SIZE;

                                if ((endPoint > 0) && (endPoint <= 240))
                                {
                                    // Find the simple descriptor of the desired endpoint.  Note that we cannot just shove this
                                    // into a message buffer, because it may have extra padding in the cluster lists.
                                    i = -1;
                                    do
                                    {
                                        i++;
                                        ProfileGetSimpleDesc( &simpleDescriptor, i );
                                    }
                                    while ((simpleDescriptor.Endpoint != endPoint) &&
                                           (i < NUM_DEFINED_ENDPOINTS));
                                    if (i == NUM_DEFINED_ENDPOINTS)
                                    {
                                        // Load result code
                                        TxBuffer[TxData++] = ZDO_NOT_ACTIVE;
                                        goto SendInactiveEndpoint;
                                    }

                                    // Load result code
                                    TxBuffer[TxData++] = SUCCESS;

                                    // Load our short address.
                                    TxBuffer[TxData++] = macPIB.macShortAddress.byte.LSB;
                                    TxBuffer[TxData++] = macPIB.macShortAddress.byte.MSB;

                                    // Load length of simple descriptor.
                                    TxBuffer[TxData++] = SIMPLE_DESCRIPTOR_BASE_SIZE +
                                            simpleDescriptor.AppInClusterCount + simpleDescriptor.AppOutClusterCount;

                                    // Load the descriptor. Since we can't send extra padding in the cluster lists,
                                    // we can't load the descriptor as one big array.
                                    TxBuffer[TxData++] = simpleDescriptor.Endpoint;

                                    TxBuffer[TxData++] = simpleDescriptor.AppProfId.byte.LSB;
                                    TxBuffer[TxData++] = simpleDescriptor.AppProfId.byte.MSB;
                                    TxBuffer[TxData++] = simpleDescriptor.AppDevId.byte.LSB;
                                    TxBuffer[TxData++] = simpleDescriptor.AppDevId.byte.MSB;

                                    TxBuffer[TxData++] = simpleDescriptor.AppDevVer | (simpleDescriptor.AppFlags << 4);

                                    TxBuffer[TxData++] = simpleDescriptor.AppInClusterCount;
                                    memcpy( (void *)&TxBuffer[TxData], (void *)simpleDescriptor.AppInClusterList, simpleDescriptor.AppInClusterCount );
                                    TxData += simpleDescriptor.AppInClusterCount * sizeof(BYTE);
                                    TxBuffer[TxData++] = simpleDescriptor.AppOutClusterCount;
                                    memcpy( (void *)&TxBuffer[TxData], (void *)simpleDescriptor.AppOutClusterList, simpleDescriptor.AppOutClusterCount );
                                    TxData += simpleDescriptor.AppOutClusterCount * sizeof(BYTE);

                                    PrepareMessageResponse( SIMPLE_DESC_rsp );
                                    nextPrimitive = APSDE_DATA_request;
                                }
                                else
                                {
                                    // Invalid or Inactive Endpoint

                                    // Load result code
                                    TxBuffer[TxData++] = ZDO_INVALID_EP;

SendInactiveEndpoint:
                                    // Load our short address.
                                    TxBuffer[TxData++] = macPIB.macShortAddress.byte.LSB;
                                    TxBuffer[TxData++] = macPIB.macShortAddress.byte.MSB;

                                    // Indicate no descriptor
                                    TxBuffer[TxData++] = 0;

                                    PrepareMessageResponse( SIMPLE_DESC_rsp );
                                    nextPrimitive = APSDE_DATA_request;
                                }
                            }
                            break;

                        case ACTIVE_EP_req:
                            // Get NWKAddrOfInterest and make sure that it is ours
                            if (IsThisMyShortAddr())
                            {
                                BYTE                    i;
                                NODE_SIMPLE_DESCRIPTOR  simpleDescriptor;
                                BYTE                    wasBroadcast;

                                // Save the broadcast indication for later.
                                wasBroadcast = params.ZDO_DATA_indication.WasBroadcast;

                                // Skip over the header info
                                TxData = TX_DATA_START + MSG_HEADER_SIZE;

                                // Load status
                                TxBuffer[TxData++] = SUCCESS;

                                // Load our short address.
                                TxBuffer[TxData++] = macPIB.macShortAddress.byte.LSB;
                                TxBuffer[TxData++] = macPIB.macShortAddress.byte.MSB;

                                // Load the endpoint count
                                TxBuffer[TxData++] = NUM_DEFINED_ENDPOINTS ;

                                for (i=0; i<NUM_DEFINED_ENDPOINTS; i++)
                                {
                                    ProfileGetSimpleDesc( &simpleDescriptor, i );
                                    TxBuffer[TxData++] = simpleDescriptor.Endpoint;
                                }

                                PrepareMessageResponse( ACTIVE_EP_rsp );

                                // This request may have been broadcast. If so, we must request an APS ACK on the response.
                                if (wasBroadcast)
                                {
                                    params.APSDE_DATA_request.TxOptions.bits.acknowledged = 1;
                                }
                                nextPrimitive = APSDE_DATA_request;
                            }
                            break;

                        case MATCH_DESC_req:
                            // Get NWKAddrOfInterest and make sure that it is ours
                            if (IsThisMyShortAddr())
                            {
                                BOOL                    checkProfileOnly;
                                BYTE                    descIndex;
                                BYTE                    i;
                                BYTE                    *inClusterList;
                                BYTE                    listIndex;
                                BOOL                    match;
                                BYTE                    numInClusters;
                                BYTE                    numMatchingEPs;
                                BYTE                    numOutClusters;
                                BYTE                    *outClusterList;
                                WORD_VAL                profileID;
                                NODE_SIMPLE_DESCRIPTOR  *simpleDescriptor;
                                BYTE                    wasBroadcast;

                                // Save the broadcast indication for later.
                                wasBroadcast = params.ZDO_DATA_indication.WasBroadcast;

                                checkProfileOnly   = FALSE;
                                inClusterList      = NULL;
                                numMatchingEPs     = 0;
                                outClusterList     = NULL;

                                if ((simpleDescriptor = (NODE_SIMPLE_DESCRIPTOR *)SRAMalloc( sizeof(NODE_SIMPLE_DESCRIPTOR) )) == NULL)
                                {
                                    goto BailFromMatchDesc;
                                }

                                profileID.byte.LSB = ZDOGet();
                                profileID.byte.MSB = ZDOGet();
                                numInClusters = ZDOGet();
                                if (numInClusters)
                                {
                                    if ((inClusterList = SRAMalloc( numInClusters )) == NULL)
                                    {
                                        goto BailFromMatchDesc;
                                    }
                                    for (i=0; i<numInClusters; i++)
                                    {
                                        inClusterList[i] = ZDOGet();
                                    }
                                }
                                numOutClusters = ZDOGet();
                                if (numOutClusters)
                                {
                                    if ((outClusterList = SRAMalloc( numOutClusters )) == NULL)
                                    {
                                        goto BailFromMatchDesc;
                                    }
                                    for (i=0; i<numOutClusters; i++)
                                    {
                                        outClusterList[i] = ZDOGet();
                                    }
                                }

                                if ((numInClusters == 0) && (numOutClusters == 0))
                                {
                                    checkProfileOnly = TRUE;
                                }

                                // Set the data pointer to the matching endpoint list in the response message.
                                TxData += 4 + MSG_HEADER_SIZE;

                                // See if any of the input clusters match any of our output clusters, or if
                                // any of the output clusters match any of our input clusters, in any of our
                                // endpoints with a given profile ID.  Do not check the ZDO endpoint (0).
                                for (i=1; i < NUM_DEFINED_ENDPOINTS; i++)
                                {
                                    match = FALSE;
                                    ProfileGetSimpleDesc( simpleDescriptor, i );
                                    if (simpleDescriptor->AppProfId.Val == profileID.Val)
                                    {
                                        if (checkProfileOnly)
                                        {
                                            match = TRUE;
                                        }
                                        else
                                        {
                                            for (descIndex=0; descIndex<simpleDescriptor->AppOutClusterCount; descIndex++)
                                            {
                                                for (listIndex=0; listIndex<numInClusters; listIndex++)
                                                {
                                                    if (inClusterList[listIndex] == simpleDescriptor->AppOutClusterList[descIndex])
                                                    {
                                                        match = TRUE;
                                                    }
                                                }
                                            }
                                            for (descIndex=0; descIndex<simpleDescriptor->AppInClusterCount; descIndex++)
                                            {
                                                for (listIndex=0; listIndex<numOutClusters; listIndex++)
                                                {
                                                    if (outClusterList[listIndex] == simpleDescriptor->AppInClusterList[descIndex])
                                                    {
                                                        match = TRUE;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    if (match)
                                    {
                                        TxBuffer[TxData++] = simpleDescriptor->Endpoint;
                                        numMatchingEPs++;
                                    }
                                }

                                // Load the response message and send it.  TxData is already set from
                                // above, so we'll use i to load up the initial part of the response.
                                // Skip over the header info.
                                i = TX_DATA_START + MSG_HEADER_SIZE;

                                // Load status
                                TxBuffer[i++] = SUCCESS;

                                // Load our short address.
                                TxBuffer[i++] = macPIB.macShortAddress.byte.LSB;
                                TxBuffer[i++] = macPIB.macShortAddress.byte.MSB;

                                // Load the number of matching endpoints. The endpoints themselves are already there.
                                TxBuffer[i++] = numMatchingEPs;

                                PrepareMessageResponse( MATCH_DESC_rsp );

                                // This request may have been broadcast. If so, we must request an APS ACK on the response.
                                if (wasBroadcast)
                                {
                                    params.APSDE_DATA_request.TxOptions.bits.acknowledged = 1;
                                }
                                nextPrimitive = APSDE_DATA_request;

BailFromMatchDesc:
                                if (inClusterList != NULL)
                                    free( inClusterList );
                                if (outClusterList != NULL)
                                    free( outClusterList );
                            }
                            break;

                        case COMPLEX_DESC_req:
                            // Get NWKAddrOfInterest and make sure that it is ours
                            if (IsThisMyShortAddr())
                            {
                                #ifdef INCLUDE_OPTIONAL_SERVICE_DISCOVERY_REQUESTS
                                    // TODO: Right now this is not supported at all.  When added,
                                    // put full functionality code here and change status.

                                    // Skip over the header info
                                    TxData = TX_DATA_START + MSG_HEADER_SIZE;

                                    TxBuffer[TxData++] = ZDO_NOT_SUPPORTED;

                                    // Load our short address.
                                    TxBuffer[TxData++] = macPIB.macShortAddress.byte.LSB;
                                    TxBuffer[TxData++] = macPIB.macShortAddress.byte.MSB;

                                    PrepareMessageResponse( COMPLEX_DESC_rsp );
                                    nextPrimitive = APSDE_DATA_request;
                                #else

                                    // Skip over the header info
                                    TxData = TX_DATA_START + MSG_HEADER_SIZE;

                                    TxBuffer[TxData++] = ZDO_NOT_SUPPORTED;

                                    // Load our short address.
                                    TxBuffer[TxData++] = macPIB.macShortAddress.byte.LSB;
                                    TxBuffer[TxData++] = macPIB.macShortAddress.byte.MSB;

                                    PrepareMessageResponse( COMPLEX_DESC_rsp );
                                    nextPrimitive = APSDE_DATA_request;
                                #endif

                            }
                            break;

                        case USER_DESC_req:
                            // Get NWKAddrOfInterest and make sure that it is ours
                            if (IsThisMyShortAddr())
                            {
                                #ifdef INCLUDE_OPTIONAL_SERVICE_DISCOVERY_REQUESTS
                                    // TODO: Right now this is not supported at all.  When added,
                                    // put full functionality code here and change status.

                                    // Skip over the header info
                                    TxData = TX_DATA_START + MSG_HEADER_SIZE;

                                    TxBuffer[TxData++] = ZDO_NOT_SUPPORTED;

                                    // Load our short address.
                                    TxBuffer[TxData++] = macPIB.macShortAddress.byte.LSB;
                                    TxBuffer[TxData++] = macPIB.macShortAddress.byte.MSB;

                                    PrepareMessageResponse( USER_DESC_rsp );
                                    nextPrimitive = APSDE_DATA_request;
                                #else

                                    // Skip over the header info
                                    TxData = TX_DATA_START + MSG_HEADER_SIZE;

                                    TxBuffer[TxData++] = ZDO_NOT_SUPPORTED;

                                    // Load our short address.
                                    TxBuffer[TxData++] = macPIB.macShortAddress.byte.LSB;
                                    TxBuffer[TxData++] = macPIB.macShortAddress.byte.MSB;

                                    PrepareMessageResponse( USER_DESC_rsp );
                                    nextPrimitive = APSDE_DATA_request;
                                #endif

                            }
                            break;

                        case DISCOVERY_REGISTER_req:
                            // Get NWKAddrOfInterest and make sure that it is ours
                            if (IsThisMyShortAddr())
                            {
                                #ifdef INCLUDE_OPTIONAL_SERVICE_DISCOVERY_REQUESTS
                                    // TODO: Right now this is not supported at all.  When added,
                                    // put full functionality code here and change status.

                                    // Skip over the header info
                                    TxData = TX_DATA_START + MSG_HEADER_SIZE;

                                    TxBuffer[TxData++] = ZDO_NOT_SUPPORTED;
                                    PrepareMessageResponse( DISCOVERY_REGISTER_rsp );
                                    nextPrimitive = APSDE_DATA_request;
                                #else

                                    // Skip over the header info
                                    TxData = TX_DATA_START + MSG_HEADER_SIZE;

                                    TxBuffer[TxData++] = ZDO_NOT_SUPPORTED;
                                    PrepareMessageResponse( DISCOVERY_REGISTER_rsp );
                                    nextPrimitive = APSDE_DATA_request;
                                #endif

                            }
                            break;

                        #ifdef INCLUDE_OPTIONAL_SERVICE_DISCOVERY_REQUESTS
                        case END_DEVICE_annce:
                            // TODO: Can add this sometime - support is optional
                            // Watch - this command can be broadcast.
                            break;
                        #endif

                        case USER_DESC_set:
                            // Get NWKAddrOfInterest and make sure that it is ours
                            if (IsThisMyShortAddr())
                            {
                                #ifdef INCLUDE_OPTIONAL_SERVICE_DISCOVERY_REQUESTS
                                    // TODO: Right now this is not supported at all.  When added,
                                    // put full functionality code here and change status.

                                    // Skip over the header info
                                    TxData = TX_DATA_START + MSG_HEADER_SIZE;

                                    TxBuffer[TxData++] = ZDO_NOT_SUPPORTED;
                                    PrepareMessageResponse( USER_DESC_conf );
                                    nextPrimitive = APSDE_DATA_request;
                                #else

                                    // Skip over the header info
                                    TxData = TX_DATA_START + MSG_HEADER_SIZE;

                                    TxBuffer[TxData++] = ZDO_NOT_SUPPORTED;
                                    PrepareMessageResponse( USER_DESC_conf );
                                    nextPrimitive = APSDE_DATA_request;
                                #endif

                            }
                            break;

                        #if defined(I_AM_COORDINATOR) || defined(I_AM_ROUTER)

                        #define BR_OFFSET_SOURCE_ADDRESS        0x00
                        #define BR_OFFSET_DESTINATION_ADDRESS   0x0a

                        #ifdef SUPPORT_END_DEVICE_BINDING
                        case END_DEVICE_BIND_req:
                            nextPrimitive = ProcessEndDeviceBind( NULL );
                            break;
                        #endif

                        case BIND_req:
                            // Skip over the header info
                            TxData = TX_DATA_START + MSG_HEADER_SIZE;

                            #if !defined(I_SUPPORT_BINDINGS)
                                TxBuffer[TxData++] = ZDO_NOT_SUPPORTED;
                            #else
                                ProcessBindAndUnbind( BIND_FROM_EXTERNAL | BIND_NODES,
                                    (LONG_ADDR *)&params.ZDO_DATA_indication.asdu[BR_OFFSET_SOURCE_ADDRESS],
                                    (LONG_ADDR *)&params.ZDO_DATA_indication.asdu[BR_OFFSET_DESTINATION_ADDRESS] );
                                TxBuffer[TxData++] = params.ZDO_BIND_req.Status;
                            #endif

                            PrepareMessageResponse( BIND_rsp );
                            nextPrimitive = APSDE_DATA_request;
                            break;

                        case UNBIND_req:
                            // Skip over the header info
                            TxData = TX_DATA_START + MSG_HEADER_SIZE;

                            #if MAX_BINDINGS == 0
                                TxBuffer[TxData++] = ZDO_NOT_SUPPORTED;
                            #else
                            {
                                ProcessBindAndUnbind( UNBIND_FROM_EXTERNAL | UNBIND_NODES,
                                    (LONG_ADDR *)&params.ZDO_DATA_indication.asdu[BR_OFFSET_SOURCE_ADDRESS],
                                    (LONG_ADDR *)&params.ZDO_DATA_indication.asdu[BR_OFFSET_DESTINATION_ADDRESS] );
                                TxBuffer[TxData++] = params.ZDO_UNBIND_req.Status;
                            }
                            #endif

                            PrepareMessageResponse( UNBIND_rsp );
                            nextPrimitive =  APSDE_DATA_request;
                            break;

                        #endif

                        case MGMT_NWK_DISC_req:
                            #ifdef INCLUDE_OPTIONAL_NODE_MANAGEMENT_SERVICES
                                // TODO: Right now this is not supported at all.  When added,
                                // put full functionality code here and change status.

                                // Skip over the header info
                                TxData = TX_DATA_START + MSG_HEADER_SIZE;

                                TxBuffer[TxData++] = ZDO_NOT_SUPPORTED;
                                PrepareMessageResponse( MGMT_NWK_DISC_rsp );
                                nextPrimitive = APSDE_DATA_request;
                            #else

                                // Skip over the header info
                                TxData = TX_DATA_START + MSG_HEADER_SIZE;

                                TxBuffer[TxData++] = ZDO_NOT_SUPPORTED;
                                PrepareMessageResponse( MGMT_NWK_DISC_rsp );
                                nextPrimitive = APSDE_DATA_request;
                            #endif
                            break;

                        case MGMT_LQI_req:
                            #ifdef INCLUDE_OPTIONAL_NODE_MANAGEMENT_SERVICES
                                // TODO: Right now this is not supported at all.  When added,
                                // put full functionality code here and change status.

                                // Skip over the header info
                                TxData = TX_DATA_START + MSG_HEADER_SIZE;

                                TxBuffer[TxData++] = ZDO_NOT_SUPPORTED;
                                PrepareMessageResponse( MGMT_LQI_rsp );
                                nextPrimitive = APSDE_DATA_request;
                            #else

                                // Skip over the header info
                                TxData = TX_DATA_START + MSG_HEADER_SIZE;

                                TxBuffer[TxData++] = ZDO_NOT_SUPPORTED;
                                PrepareMessageResponse( MGMT_LQI_rsp );
                                nextPrimitive = APSDE_DATA_request;
                            #endif
                            break;

                        case MGMT_RTG_req:
                            #ifdef INCLUDE_OPTIONAL_NODE_MANAGEMENT_SERVICES
                                // TODO: Right now this is not supported at all.  When added,
                                // put full functionality code here and change status.

                                // Skip over the header info
                                TxData = TX_DATA_START + MSG_HEADER_SIZE;

                                TxBuffer[TxData++] = ZDO_NOT_SUPPORTED;
                                PrepareMessageResponse( MGMT_RTG_rsp );
                                nextPrimitive = APSDE_DATA_request;
                            #else

                                // Skip over the header info
                                TxData = TX_DATA_START + MSG_HEADER_SIZE;

                                TxBuffer[TxData++] = ZDO_NOT_SUPPORTED;
                                PrepareMessageResponse( MGMT_RTG_rsp );
                                nextPrimitive = APSDE_DATA_request;
                            #endif
                            break;

                        case MGMT_BIND_req:
                            #ifdef INCLUDE_OPTIONAL_NODE_MANAGEMENT_SERVICES
                                // TODO: Right now this is not supported at all.  When added,
                                // put full functionality code here and change status.

                                // Skip over the header info
                                TxData = TX_DATA_START + MSG_HEADER_SIZE;

                                TxBuffer[TxData++] = ZDO_NOT_SUPPORTED;
                                PrepareMessageResponse( MGMT_BIND_rsp );
                                nextPrimitive = APSDE_DATA_request;
                            #else

                                // Skip over the header info
                                TxData = TX_DATA_START + MSG_HEADER_SIZE;

                                TxBuffer[TxData++] = ZDO_NOT_SUPPORTED;
                                PrepareMessageResponse( MGMT_BIND_rsp );
                                nextPrimitive = APSDE_DATA_request;
                            #endif
                            break;

                        case MGMT_LEAVE_req:
                            #ifdef INCLUDE_OPTIONAL_NODE_MANAGEMENT_SERVICES
                                // TODO: Right now this is not supported at all.  When added,
                                // put full functionality code here and change status.

                                // Skip over the header info
                                TxData = TX_DATA_START + MSG_HEADER_SIZE;

                                TxBuffer[TxData++] = ZDO_NOT_SUPPORTED;
                                PrepareMessageResponse( MGMT_LEAVE_rsp );
                                nextPrimitive = APSDE_DATA_request;
                            #else

                                // Skip over the header info
                                TxData = TX_DATA_START + MSG_HEADER_SIZE;

                                TxBuffer[TxData++] = ZDO_NOT_SUPPORTED;
                                PrepareMessageResponse( MGMT_LEAVE_rsp );
                                nextPrimitive = APSDE_DATA_request;
                            #endif
                            break;

                        case MGMT_DIRECT_JOIN_req:
                            #ifdef INCLUDE_OPTIONAL_NODE_MANAGEMENT_SERVICES
                                // TODO: Right now this is not supported at all.  When added,
                                // put full functionality code here and change status.

                                // Skip over the header info
                                TxData = TX_DATA_START + MSG_HEADER_SIZE;

                                TxBuffer[TxData++] = ZDO_NOT_SUPPORTED;
                                PrepareMessageResponse( MGMT_DIRECT_JOIN_rsp );
                                nextPrimitive = APSDE_DATA_request;
                            #else

                                // Skip over the header info
                                TxData = TX_DATA_START + MSG_HEADER_SIZE;

                                TxBuffer[TxData++] = ZDO_NOT_SUPPORTED;
                                PrepareMessageResponse( MGMT_DIRECT_JOIN_rsp );
                                nextPrimitive = APSDE_DATA_request;
                            #endif
                            break;

                        default:
                            break;
                    }

                    ZDODiscardRx();
                }
                break;  // ZDO_DATA_indication

            #if defined (SUPPORT_END_DEVICE_BINDING)

            case ZDO_END_DEVICE_BIND_req:
                {
                    END_DEVICE_BIND_INFO    *bindInfo;

                    nextPrimitive = NO_PRIMITIVE;
                    if ((bindInfo = (END_DEVICE_BIND_INFO *)SRAMalloc( sizeof(END_DEVICE_BIND_INFO) )) != NULL)
                    {
                        bindInfo->shortAddr          = macPIB.macShortAddress;
                        // should always be EP0 bindInfo->sourceEP           = sourceEP       = params.ZDO_END_DEVICE_BIND_req.SrcEndpoint;
                        bindInfo->sequenceNumber     = sequenceNumber = params.ZDO_END_DEVICE_BIND_req.sequenceNumber;
                        bindInfo->bindEP             = params.ZDO_END_DEVICE_BIND_req.endpoint;
                        bindInfo->profileID          = params.ZDO_END_DEVICE_BIND_req.ProfileID;
                        bindInfo->numInClusters      = params.ZDO_END_DEVICE_BIND_req.NumInClusters;
                        bindInfo->inClusterList      = params.ZDO_END_DEVICE_BIND_req.InClusterList;
                        bindInfo->numOutClusters     = params.ZDO_END_DEVICE_BIND_req.NumOutClusters;
                        bindInfo->outClusterList     = params.ZDO_END_DEVICE_BIND_req.OutClusterList;

                        nextPrimitive = ProcessEndDeviceBind( bindInfo );
                    }
                }
                break;

            #endif

            #if defined(I_SUPPORT_BINDINGS)

            case ZDO_BIND_req:
                if (ProcessBindAndUnbind( BIND_FROM_UPPER_LAYERS | BIND_NODES,
                    (LONG_ADDR *)&params.ZDO_BIND_req.SrcAddress,
                    (LONG_ADDR *)&params.ZDO_BIND_req.DstAddress ))
                {
                    // If successful, a fake APSDE_DATA_indication was created with the response.
                    nextPrimitive = APSDE_DATA_indication;
                }
                else
                {
                    nextPrimitive = NO_PRIMITIVE;
                }
                break;

            case ZDO_UNBIND_req:
                if (ProcessBindAndUnbind( UNBIND_FROM_UPPER_LAYERS | UNBIND_NODES,
                    (LONG_ADDR *)&params.ZDO_UNBIND_req.SrcAddress,
                    (LONG_ADDR *)&params.ZDO_UNBIND_req.DstAddress ))
                {
                    // If successful, a fake APSDE_DATA_indication was created with the response.
                    nextPrimitive = APSDE_DATA_indication;
                }
                else
                {
                    nextPrimitive = NO_PRIMITIVE;
                }
                break;

            #endif

            default:
                break;
        } // Input Primitive
    }
    return nextPrimitive;
}



/*********************************************************************
 * Function:        void FinishAddressResponses( BYTE clusterID )
 *
 * PreCondition:    None
 *
 * Input:           clusterID - output cluster ID
 *
 * Output:          None
 *
 * Side Effects:    Message sent
 *
 * Overview:        This function finishes the address responses for the
 *                  NWK_ADDR_req and IEEE_ADDR_req clusters.
 *
 * Note:
 *
 * TODO: The spec is confusing on the IEEE_ADDR_rsp.  They mention in one place that we're
 * supposed to respond with the "16-bit IEEE addresses", but in the table for the
 * response it says 16-bit short address, which make the response the same
 * as for NWK_addr_rps.  And the Framework spec alludes to being able to request
 * both short and long addresses.  We'll send the short addresses, with code for
 * the long addresses left in the comments in case we need it later.
 ********************************************************************/
void FinishAddressResponses( BYTE clusterID )
{
#ifndef I_AM_END_DEVICE
    BYTE        i;
#endif
    BYTE        requestType;
    BYTE        startIndex;

    // Now get the rests of the paramters.
    requestType = ZDOGet();
    startIndex  = ZDOGet();          // 0-based or 1-based? assuming 0-based...

    if (requestType == SINGLE_DEVICE_RESPONSE)
    {
        // Skip over the header info
        TxData = TX_DATA_START + MSG_HEADER_SIZE;

        // Load status byte
        TxBuffer[TxData++] = SUCCESS;

        // Load our long address.
        GetMACAddress( &(TxBuffer[TxData]) );
        TxData += 8;

        // Load our short address.
        TxBuffer[TxData++] = macPIB.macShortAddress.byte.LSB;
        TxBuffer[TxData++] = macPIB.macShortAddress.byte.MSB;

        PrepareMessageResponse( clusterID );
    }
    else if (requestType == EXTENDED_RESPONSE)
    {
        #if defined(I_AM_END_DEVICE)

            // Skip over the header info
            TxData = TX_DATA_START + MSG_HEADER_SIZE;

            // Load status byte
            TxBuffer[TxData++] = SUCCESS;

            // Load our long address.
            GetMACAddress( &(TxBuffer[TxData]) );
            TxData += 8;

            // Load our short address.
            TxBuffer[TxData++] = macPIB.macShortAddress.byte.LSB;
            TxBuffer[TxData++] = macPIB.macShortAddress.byte.MSB;

            // Load NumAssocDev value
            // For end device, there are no associated device.  So we send 0 for the
            // number of associated devices and don't send StartIndex and the list
            // of associated devices.
            TxBuffer[TxData++] = 0;

            PrepareMessageResponse( clusterID );
        #else

            // Skip over the header info
            TxData = TX_DATA_START + MSG_HEADER_SIZE;

            // Load status byte
            TxBuffer[TxData++] = SUCCESS;

            // Load our long address.
            GetMACAddress( &(TxBuffer[TxData]) );
            TxData += 8;

            // Load our short address.
            TxBuffer[TxData++] = macPIB.macShortAddress.byte.LSB;
            TxBuffer[TxData++] = macPIB.macShortAddress.byte.MSB;

            // Load NumAssocDev value
            TxBuffer[TxData++] = currentNeighborTableInfo.numChildren;
                // byte count += 1 : 12

            // Send the start index back
            TxBuffer[TxData++] = startIndex;
                // byte count += 1 : 13

            if (startIndex < currentNeighborTableInfo.numChildren) // if 1-based, <=
            {
                // Send the associated devices list.
                i = 0;
                pCurrentNeighborRecord = neighborTable;
                while ((i < MAX_SHORTADDS_TO_SEND) && (i < currentNeighborTableInfo.numChildren))
//              while ((i < MAX_LONGADDS_TO_SEND) && (i < currentNeighborTableInfo.numChildren))
                {
                    do
                    {
                        GetNeighborRecord(&currentNeighborRecord, (ROM void*)pCurrentNeighborRecord );
                    }
                    while ( (!currentNeighborRecord.deviceInfo.bits.bInUse) ||
                            (currentNeighborRecord.deviceInfo.bits.Relationship != NEIGHBOR_IS_CHILD));

                    if (i >= startIndex)
                    {
                        TxBuffer[TxData++] = currentNeighborRecord.shortAddr.byte.LSB;
                        TxBuffer[TxData++] = currentNeighborRecord.shortAddr.byte.MSB;
//                      memcpy( (void *)&TxBuffer[TxData], (void *)&currentNeighborRecord.longAddr, sizeof(currentNeighborRecord.longAddr) );
//                      TxData += sizeof(currentNeighborRecord.longAddr;
                    }

                    i ++;   // if 1-based, move to before if statement
                }

            }
            PrepareMessageResponse( clusterID );
        #endif
    }
    else
    {
        // Load status byte
        TxBuffer[TxData++] = ZDO_INV_REQUESTTYPE;

        PrepareMessageResponse( clusterID );
    }
}

/*********************************************************************
 * Function:        BOOL IsThisMyShortAddr(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          TRUE - The next two bytes at adsu match my short address
 *                  FALSE - The next two bytes at adsu do not match
 *
 * Side Effects:    None
 *
 * Overview:        This function determines if the next two bytes at
 *                  adsu match my short address or are the broadcast
 *                  address.
 *
 * Note:            The broadcast address is allowed as a match for
 *                  the MATCH_DESC_req.  That is the only one that is
 *                  allowed to be broadcast - all the others must be
 *                  unicast.
 ********************************************************************/

BOOL IsThisMyShortAddr(void)
{
    SHORT_ADDR  address;

    address.byte.LSB = ZDOGet();
    address.byte.MSB = ZDOGet();

    if ((address.Val == macPIB.macShortAddress.Val) ||
        (address.Val == 0xFFFF))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


/*********************************************************************
 * Function:        END_DEVICE_BIND_INFO * GetOneEndDeviceBindRequest(
                            END_DEVICE_BIND_INFO * pRequestInfo )
 *
 * PreCondition:    None
 *
 * Input:           pRequestInfo - pointer to the end device bind request
 *                  information.  If this is null, the request is being
 *                  received from another device, and the information is
 *                  in the received message.  Otherwise, the information
 *                  must be extracted from the incoming message.
 *
 * Output:          Next ZigBee primitive
 *
 * Side Effects:    Numerous
 *
 * Overview:        This function retrieves end device binding information
 *                  from either the upper layers or an incoming message.
 *
 * Note:            None
 ********************************************************************/
#if defined SUPPORT_END_DEVICE_BINDING && (MAX_BINDINGS > 0)

END_DEVICE_BIND_INFO * GetOneEndDeviceBindRequest( END_DEVICE_BIND_INFO * pRequestInfo )
{
    BYTE                    *firstClusterPtr;
    BYTE                    i;
    END_DEVICE_BIND_INFO    *pEndDeviceBindRequest;
    BYTE                    *secondClusterPtr;

    pEndDeviceBindRequest = NULL;

    if (pRequestInfo == NULL)
    {
        // We are getting this request from another device
        if ((pEndDeviceBindRequest = (END_DEVICE_BIND_INFO *)SRAMalloc( sizeof(END_DEVICE_BIND_INFO) )) != NULL)
        {
            pEndDeviceBindRequest->flags.Val             = 0;
            pEndDeviceBindRequest->shortAddr             = params.ZDO_DATA_indication.SrcAddress.ShortAddr;
            pEndDeviceBindRequest->sequenceNumber        = sequenceNumber;
            // should always be EP0 pEndDeviceBindRequest->sourceEP              = params.ZDO_DATA_indication.SrcEndpoint;
            pEndDeviceBindRequest->inClusterList         = NULL;
            pEndDeviceBindRequest->outClusterList        = NULL;

            pEndDeviceBindRequest->bindEP                = ZDOGet();
            pEndDeviceBindRequest->profileID.byte.LSB    = ZDOGet();
            pEndDeviceBindRequest->profileID.byte.MSB    = ZDOGet();
            pEndDeviceBindRequest->numInClusters         = ZDOGet();

            if (pEndDeviceBindRequest->numInClusters != 0)
            {
                //DEBUG_OUT( "Getting input clusters\r\n" );
                if ((firstClusterPtr = (BYTE *)SRAMalloc( pEndDeviceBindRequest->numInClusters )) == NULL)
                {
                    free( pEndDeviceBindRequest );
                    goto BailFromGatheringInfo;
                }
                else
                    pEndDeviceBindRequest->inClusterList = firstClusterPtr;
            }
            for (i=0; i<pEndDeviceBindRequest->numInClusters; i++, firstClusterPtr++)
            {
                *firstClusterPtr = ZDOGet();
            }

            pEndDeviceBindRequest->numOutClusters        = ZDOGet();
            if (pEndDeviceBindRequest->numOutClusters != 0)
            {
                if ((firstClusterPtr = (BYTE *)SRAMalloc( pEndDeviceBindRequest->numOutClusters )) == NULL)
                {
                    if (pEndDeviceBindRequest->inClusterList)
                    {
                        free( pEndDeviceBindRequest->inClusterList);
                    }
                    free( pEndDeviceBindRequest );
                    goto BailFromGatheringInfo;
                }
                else
                    pEndDeviceBindRequest->outClusterList = firstClusterPtr;
            }
            for (i=0; i<pEndDeviceBindRequest->numOutClusters; i++, firstClusterPtr++)
            {
                *firstClusterPtr = ZDOGet();
            }

            pEndDeviceBindRequest->flags.bits.bFromSelf = 0;
        }
    }
    else
    {
        // We are getting this request from ourself
        pEndDeviceBindRequest = pRequestInfo;
        pEndDeviceBindRequest->flags.bits.bFromSelf = 1;
    }

BailFromGatheringInfo:
    return pEndDeviceBindRequest;
}
#endif

/*********************************************************************
 * Function:        ZIGBEE_PRIMITIVE ProcessEndDeviceBind(
                        END_DEVICE_BIND_INFO *pRequestInfo )
 *
 * PreCondition:    sequenceNumber and sourceEP must be set
 *
 * Input:           pRequestInfo - pointer to the end device bind request
 *                  information.  If this is null, the request is being
 *                  received from another device, and the information is
 *                  in the received message.
 *
 * Output:          Next ZigBee primitive
 *
 * Side Effects:    Numerous
 *
 * Overview:        This function performs either the first half or the
 *                  second half of end device binding.  It can be invoked
 *                  either by receiving an END_DEVICE_BIND_req message
 *                  or by the upper layers of the application.  The first
 *                  time the routine is called, the bind information is
 *                  simply stored.  The second time it is called, the two
 *                  lists are checked for a match, and bindings are
 *                  created if possible.  The background processing will
 *                  check for timeout of the first message.
 *
 * Note:            None
 ********************************************************************/
#ifdef SUPPORT_END_DEVICE_BINDING

ZIGBEE_PRIMITIVE ProcessEndDeviceBind( END_DEVICE_BIND_INFO *pRequestInfo )
{

#if defined SUPPORT_END_DEVICE_BINDING && (MAX_BINDINGS > 0)

    BYTE                    bindDirection;
    BYTE                    *firstClusterPtr;
    BYTE                    firstIndex;
    ZIGBEE_PRIMITIVE        nextPrimitive;
    END_DEVICE_BIND_INFO    *pSecondEndBindRequest  = NULL;
    BYTE                    *secondClusterPtr;
    BYTE                    secondIndex;
    BYTE                    status;

    status = ZDO_NO_MATCH;
    nextPrimitive = NO_PRIMITIVE;

    // Get Binding Target and make sure that it is us
    if (pRequestInfo == NULL)
    {
        if (!IsThisMyShortAddr())
        {
            return NO_PRIMITIVE;
        }
    }

    if (pFirstEndBindRequest == NULL)
    {
        // We are receiving the first request.  Gather all of the information from
        // this request and store it for later.
        if ((pFirstEndBindRequest = GetOneEndDeviceBindRequest( pRequestInfo )) == NULL)
        {
            goto BailFromEndDeviceRequest;
        }

        // Set the timer and wait for the second request.
        pFirstEndBindRequest->lastTick = TickGet();
        pFirstEndBindRequest->flags.bits.bSendResponse = 0;

        // Set the background flag for processing end device binding.
        zdoStatus.flags.bits.bEndDeviceBinding = 1;

        return NO_PRIMITIVE;
    }
    else
    {
        // We can only process one request at a time!  So make sure we aren't really receiving
        // a new request.  We can tell by checking the bSendResponse flag for the first request.
        // If we are still trying to send a response, then we are not ready for a new request,
        // and we'll have to discard it.
        if (pFirstEndBindRequest->flags.bits.bSendResponse)
        {
            return NO_PRIMITIVE;
        }

        // We are receiving the second request.  Gather all of the information from
        // this request so we can check for matches.
        if ((pSecondEndBindRequest = GetOneEndDeviceBindRequest( pRequestInfo )) == NULL)
        {
            goto BailFromEndDeviceRequest;
        }

        bindDirection = BIND_NOT_DETERMINED;

        // See if there are any matches between the first and the second requests.
        if (pFirstEndBindRequest->profileID.Val == pSecondEndBindRequest->profileID.Val)
        {
            // Check first request's input clusters against second request's output clusters
            for (firstIndex=0, firstClusterPtr = pFirstEndBindRequest->inClusterList;
                 firstIndex<pFirstEndBindRequest->numInClusters;
                 firstIndex++, firstClusterPtr++)
            {
                for (secondIndex=0, secondClusterPtr = pSecondEndBindRequest->outClusterList;
                     secondIndex<pSecondEndBindRequest->numOutClusters;
                     secondIndex++, secondClusterPtr++)
                {
                    if (*firstClusterPtr == *secondClusterPtr)
                    {
                        status = SUCCESS;
                        if (bindDirection == BIND_NOT_DETERMINED)
                        {
                            if (APSRemoveBindingInfo(pSecondEndBindRequest->shortAddr, pSecondEndBindRequest->bindEP,
                                    *secondClusterPtr, pFirstEndBindRequest->shortAddr, pFirstEndBindRequest->bindEP) == SUCCESS)
                                bindDirection = UNBIND_NODES;
                            else
                                bindDirection = BIND_NODES;
                        }
                        if (bindDirection == BIND_NODES)
                        {
                            APSAddBindingInfo(pSecondEndBindRequest->shortAddr, pSecondEndBindRequest->bindEP,
                                    *secondClusterPtr, pFirstEndBindRequest->shortAddr, pFirstEndBindRequest->bindEP);
                        }
                        else
                        {
                            APSRemoveBindingInfo(pSecondEndBindRequest->shortAddr, pSecondEndBindRequest->bindEP,
                                    *secondClusterPtr, pFirstEndBindRequest->shortAddr, pFirstEndBindRequest->bindEP);
                        }
                    }
                }
            }

            // Check first request's output clusters against second request's input clusters
            for (firstIndex=0, firstClusterPtr = pFirstEndBindRequest->outClusterList;
                 firstIndex<pFirstEndBindRequest->numOutClusters;
                 firstIndex++, firstClusterPtr++)
            {
                for (secondIndex=0, secondClusterPtr = pSecondEndBindRequest->inClusterList;
                     secondIndex<pSecondEndBindRequest->numInClusters;
                     secondIndex++, secondClusterPtr++)
                {
                    if (*secondClusterPtr == *firstClusterPtr)
                    {
                        status = SUCCESS;
                        if (bindDirection == BIND_NOT_DETERMINED)
                        {
                            if (APSRemoveBindingInfo(pFirstEndBindRequest->shortAddr, pFirstEndBindRequest->bindEP,
                                    *firstClusterPtr, pSecondEndBindRequest->shortAddr, pSecondEndBindRequest->bindEP) == SUCCESS)
                                bindDirection = UNBIND_NODES;
                            else
                                bindDirection = BIND_NODES;
                        }
                        if (bindDirection == BIND_NODES)
                        {
                            APSAddBindingInfo(pFirstEndBindRequest->shortAddr, pFirstEndBindRequest->bindEP,
                                    *firstClusterPtr, pSecondEndBindRequest->shortAddr, pSecondEndBindRequest->bindEP);
                        }
                        else
                        {
                            APSRemoveBindingInfo(pFirstEndBindRequest->shortAddr, pFirstEndBindRequest->bindEP,
                                    *firstClusterPtr, pSecondEndBindRequest->shortAddr, pSecondEndBindRequest->bindEP);
                        }
                    }
                }
            }
        }
    }

BailFromEndDeviceRequest:

    // Send the responses to the two requestors.  Since each requires its own message, we must send one from
    // the background.  We'll end the second request here, and the first from the background, since pSecondEndBindRequest
    // is a local variable.  We'll clear zdoStatus.flags.bits.bEndDeviceBinding from the background after all of the
    // responses are sent.

    if (pFirstEndBindRequest != NULL)
    {
        // We can free the cluster lists here.
        if (pFirstEndBindRequest->inClusterList != NULL)
        {
            free( pFirstEndBindRequest->inClusterList );
        }
        if (pFirstEndBindRequest->outClusterList != NULL)
        {
            free( pFirstEndBindRequest->outClusterList );
        }

        pFirstEndBindRequest->flags.bits.fResponse = status;
        pFirstEndBindRequest->flags.bits.bSendResponse = 1;
    }
    if (pSecondEndBindRequest != NULL)
    {
        nextPrimitive = Send_END_DEVICE_BIND_rsp( pSecondEndBindRequest, status );

        if (pSecondEndBindRequest->inClusterList != NULL)
        {
            free( pSecondEndBindRequest->inClusterList );
        }
        if (pSecondEndBindRequest->outClusterList != NULL)
        {
            free( pSecondEndBindRequest->outClusterList );
        }

        free( pSecondEndBindRequest );
    }

    return nextPrimitive;

#else

    // Skip over the header info
    TxData = TX_DATA_START + MSG_HEADER_SIZE;

    TxBuffer[TxData++] = ZDO_NOT_SUPPORTED;
    PrepareMessageResponse( END_DEVICE_BIND_rsp );
    return APSDE_DATA_request;

#endif

}

#endif

/*********************************************************************
 * Function:        ZIGBEE_PRIMITIVE Send_END_DEVICE_BIND_rsp(
                        END_DEVICE_BIND_INFO *pBindRequest, BYTE status )
 *
 * PreCondition:    End Device Binding should be in progress.
 *
 * Input:           *pBindRequest - pointer to the end device bind info
 *                  status - status of the end device bind
 *
 * Output:          Next primitive
 *
 * Side Effects:    None
 *
 * Overview:        This function either notifies the application of the
 *                  result of the end device bind, or sends a message to the
 *                  end device with the result.  It then deallocates the
 *                  memory that was allocated for the end device bind.
 *
 * Note:            Do to compiler limitations, we cannot set the
 *                  xxxBindRequest pointer passed in to 0.  Therefore,
 *                  the application must do the final free of the
 *                  xxxBindRequest memory.
 *
 ********************************************************************/
#ifdef SUPPORT_END_DEVICE_BINDING
ZIGBEE_PRIMITIVE Send_END_DEVICE_BIND_rsp( END_DEVICE_BIND_INFO *pBindRequest, BYTE status )
{
    ZIGBEE_PRIMITIVE    nextPrimitive;
    BYTE                *ptr;

    nextPrimitive = NO_PRIMITIVE;
    if (pBindRequest != NULL)
    {
        if (pBindRequest->flags.bits.bFromSelf)
        {
            // If we called Process_END_DEVICE_BIND_req from the upper layers, generate a fake
            // APSDE_DATA_indication to send back the answer.
            if (CurrentRxPacket == NULL)
            {
                if ((CurrentRxPacket = SRAMalloc(4)) != NULL)
                {
                    ptr = CurrentRxPacket;

                    // Load header information.
                    *ptr++ = APL_FRAME_TYPE_MSG | 1;            // Frame Header, MSG, 1 transaction
                    *ptr++ = pBindRequest->sequenceNumber;      // Transaction Sequence Number
                    *ptr++ = 1;                                 // Transaction Length

                    // Load data.
                    *ptr = status;

                    // Populate the remainder of the parameters.
                    params.APSDE_DATA_indication.asduLength             = 4;
                    params.APSDE_DATA_indication.SecurityStatus         = FALSE;
                    params.APSDE_DATA_indication.asdu                   = CurrentRxPacket;
                    params.APSDE_DATA_indication.ProfileId.Val          = ZDP_PROFILE_ID;
                    params.APSDE_DATA_indication.SrcAddrMode            = APS_ADDRESS_16_BIT;
                    params.APSDE_DATA_indication.WasBroadcast           = FALSE;
                    params.APSDE_DATA_indication.SrcAddress.ShortAddr   = macPIB.macShortAddress;
                    params.APSDE_DATA_indication.SrcEndpoint            = EP_ZDO;
                    params.APSDE_DATA_indication.DstEndpoint            = EP_ZDO; // should always be EP0 pBindRequest->sourceEP;
                    params.APSDE_DATA_indication.ClusterId              = END_DEVICE_BIND_rsp;

                    nextPrimitive = APSDE_DATA_indication;
                }
            }
        }
        else
        {
            // Skip over the header info
            TxData = TX_DATA_START + MSG_HEADER_SIZE;

            TxBuffer[TxData++] = status;

            // Load the message information.  We have to patch the destination address.
            sequenceNumber = pBindRequest->sequenceNumber;
            // should always be EP0 sourceEP = pBindRequest->sourceEP;
            PrepareMessageResponse( END_DEVICE_BIND_rsp );
            params.APSDE_DATA_request.DstAddress.ShortAddr  = pBindRequest->shortAddr;

            #ifdef ENABLE_DEBUG
            {
                BYTE    buffer[60];

                sprintf( (char *)buffer, (ROM char *) "Sending end device bind response %d to %04x.\r\n\0",
                    status, pBindRequest->shortAddr.Val );
                ConsolePutString( buffer );
            }
            #endif

            nextPrimitive = APSDE_DATA_request;
        }

        if (pBindRequest->inClusterList != NULL)
        {
            free( pBindRequest->inClusterList );
        }
        if (pBindRequest->outClusterList != NULL)
        {
            free( pBindRequest->outClusterList );
        }
    }

    return nextPrimitive;
}
#endif

/*********************************************************************
 * Function:        void PrepareMessageResponse( BYTE clusterID )
 *
 * PreCondition:    Tx is not blocked, all data is loaded and TxData
 *                  is accurate.
 *
 * Input:           clusterID - cluster ID of the ZDO packet
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This function prepares the APSDE_DATA_request
 *                  parameters as needed by the ZDO responses.
 *
 * Note:            None
 *
 ********************************************************************/

void PrepareMessageResponse( BYTE clusterID )
{
    ZigBeeBlockTx();

    // Load the header information
    TxBuffer[TX_DATA_START]     = APL_FRAME_TYPE_MSG | 1;              // Frame Header, MSG, 1 transaction
    TxBuffer[TX_DATA_START+1]   = sequenceNumber;                      // Transaction Sequence Number of the request
    TxBuffer[TX_DATA_START+2]   = TxData-TX_DATA_START-MSG_HEADER_SIZE; // Transaction Length

    // We only get short address from the NWK layer.
    params.APSDE_DATA_request.DstAddrMode   = APS_ADDRESS_16_BIT;
    params.APSDE_DATA_request.DstAddress.ShortAddr.Val = params.ZDO_DATA_indication.SrcAddress.ShortAddr.Val;

    // params.APSDE_DATA_request.asduLength; in place with TxData
    params.APSDE_DATA_request.ProfileId.Val = ZDP_PROFILE_ID;
    params.APSDE_DATA_request.RadiusCounter = DEFAULT_RADIUS;
    params.APSDE_DATA_request.DiscoverRoute = ROUTE_DISCOVERY_ENABLE;
    params.APSDE_DATA_request.TxOptions.Val = 0x00;
    // TODO params.APSDE_DATA_request.TxOptions.bits.securityEnabled    = ???
    // TODO params.APSDE_DATA_request.TxOptions.bits.useNWKKey          = ???
    params.APSDE_DATA_request.SrcEndpoint   = EP_ZDO;   // should always be EP0 sourceEP;
    params.APSDE_DATA_request.DstEndpoint   = EP_ZDO;
    params.APSDE_DATA_request.ClusterId     = clusterID;
}


/*********************************************************************
 * Function:        BOOL ProcessBindAndUnbind( BIND_INFO bindInfo,
                    LONG_ADDR *sourceAddress, LONG_ADDR *destinationAddress )
 *
 * PreCondition:    If bindInfo.bFromUpperLayers is false, we are coming
 *                  from ZDO_DATA_indication, and asdu still points to the
 *                  beginning of the data area.
 *
 * Input:           bindInfo - the bind direction (bind/unbind) and if the
 *                      request is from upper layers or a received message.
 *
 * Output:          TRUE - process completed successfully
 *                  FALSE - from upper layers only, response could not be
 *                      generated
 *                  params.ZDO_BIND_req.Status updated
 *
 * Side Effects:    Binding created/destroyed.
 *
 * Overview:        This function performs a bind or unbind request.  The
 *                  request can be from either the upper layers or a
 *                  received message.
 *
 * Note:            The parameters for ZDO_BIND_req and ZDO_UNBIND_req
 *                  overlay each other, so we will just reference
 *                  ZDO_BIND_req parameters.
 *                  NOTE the spec is very vague on this function.  The
 *                  bind/unbind functions receive long addresses, but
 *                  bindings are created with short addresses. The spec
 *                  does not say how to obtain the short address.
 *
 *                  Coordinators may bind anyone to anyone.  Routers
 *                  may only bind themselves as the source.
 *
 ********************************************************************/
#if defined(I_SUPPORT_BINDINGS)

BOOL ProcessBindAndUnbind( BYTE bindInfo, LONG_ADDR *sourceAddress, LONG_ADDR *destinationAddress )
{
    BYTE    *ptr;

    params.ZDO_BIND_req.Status = SUCCESS;

    // Make sure that specified source and destination long addresses are
    // in our PAN.

    #ifndef I_AM_COORDINATOR
        if (NWKThisIsMyLongAddress( sourceAddress ))
        {
            // I am the source
            params.ZDO_BIND_req.SrcAddress.ShortAddr = macPIB.macShortAddress;
        }
        else
        {
            // Routers can only bind themselves as the source.
            params.ZDO_BIND_req.Status = BIND_NOT_SUPPORTED;
        }
    #else
        if ( NWKLookupNodeByLongAddr( sourceAddress ) == INVALID_NEIGHBOR_KEY )
        {
            if (NWKThisIsMyLongAddress( sourceAddress ))
            {
                // It is my address
                params.ZDO_BIND_req.SrcAddress.ShortAddr = macPIB.macShortAddress;
            }
            else
            {
                // Unknown source address.
                params.ZDO_BIND_req.Status = BIND_NOT_SUPPORTED;
            }
        }
        else
        {
            params.ZDO_BIND_req.SrcAddress.ShortAddr = currentNeighborRecord.shortAddr;
        }
    #endif

    if ( NWKLookupNodeByLongAddr( destinationAddress ) == INVALID_NEIGHBOR_KEY )
    {
        if (NWKThisIsMyLongAddress( destinationAddress ))
        {
            // It is my address
            params.ZDO_BIND_req.DstAddress.ShortAddr = macPIB.macShortAddress;
        }
        else
        {
            // Unknown destination address.
            params.ZDO_BIND_req.Status = BIND_NOT_SUPPORTED;
        }
    }
    else
    {
        params.ZDO_BIND_req.DstAddress.ShortAddr = currentNeighborRecord.shortAddr;
    }

    if (params.ZDO_BIND_req.Status == SUCCESS)
    {
        if ((bindInfo & BIND_SOURCE_MASK) == BIND_FROM_EXTERNAL)
        {
            // If the request is from the upper layers, the parameters are already in place.
            // Otherwise, load them.

            // Skip over the source address.
            params.ZDO_DATA_indication.asdu += 8;

            params.ZDO_BIND_req.SrcEndp   = ZDOGet();
            params.ZDO_BIND_req.ClusterID = ZDOGet();

            // Skip over the destination address.
            params.ZDO_DATA_indication.asdu += 8;

            params.ZDO_BIND_req.DstEndp = APSGet();
        }

        if ((bindInfo & BIND_DIRECTION_MASK) == BIND_NODES)
        {
            if (APSAddBindingInfo( params.ZDO_BIND_req.SrcAddress.ShortAddr, params.ZDO_BIND_req.SrcEndp,
                params.ZDO_BIND_req.ClusterID, params.ZDO_BIND_req.DstAddress.ShortAddr, params.ZDO_BIND_req.DstEndp))
            {
                params.ZDO_BIND_req.Status = ZDO_TABLE_FULL;
            }
        }
        else
        {
            if (APSRemoveBindingInfo( params.ZDO_BIND_req.SrcAddress.ShortAddr, params.ZDO_BIND_req.SrcEndp,
                params.ZDO_BIND_req.ClusterID, params.ZDO_BIND_req.DstAddress.ShortAddr, params.ZDO_BIND_req.DstEndp))
            {
                params.ZDO_BIND_req.Status = ZDO_NO_ENTRY;
            }
        }
    }

    if ((bindInfo & BIND_SOURCE_MASK) == BIND_FROM_UPPER_LAYERS)
    {
        // Create a fake APSDE_DATA_indication with the answer, as though it came from
        // a different node. This way the user can capture the response the same way for
        // both sources.
        if (CurrentRxPacket == NULL)
        {
            if ((CurrentRxPacket = SRAMalloc(4)) != NULL)
            {
                ptr = CurrentRxPacket;

                // Load header information.
                *ptr++ = APL_FRAME_TYPE_MSG | 1;            // Frame Header, MSG, 1 transaction
                *ptr++ = 0;                                 // Transaction Sequence Number
                *ptr++ = 1;                                 // Transaction Length

                // Load data.
                *ptr = params.ZDO_BIND_req.Status;

                // Populate the remainder of the parameters.
                params.APSDE_DATA_indication.asduLength                 = 4;
                params.APSDE_DATA_indication.SecurityStatus             = FALSE;
                params.APSDE_DATA_indication.asdu                       = CurrentRxPacket;
                params.APSDE_DATA_indication.ProfileId.Val              = ZDP_PROFILE_ID;
                params.APSDE_DATA_indication.SrcAddrMode                = APS_ADDRESS_16_BIT;
                params.APSDE_DATA_indication.WasBroadcast               = FALSE;
                params.APSDE_DATA_indication.SrcAddress.ShortAddr       = macPIB.macShortAddress;
                params.APSDE_DATA_indication.SrcEndpoint                = EP_ZDO;
                params.APSDE_DATA_indication.DstEndpoint                = EP_ZDO; //!@#$% Is this guaranteed?
                if ((bindInfo & BIND_DIRECTION_MASK) == BIND_NODES)
                {
                    params.APSDE_DATA_indication.ClusterId              = BIND_rsp;
                }
                else
                {
                    params.APSDE_DATA_indication.ClusterId              = UNBIND_rsp;
                }
            }
            else
            {
                return FALSE;
            }
        }
        else
        {
            return FALSE;
        }
    }
    return TRUE;
}
#endif
