/*********************************************************************
 *
 *                  ZigBee NWK Header File
 *
 *********************************************************************
 * FileName:        zNWK.h
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

#ifndef _zNWK_H_
#define _zNWK_H_

#include "ZigbeeTasks.h"

// ******************************************************************************
// NWK Layer Spec Constants

// A Boolean flag indicating whether the device is capable of becoming the
// ZigBee coordinator.
#ifdef I_HAVE_COORDINATOR_CAPABILITY
    #define nwkcCoordinatorCapable  0x01
#else
    #define nwkcCoordinatorCapable  0x00
#endif

// The default security level to be used.
#define nwkcDefaultSecurityLevel  ENC-MIC-64

// The maximum number of times a route discovery will be retried.
#define nwkcDiscoveryRetryLimit     0x03

// The maximum depth (minimum number of logical hops from the ZigBee
// coordinator) a device can have.
#define nwkcMaxDepth                0x0f

// The maximum number of octets added by the NWK layer to its payload without
// security. If security is required on a frame, its secure processing may inflate
// the frame length so that it is greater than this value.
#define nwkcMaxFrameOverhead        0x0d

// The maximum number of octets that can be transmitted in the NWK frame payload field.
#define nwkcMaxPayloadSize          (aMaxMACFrameSize � nwkcMaxFrameOverhead)

// The version of the ZigBee NWK protocol in the device.
#define nwkcProtocolVersion         0x01

// Maximum number of allowed communication errors after which the route repair mechanism is initiated.
#define nwkcRepairThreshold         0x03

// Time duration in milliseconds until a route discovery expires.
#define nwkcRouteDiscoveryTime      0x2710

// The maximum broadcast jitter time measured in milliseconds.
#define nwkcMaxBroadcastJitter      0x40

// The number of times the first broadcast transmission of a route request
// command frame is retried.
#define nwkcInitialRREQRetries      0x03

// The number of times the broadcast transmission of a route request command frame is retried on
// relay by an intermediate ZigBee router or ZigBee coordinator.
#define nwkcRREQRetries             0x02

// The number of milliseconds between retries of a broadcast route request command frame.
#define nwkcRREQRetryInterval       0xfe

// The minimum jitter, in 2 millisecond slots, for broadcast retransmission of a route
// request command frame.
#define nwkcMinRREQJitter           0x01

// The maximum jitter, in 2 millisecond slots, for broadcast retransmission of a
// route request command frame.
#define nwkcMaxRREQJitter           0x40


// ******************************************************************************
// Constants and Enumerations

#define DEFAULT_RADIUS                  (2*PROFILE_nwkMaxDepth)
#define INVALID_NWK_HANDLE              0x00
#define INVALID_NEIGHBOR_KEY            (NEIGHBOR_KEY)(MAX_NEIGHBORS)
#define ROUTE_DISCOVERY_ENABLE          0x01
#define ROUTE_DISCOVERY_FORCE           0x02
#define ROUTE_DISCOVERY_SUPPRESS        0x00


typedef enum _LEAVE_REASONS
{
    COORDINATOR_FORCED_LEAVE        = 0x01,
    SELF_INITIATED_LEAVE            = 0x02
} LEAVE_REASONS;


typedef enum _NWK_STATUS_VALUES
{
    NWK_SUCCESS                     = 0x00,
    NWK_INVALID_PARAMETER           = 0xC1,
    NWK_INVALID_REQUEST             = 0xC2,
    NWK_NOT_PERMITTED               = 0xC3,
    NWK_STARTUP_FAILURE             = 0xC4,
    NWK_ALREADY_PRESENT             = 0xC5,
    NWK_SYNC_FAILURE                = 0xC6,
    NWK_TABLE_FULL                  = 0xC7,
    NWK_UNKNOWN_DEVICE              = 0xC8,
    NWK_UNSUPPORTED_ATTRIBUTE       = 0xC9,
    NWK_NO_NETWORKS                 = 0xCA,
    NWK_LEAVE_UNCONFIRMED           = 0xCB,
    NWK_MAX_FRM_CNTR                = 0xCC, // Security failed - frame counter reached maximum
    NWK_NO_KEY                      = 0xCD, // Security failed - no key
    NWK_BAD_CCM_OUTPUT              = 0xCE, // Security failed - bad output
    NWK_CANNOT_ROUTE
} NWK_STATUS_VALUES;


typedef enum _RELATIONSHIP_TYPE
{
    NEIGHBOR_IS_PARENT  = 0,
    NEIGHBOR_IS_CHILD,
    NEIGHBOR_IS_SIBLING,
    NEIGHBOR_IS_NONE
} RELATIONSHIP_TYPE;


typedef enum _ROUTE_STATUS
{
    ROUTE_ACTIVE                    = 0x00,
    ROUTE_DISCOVERY_UNDERWAY        = 0x01,
    ROUTE_DISCOVERY_FAILED          = 0x02,
    ROUTE_INACTIVE                  = 0x03
} ROUTE_STATUS;


// ******************************************************************************
// Structures

typedef struct __Config_NWK_Mode_and_Params
{
     BYTE   ProtocolVersion;
     BYTE   StackProfile;
     BYTE   BeaconOrder;
     BYTE   SuperframeOrder;
     BYTE   BatteryLifeExtension;
     BYTE   SecuritySetting;
     DWORD  Channels;
} _Config_NWK_Mode_and_Params;


typedef BYTE NEIGHBOR_KEY;


typedef union _NEIGHBOR_RECORD_DEVICE_INFO
{
    struct
    {
        BYTE LQI                : 8;
        BYTE Depth              : 4;
        BYTE StackProfile       : 4;    // Needed for network discovery
        BYTE ZigBeeVersion      : 4;    // Needed for network discovery
        BYTE deviceType         : 2;
        BYTE Relationship       : 2;
        BYTE RxOnWhenIdle       : 1;
        BYTE bInUse             : 1;
        BYTE PermitJoining      : 1;
        BYTE PotentialParent    : 1;
    } bits;
    DWORD Val;
} NEIGHBOR_RECORD_DEVICE_INFO;


typedef struct _NEIGHBOR_RECORD
{
    LONG_ADDR                   longAddr;
    SHORT_ADDR                  shortAddr;
    PAN_ADDR                    panID;
    BYTE                        LogicalChannel; // Needed to add for NLME_JOIN_request and other things.
    NEIGHBOR_RECORD_DEVICE_INFO deviceInfo;
} NEIGHBOR_RECORD;  // 15 bytes long


typedef struct _NEIGHBOR_TABLE_INFO
{
    WORD        validityKey;
    BYTE        neighborTableSize;

#ifndef I_AM_COORDINATOR
    BYTE        parentNeighborTableIndex;
#endif

#ifndef I_AM_END_DEVICE
    BYTE        depth;              // Our depth in the network
    SHORT_ADDR  cSkip;              // Address block size
    SHORT_ADDR  nextEndDeviceAddr;  // Next address available to give to an end device
    SHORT_ADDR  nextRouterAddr;     // Next address available to give to a router
    BYTE        numChildren;        // How many children we have
    BYTE        numChildRouters;    // How many of our children are routers
    union _flags
    {
        BYTE    Val;
        struct _bits
        {
            BYTE    bChildAddressInfoValid : 1;  // Child addressing information is valid
        } bits;
    }flags;
#endif
} NEIGHBOR_TABLE_INFO;


typedef struct _ROUTING_ENTRY
{
    SHORT_ADDR      destAddress;
    ROUTE_STATUS    status;     // 3 bits valid
    SHORT_ADDR      nextHop;
} ROUTING_ENTRY;


// ******************************************************************************
// Macro Definitions

#define NWKDiscardRx() MACDiscardRx()


// ******************************************************************************
// Function Prototypes

void                NWKClearNeighborTable( void );
void                NWKClearRoutingTable( void );
NEIGHBOR_KEY        NWKLookupNodeByLongAddr( LONG_ADDR *longAddr );
BOOL                NWKHasBackgroundTasks( void );
void                NWKTidyNeighborTable( void );
void                NWKInit( void );
ZIGBEE_PRIMITIVE    NWKTasks( ZIGBEE_PRIMITIVE inputPrimitive );
BOOL                NWKThisIsMyLongAddress( LONG_ADDR *address );
BYTE                NLME_GET_nwkBCSN( void );

#endif
