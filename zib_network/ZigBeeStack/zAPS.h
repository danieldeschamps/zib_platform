/*********************************************************************
 *
 *                  ZigBee APS Header File
 *
 *********************************************************************
 * FileName:        zAPS.h
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

#ifndef _zAPS_H_
#define _zAPS_H_

// zAPS.h

// ******************************************************************************
// APS Layer Spec Constants

// The maximum number of Address Map entries.
#define apscMaxAddrMapEntries       MAX_APS_ADDRESSES

// The maximum number of octets contained in a non-complex descriptor.
#define apscMaxDescriptorSize       64

// The maximum number of octets that can be returned through the discovery process.
#define apscMaxDiscoverySize        64

// The maximum number of octets added by the APS sub-layer to its payload.
#ifdef I_SUPPORT_SECURITY
    #define apscMaxFrameOverhead    20
#else
    #define apscMaxFrameOverhead    6
#endif

// The maximum number of retries allowed after a transmission failure.
#define apscMaxFrameRetries         3

// The maximum number of octets that can be transmitted in the APS frame payload
// field (see [R3]).
#define apscMaxPayloadSize          (nwkcMaxPayloadSize –apscMaxFrameOverhead)

// The maximum number of seconds to wait for an acknowledgement to a transmitted frame.
#ifdef I_SUPPORT_SECURITY
    #define apscAckWaitDuration     (ONE_SECOND * (0.05 * (2*nwkcMaxDepth) + 0.1))
#else
    #define apscAckWaitDuration     (ONE_SECOND * (0.05 * (2*nwkcMaxDepth)))
#endif


// ******************************************************************************
// Constant Definitions and Enumerations

#define APS_ADDRESS_MAP_VALID       0xC0DE
#define APS_ADDRESS_NOT_PRESENT     0x00
#define APS_ADDRESS_16_BIT          0x01
#define APS_ADDRESS_64_BIT          0x02

typedef enum _APS_STATUS_VALUES
{
    APS_SUCCESS             = 0x00,
    APS_NO_BOUND_DEVICE,    //0x01
    APS_SECURITY_FAIL,      //0x02
    APS_NO_ACK,             //0x03
    APS_INVALID_REQUEST     //0x04
} APS_STATUS_VALUES;


#define BIND_SUCCESS                0x00
#define BIND_NOT_SUPPORTED          0x84
#define BIND_TABLE_FULL             0x87
#define BIND_NO_ENTRY               0x88

typedef enum _BINDING_RESULTS
{
    BIND_ILLEGAL_DEVICE = 0x03,     //0x03
    BIND_ILLEGAL_REQUEST,           //0x04
    BIND_INVALID_BINDING            //0x05
} BINDING_RESULTS;

#define MAX_APS_FRAMES              (NUM_BUFFERED_INDIRECT_MESSAGES + MAX_APL_FRAMES)

// ******************************************************************************
// Structures

typedef struct _APS_ADDRESS_MAP
{
    LONG_ADDR   longAddr;
    SHORT_ADDR  shortAddr;
} APS_ADDRESS_MAP;


// This structure is used for storing source and destination binding
// records.  Within the array of binding records, a valid node is
// indicated by the corresponding bit in bindingTableUsageMap being
// set.  The type of record is indicated by the corresponding bit in
// bindingTableSourceNodeMap.  If the bit is set, then shortAddress
// and endPoint pertain to the source and clusterID is guaranteed to
// be correct.  If it is clear, then they pertain to the destination,
// and clusterID may be undefined.  Note that the first node in each
// list will be the source record, followed by one or more
// destination records.
// NOTE - the limitation is that a short address may only support one profile.
typedef struct _BINDING_RECORD
{
    SHORT_ADDR      shortAddr;
    BYTE            endPoint;
    BYTE            clusterID;
    BYTE            nextBindingRecord;
} BINDING_RECORD;    // 5 bytes long


// ******************************************************************************
// Macro Definitions

#define APSDiscardRx()          MACDiscardRx()
#define APSClearBindingTable()  ClearBindingTable()


// ******************************************************************************
// Function Prototypes

BYTE                APSGet( void ); // TODO can we consolidate all these get functions?
BOOL                APSHasBackgroundTasks( void );
void                APSInit( void );
ZIGBEE_PRIMITIVE    APSTasks(ZIGBEE_PRIMITIVE inputPrimitive);

#if defined(I_SUPPORT_BINDINGS)
    BYTE APSAddBindingInfo( SHORT_ADDR srcAddr, BYTE srcEP, BYTE clusterID,
                     SHORT_ADDR destAddr, BYTE destEP );
    BYTE APSRemoveBindingInfo( SHORT_ADDR srcAddr, BYTE srcEP, BYTE clusterID, SHORT_ADDR destAddr, BYTE destEP );
#endif

#if MAX_APS_ADDRESSES > 0
    void APSClearAPSAddressTable( void );
#endif

#endif
