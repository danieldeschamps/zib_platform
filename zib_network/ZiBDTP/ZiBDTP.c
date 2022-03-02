/*
    Microchip ZigBee Stack

    Demo RFD

    This demonstration shows how a ZigBee RFD can be set up.  This demo allows
    the PICDEM Z Demostration Board to act as either a "Switching Load Controller"
    (e.g. a light) or a "Switching Remote Control" (e.g. a switch) as defined by
    the Home Controls, Lighting profile.  It is designed to interact with a
    second PICDEM Z programmed with the Demo Coordinator project.

    To give the PICDEM Z "switch" capability, uncomment the I_AM_SWITCH definition
    below.  To give the PICDEM Z "light" capability, uncomment the I_AM_LIGHT
    definition below.  The PICDEM Z may have both capabilities enabled.  Be sure
    that the corresponding Demo Coordinator device is programmed with complementary
    capabilities.  NOTE - for simplicity, the ZigBee simple descriptors for this
    demonstration are fixed.

    If this node is configured as a "switch", it can discover the network address
    of the "light" using two methods.  If the USE_BINDINGS definition is
    uncommented below, then End Device Binding must be performed between the
    "switch" and the "light" before messages can be sent and received successfully.
    If USE_BINDINGS is commented out, then the node will default to the probable
    network address of the other node, and messages may be able to be sent
    immediately.  However, the node will also be capable of performing Device
    Discovery to discover the actual network address of the other node, in case
    the network was formed with alternate short address assignments.  NOTE: The
    USE_BINDINGS definition must be the same in both the RFD and the ZigBee
    Coordinator nodes.

    Switch functionality is as follows:
        RB4, I_AM_SWITCH defined, sends a "toggle" message to the other node's "light"
        RB4, I_AM_SWITCH not defined, no effect
        RB5, USE_BINDINGS defined, sends an End Device Bind request
        RB5, USE_BINDINGS undefined, sends a NWK_ADDR_req for the MAC address specified

    End Device Binding
    ------------------
    If the USE_BINDINGS definition is uncommented, the "switch" will send an
    APS indirect message to toggle the "light".  In order for the message to
    reach its final destination, a binding must be created between the "switch"
    and the "light".  To do this, press RB5 on one PICDEM Z, and then press RB5
    on the other PICDEM Z within 5 seconds.  A message will be displayed indicating
    if binding was successful or not.  Note that End Device Binding is a toggle
    function.  Performing the operation again will unbind the nodes, and messages
    will not reach their final destination.

    Device Discovery
    ----------------
    If the USE_BINDINGS definition is not uncommented, pressing RB5 will send a
    broadcast NWK_ADDR_req message.  The NWK_ADDR_req message contains the MAC
    address of the desired node.  Be sure this address matches the address
    contained in the other node's zigbee.def file.

    NOTE: To speed network formation, ALLOWED_CHANNELS has been set to
    channel 12 only.

 *********************************************************************
 * FileName:        RFD.c
 * Dependencies:
 * Processor:       PIC18F
 * Complier:        MCC18 v3.00 or higher
 * Company:         Microchip Technology, Inc.
 *
 * Software License Agreement
 *
 * The software supplied herewith by Microchip Technology Incorporated
 * (the “Company”) for its PICmicro® Microcontroller is intended and
 * supplied to you, the Company’s customer, for use solely and
 * exclusively on Microchip PICmicro Microcontroller products. The
 * software is owned by the Company and/or its supplier, and is
 * protected under applicable copyright laws. All rights are reserved.
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


//******************************************************************************
// Header Files
//******************************************************************************

// Include the main ZigBee header file.
#include "zAPL.h"

// If you are going to send data to a terminal, include this file.
#include "console.h"

// dds:
#include <delays.h>
#include <adc.h>
#include <stdlib.h>
#include <stdio.h> // sprintf

#include "tc77.h"

//******************************************************************************
// Configuration Bits
//******************************************************************************

#if defined(MCHP_C18) && defined(__18F4620)
    #pragma romdata CONFIG1H = 0x300001
    const rom unsigned char config1H = 0b00000110;      // HSPLL oscillator

    #pragma romdata CONFIG2L = 0x300002
    const rom unsigned char config2L = 0b00011111;      // Brown-out Reset Enabled in hardware @ 2.0V, PWRTEN disabled

    #pragma romdata CONFIG2H = 0x300003
    const rom unsigned char config2H = 0b00010010;      // HW WD disabled, 1:512 prescaler

    #pragma romdata CONFIG3H = 0x300005
    const rom unsigned char config3H = 0b10000000;      // PORTB digital on RESET

    #pragma romdata CONFIG4L = 0x300006
    const rom unsigned char config4L = 0b10000001;      // DEBUG disabled,
                                                        // XINST disabled
                                                        // LVP disabled
                                                        // STVREN enabled

    #pragma romdata

#elif defined(HITECH_C18) && defined(_18F4620)
    // Set configuration fuses for HITECH compiler.
    __CONFIG(1, 0x0600);    // HSPLL oscillator
    __CONFIG(2, 0x101F);    // PWRTEN disabled, BOR enabled @ 2.0V, HW WD disabled, 1:128 prescaler
    __CONFIG(3, 0x8000);    // PORTB digital on RESET
    __CONFIG(4, 0x0081);    // DEBUG disabled,
                            // XINST disabled
                            // LVP disabled
                            // STVREN enabled
#endif

//******************************************************************************
// Compilation Configuration
//******************************************************************************

// #define USE_BINDINGS
#define I_AM_LIGHT
#define I_AM_SWITCH

//******************************************************************************
// Constants
//******************************************************************************

// Switches and LEDs locations.
#define WAKEUP_SWITCH               RB5
#define LIGHT_SWITCH                RB4

#define BIND_INDICATION             LATA0
#define MESSAGE_INDICATION          LATA1

#define BIND_STATE_BOUND            0
#define BIND_STATE_TOGGLE           1
#define BIND_STATE_UNBOUND          1
#define BIND_WAIT_DURATION          (6*ONE_SECOND)

#define LIGHT_OFF                   0x00
#define LIGHT_ON                    0xFF
#define LIGHT_TOGGLE                0xF0


//******************************************************************************
// Application Variables
//******************************************************************************
// uChip

static union
{
    struct
    {
        BYTE    bWakeupSwitchToggled       : 1;
        BYTE    bLightSwitchToggled        : 1;
        BYTE    bTryingToBind              : 1;
        BYTE    bIsBound                   : 1;
        BYTE    bDestinationAddressKnown   : 1;
		BYTE	bConsoleKeyReceived        : 1;
    } bits;
    BYTE Val;
} myStatusFlags;
#define STATUS_FLAGS_INIT       0x00
#define TOGGLE_BOUND_FLAG       0x08

NETWORK_DESCRIPTOR  *currentNetworkDescriptor;
ZIGBEE_PRIMITIVE    currentPrimitive;
SHORT_ADDR          destinationAddress;
NETWORK_DESCRIPTOR  *NetworkDescriptor;
BYTE                orphanTries;

// dds:
#define MAX_DISCOVERY_ENTRY		2 // Network Nodes
#define MAX_DESCRIPTOR_ENTRY	2 // EPs inside the nodes

typedef struct _EP_DISCOVERY_ENTRY
{
	SHORT_ADDR addr;
	NODE_SIMPLE_DESCRIPTOR descriptor[MAX_DESCRIPTOR_ENTRY];
	int iDescriptorEntryCount;
} EP_DISCOVERY_ENTRY;

BYTE g_byteLastConsoleKey;
DWORD g_dwLastChannelMask;

int iDiscoveryEntryCount;
EP_DISCOVERY_ENTRY tableDiscovery[MAX_DISCOVERY_ENTRY];

int iIndex; // index for general purposes

//******************************************************************************
// Function Prototypes
//******************************************************************************

void HardwareInit( void );
BOOL myProcessesAreDone( void );
void InitInternalVariables(void);
static BYTE GetADCString( char *pstrValueRead );

//******************************************************************************
//******************************************************************************
// Main
//******************************************************************************
//******************************************************************************

void InitInternalVariables(void)
{
	// uChip
	myStatusFlags.Val = STATUS_FLAGS_INIT;
    destinationAddress.Val = 0x0000;
	BIND_INDICATION = !myStatusFlags.bits.bIsBound;
    MESSAGE_INDICATION = 0;

	// dds
	g_byteLastConsoleKey = 0;
	g_dwLastChannelMask = 0;
	iDiscoveryEntryCount = 0;
	memset((void*)&tableDiscovery, 0, sizeof(tableDiscovery));
}

void main(void)
{

    CLRWDT();
    ENABLE_WDT();

    currentPrimitive = NO_PRIMITIVE;
    NetworkDescriptor = NULL;
    orphanTries = 3;

    // If you are going to send data to a terminal, initialize the UART.
    ConsoleInit();

    TRACE("\r\n\r\n\r\n*************************************\r\n" );
    TRACE("Microchip ZigBee(TM) Stack - v1.0-3.6\r\n\r\n" );
    TRACE("ZigBee RFD - ZiB DTP\r\n\r\n" );
    #if (RF_CHIP == MRF24J40)
        TRACE("Transceiver-MRF24J40\r\n\r\n" );
    #elif (RF_CHIP==UZ2400)
        TRACE("Transceiver-UZ2400\r\n\r\n" );
    #elif (RF_CHIP==CC2420)
        TRACE("Transceiver-CC2420\r\n\r\n" );
    #else
        TRACE("Transceiver-Unknown\r\n\r\n" );
    #endif

    // Initialize the hardware - must be done before initializing ZigBee.
    HardwareInit();

    // Initialize the ZigBee Stack.
    ZigBeeInit();
    // *************************************************************************
    // Perform any other initialization here
    // *************************************************************************

    InitInternalVariables(); // dds

    // Enable interrupts to get everything going.
    RBIE = 1;
    IPEN = 1;
    GIEH = 1;
	GIEL = 1; // dds (ConsoleInterrupt): Peripheral Interrupt Enable bit
	RCIE = 1; // dds (ConsoleInterrupt): Enable RS-232 receive Interrupt

    while (1)
    {
        CLRWDT();
        ZigBeeTasks( &currentPrimitive );

        switch (currentPrimitive)
        {
            case NLME_NETWORK_DISCOVERY_confirm:
                currentPrimitive = NO_PRIMITIVE;
                if (!params.NLME_NETWORK_DISCOVERY_confirm.Status)
                {
                    if (!params.NLME_NETWORK_DISCOVERY_confirm.NetworkCount)
                    {
                        TRACE("No networks found.  Trying again...\r\n" );
                    }
                    else
                    {
                        // Save the descriptor list pointer so we can destroy it later.
                        NetworkDescriptor = params.NLME_NETWORK_DISCOVERY_confirm.NetworkDescriptor;

                        // Select a network to try to join.  We're not going to be picky right now...
                        currentNetworkDescriptor = NetworkDescriptor;

SubmitJoinRequest:
                        // not needed for new join params.NLME_JOIN_request.ScanDuration = ;
                        // not needed for new join params.NLME_JOIN_request.ScanChannels = ;
                        params.NLME_JOIN_request.PANId          = currentNetworkDescriptor->PanID;
						
						PrintChar(params.NLME_NETWORK_DISCOVERY_confirm.NetworkCount);
                        TRACE(" Network(s) found. Trying to join " );
                        PrintChar( params.NLME_JOIN_request.PANId.byte.MSB );
                        PrintChar( params.NLME_JOIN_request.PANId.byte.LSB );
                        TRACE(" (first always).\r\n" );
                        
						params.NLME_JOIN_request.JoinAsRouter   = FALSE;
                        params.NLME_JOIN_request.RejoinNetwork  = FALSE;
                        params.NLME_JOIN_request.PowerSource    = NOT_MAINS_POWERED;
                        params.NLME_JOIN_request.RxOnWhenIdle   = FALSE;
                        params.NLME_JOIN_request.MACSecurity    = FALSE;
                        currentPrimitive = NLME_JOIN_request;
                    }
                }
                else
                {
                    PrintChar( params.NLME_NETWORK_DISCOVERY_confirm.Status );
                    TRACE(" Error finding network.  Trying again...\r\n" );
                }
                break;

            case NLME_JOIN_confirm:
                currentPrimitive = NO_PRIMITIVE;
                if (!params.NLME_JOIN_confirm.Status)
                {
                    TRACE("Join successful!\r\n" );

                    // Free the network descriptor list, if it exists. If we joined as an orphan, it will be NULL.
                    while (NetworkDescriptor)
                    {
                        currentNetworkDescriptor = NetworkDescriptor->next;
                        free( NetworkDescriptor );
                        NetworkDescriptor = currentNetworkDescriptor;
                    }
                }
                else
                {
                    PrintChar( params.NLME_JOIN_confirm.Status );

                    // If we were trying as an orphan, see if we have some more orphan attempts.
                    if (ZigBeeStatus.flags.bits.bTryOrphanJoin)
                    {
                        // If we tried to join as an orphan, we do not have NetworkDescriptor, so we do
                        // not have to free it.

                        TRACE(" Could not join as orphan. " );
                        orphanTries--;
                        if (orphanTries == 0)
                        {
                            TRACE("Must try as new node...\r\n" );
                            ZigBeeStatus.flags.bits.bTryOrphanJoin = 0;
                        }
                        else
                        {
                            TRACE("Trying again...\r\n" );
                        }
                    }
                    else
                    {
                        TRACE(" Could not join selected network. " );
                        currentNetworkDescriptor = currentNetworkDescriptor->next;
                        if (currentNetworkDescriptor)
                        {
                            TRACE("Trying next discovered network...\r\n" );
                            goto SubmitJoinRequest;
                        }
                        else
                        {
                            // We ran out of descriptors.  Free the network descriptor list, and fall
                            // through to try discovery again.
                            TRACE("Cleaning up and retrying discovery...\r\n" );
                            while (NetworkDescriptor)
                            {
                                currentNetworkDescriptor = NetworkDescriptor->next;
                                free( NetworkDescriptor );
                                NetworkDescriptor = currentNetworkDescriptor;
                            }
                        }
                    }
                }
                break;

            case NLME_LEAVE_indication:
                if (!memcmppgm2ram( &params.NLME_LEAVE_indication.DeviceAddress, (ROM void *)&macLongAddr, 8 ))
                {
                    TRACE("We have left the network.\r\n" );
                }
                else
                {
                    TRACE("Another node has left the network.\r\n" );
                }
                currentPrimitive = NO_PRIMITIVE;
                break;

            case NLME_RESET_confirm:
                TRACE("ZigBee Stack has been reset.\r\n" );
                currentPrimitive = NO_PRIMITIVE;
                break;

            case NLME_SYNC_confirm:
                switch (params.NLME_SYNC_confirm.Status)
                {
                    case SUCCESS:
                        // I have heard from my parent, but it has no data for me.  Note that
                        // if my parent has data for me, I will get an APSDE_DATA_indication.
                        TRACE("No data available.\r\n" );
                        break;

                    case NWK_SYNC_FAILURE:
                        // I cannot communicate with my parent.
                        TRACE("I cannot communicate with my parent.\r\n" );
                        break;

                    case NWK_INVALID_PARAMETER:
                        // If we call NLME_SYNC_request correctly, this doesn't occur.
                        TRACE("Invalid sync parameter.\r\n" );
                        break;
                }
                currentPrimitive = NO_PRIMITIVE;
                break;

            case APSDE_DATA_indication:
                {
                    WORD_VAL    attributeId;
                    BYTE        command;
                    BYTE        data;
                    BYTE        dataLength;
                    //BYTE        dataType;
                    BYTE        frameHeader;
                    BYTE        sequenceNumber;
                    BYTE        transaction;
                    BYTE        transByte;

                    currentPrimitive = NO_PRIMITIVE;
                    frameHeader = APLGet(); // dds: AF Frame Pg 77 Figure 19 (Transaction count + Frame type)

                    switch (params.APSDE_DATA_indication.DstEndpoint)
                    {
                        case EP_ZDO:
                            TRACE("  Receiving ZDO cluster " );
                            PrintChar( params.APSDE_DATA_indication.ClusterId );
                            TRACE("\r\n" );

                            // Put code here to handle any ZDO responses that we requested
                            if ((frameHeader & APL_FRAME_TYPE_MASK) == APL_FRAME_TYPE_MSG)
                            {
                                frameHeader &= APL_FRAME_COUNT_MASK;
                                for (transaction=0; transaction<frameHeader; transaction++) // dds: Each transaction within the AF
                                {
                                    sequenceNumber          = APLGet();
                                    dataLength              = APLGet();
                                    transByte               = 1;    // Account for status byte

                                    switch( params.APSDE_DATA_indication.ClusterId )
                                    {

                                        // ********************************************************
                                        // Put a case here to handle each ZDO response that we requested.
                                        // ********************************************************

                                        case NWK_ADDR_rsp:
                                            if (APLGet() == SUCCESS)
                                            {
                                                TRACE("  Receiving NWK_ADDR_rsp.\r\n" );

                                                // Skip over the IEEE address of the responder.
                                                for (data=0; data<8; data++)
                                                {
                                                    APLGet();
                                                    transByte++;
                                                }
                                                destinationAddress.byte.LSB = APLGet();
                                                destinationAddress.byte.MSB = APLGet();
                                                transByte += 2;
                                                myStatusFlags.bits.bDestinationAddressKnown = 1;
                                            }
                                            break;

                                        default:
                                            break;
                                    }

                                    // Read out the rest of the MSG in case there is another transaction.
                                    for (; transByte<dataLength; transByte++)
                                    {
                                        APLGet();
                                    }
                                }
                            }
                            break;

                        // ************************************************************************
                        // Place a case for each user defined endpoint.
                        // ************************************************************************

                        case EP_ZIB:
                            if ((frameHeader & APL_FRAME_TYPE_MASK) == APL_FRAME_TYPE_KVP)
                            {
                                frameHeader &= APL_FRAME_COUNT_MASK;
                                for (transaction=0; transaction<frameHeader; transaction++)
                                {
                                    sequenceNumber          = APLGet();
                                    command                 = APLGet();
                                    attributeId.byte.LSB    = APLGet();
                                    attributeId.byte.MSB    = APLGet();

                                    //dataType = command & APL_FRAME_DATA_TYPE_MASK;
                                    command &= APL_FRAME_COMMAND_MASK;

                                    if ((params.APSDE_DATA_indication.ClusterId == ZIB_DTP_ANALOG_CLUSTER) &&
                                        (attributeId.Val == ATTRIBUTE_ZIB_DTP_ANALOG_VOLTS))
                                    {
                                        if (command == APL_FRAME_COMMAND_GETACK)
                                        {
                                            if (ZigBeeReady())
                                            {
                                                char *ptr;

                                                ZigBeeBlockTx();
                                                TxBuffer[TxData++] = APL_FRAME_TYPE_KVP | 1;    // KVP, 1 transaction
                                                TxBuffer[TxData++] = sequenceNumber;
                                                TxBuffer[TxData++] = APL_FRAME_COMMAND_GET_RES | (ATTRIBUTE_ZIB_DTP_ANALOG_VOLTS_DATATYPE << 4);
                                                TxBuffer[TxData++] = ATTRIBUTE_ZIB_DTP_ANALOG_VOLTS & 0xFF;         // Attribute ID LSB
                                                TxBuffer[TxData++] = (ATTRIBUTE_ZIB_DTP_ANALOG_VOLTS >> 8) & 0xFF;  // Attribute ID MSB

                                                TxBuffer[TxData++] = KVP_SUCCESS;

                                                // read the AD value
                                                ptr = (char *) &(TxBuffer[TxData]);
                                                ptr++;
                                                TxBuffer[TxData] = GetADCString( ptr );
                                                TxData += TxBuffer[TxData] + 1;

                                                if (params.APSDE_DATA_indication.SrcAddrMode == APS_ADDRESS_NOT_PRESENT)
                                                {
                                                    params.APSDE_DATA_request.DstAddrMode = APS_ADDRESS_NOT_PRESENT;
                                                }
                                                else
                                                {
                                                    params.APSDE_DATA_request.DstAddrMode = APS_ADDRESS_16_BIT;
                                                    params.APSDE_DATA_request.DstEndpoint = params.APSDE_DATA_indication.SrcEndpoint;
                                                    params.APSDE_DATA_request.DstAddress.ShortAddr = params.APSDE_DATA_indication.SrcAddress.ShortAddr;
                                                }
                                                // If we use an APS address table, we should check for 64-bit addresses as well.

                                                //params.APSDE_DATA_request.asduLength; TxData
                                                // Profile ID unchanged params.APSDE_DATA_request.ProfileId.Val
                                                params.APSDE_DATA_request.RadiusCounter = DEFAULT_RADIUS;
                                                params.APSDE_DATA_request.DiscoverRoute = ROUTE_DISCOVERY_ENABLE;
                                                params.APSDE_DATA_request.TxOptions.Val = 0;
                                                params.APSDE_DATA_request.SrcEndpoint = EP_ZIB;
                                                // Cluster ID unchanged params.APSDE_DATA_request.ClusterId

                                                TRACE(" Trying to send AD value.\r\n" );

                                                currentPrimitive = APSDE_DATA_request;
                                            }
                                        }
                                    }
                                    
                                    if ((params.APSDE_DATA_indication.ClusterId == ZIB_DTP_SENSORS_CLUSTER) &&
                                        (attributeId.Val == ATTRIBUTE_ZIB_DTP_SENSORS_TEMPERATURE))
                                    {
                                        if (command == APL_FRAME_COMMAND_GETACK)
                                        {
                                            if (ZigBeeReady())
                                            {
                                                char *ptr;

                                                ZigBeeBlockTx();
                                                TxBuffer[TxData++] = APL_FRAME_TYPE_KVP | 1;    // KVP, 1 transaction
                                                TxBuffer[TxData++] = sequenceNumber;
                                                TxBuffer[TxData++] = APL_FRAME_COMMAND_GET_RES | (ATTRIBUTE_ZIB_DTP_SENSORS_TEMPERATURE_DATATYPE << 4);
                                                TxBuffer[TxData++] = ATTRIBUTE_ZIB_DTP_SENSORS_TEMPERATURE & 0xFF;         // Attribute ID LSB
                                                TxBuffer[TxData++] = (ATTRIBUTE_ZIB_DTP_SENSORS_TEMPERATURE >> 8) & 0xFF;  // Attribute ID MSB

                                                TxBuffer[TxData++] = KVP_SUCCESS;

                                                ptr = (char *) &(TxBuffer[TxData]);
                                                ptr++;
                                                TxBuffer[TxData] = GetTC77String( ptr );
                                                #ifdef DEBUG_ACTIVE
                                                	ConsolePutString( (BYTE *)ptr );
												#endif
                                                TxData += TxBuffer[TxData] + 1;

                                                if (params.APSDE_DATA_indication.SrcAddrMode == APS_ADDRESS_NOT_PRESENT)
                                                {
                                                    params.APSDE_DATA_request.DstAddrMode = APS_ADDRESS_NOT_PRESENT;
                                                }
                                                else
                                                {
                                                    params.APSDE_DATA_request.DstAddrMode = APS_ADDRESS_16_BIT;
                                                    params.APSDE_DATA_request.DstEndpoint = params.APSDE_DATA_indication.SrcEndpoint;
                                                    params.APSDE_DATA_request.DstAddress.ShortAddr = params.APSDE_DATA_indication.SrcAddress.ShortAddr;
                                                }
                                                // If we use an APS address table, we should check for 64-bit addresses as well.

                                                //params.APSDE_DATA_request.asduLength; TxData
                                                // Profile ID unchanged params.APSDE_DATA_request.ProfileId.Val
                                                params.APSDE_DATA_request.RadiusCounter = DEFAULT_RADIUS;
                                                params.APSDE_DATA_request.DiscoverRoute = ROUTE_DISCOVERY_ENABLE;
                                                params.APSDE_DATA_request.TxOptions.Val = 0;
                                                params.APSDE_DATA_request.SrcEndpoint = EP_ZIB;
                                                // Cluster ID unchanged params.APSDE_DATA_request.ClusterId

                                                TRACE(" Trying to send temperature message.\r\n");

                                                currentPrimitive = APSDE_DATA_request;
                                            }
                                        }
                                    }
                                    
                                    if ((params.APSDE_DATA_indication.ClusterId == ZIB_DTP_ACTUATORS_CLUSTER) &&
                                        (attributeId.Val == ATTRIBUTE_ZIB_DTP_ACTUATORS_LED1 || attributeId.Val == ATTRIBUTE_ZIB_DTP_ACTUATORS_LED2)) // TODO: split in two leds
                                    {
                                        if ((command == APL_FRAME_COMMAND_SET) ||
                                            (command == APL_FRAME_COMMAND_SETACK))
                                        {
                                            // Prepare a response in case it is needed.
                                            TxBuffer[TxData++] = APL_FRAME_TYPE_KVP | 1;    // KVP, 1 transaction
                                            TxBuffer[TxData++] = sequenceNumber;
                                            TxBuffer[TxData++] = APL_FRAME_COMMAND_SET_RES | (APL_FRAME_DATA_TYPE_UINT8 << 4);
                                            TxBuffer[TxData++] = attributeId.byte.LSB;
                                            TxBuffer[TxData++] = attributeId.byte.MSB;

                                            // Data type for this attibute must be APL_FRAME_DATA_TYPE_UINT8
                                            data = APLGet();
                                            switch (data)
                                            {
                                                case LIGHT_OFF:
                                                    TRACE(" Turning light off.\r\n" );
                                                    MESSAGE_INDICATION = 0;
                                                    TxBuffer[TxData++] = SUCCESS;
                                                    break;
                                                case LIGHT_ON:
                                                    TRACE(" Turning light on.\r\n" );
                                                    MESSAGE_INDICATION = 1;
                                                    TxBuffer[TxData++] = SUCCESS;
                                                    break;
                                                case LIGHT_TOGGLE:
                                                    TRACE(" Toggling light.\r\n" );
                                                    MESSAGE_INDICATION ^= 1;
                                                    TxBuffer[TxData++] = SUCCESS;
                                                    break;
                                                default:
                                                    PrintChar( data );
                                                    TRACE(" Invalid light message.\r\n" );
                                                    TxBuffer[TxData++] = KVP_INVALID_ATTRIBUTE_DATA;
                                                    break;
                                            }
                                        }
                                        if (command == APL_FRAME_COMMAND_SETACK)
                                        {
                                            // Send back an application level acknowledge.
                                            ZigBeeBlockTx();

                                            // Take care here that parameters are not overwritten before they are used.
                                            // We can use the data byte as a temporary variable.
                                            params.APSDE_DATA_request.DstAddrMode = params.APSDE_DATA_indication.SrcAddrMode;
                                            params.APSDE_DATA_request.DstEndpoint = params.APSDE_DATA_indication.SrcEndpoint;
                                            params.APSDE_DATA_request.DstAddress.ShortAddr = params.APSDE_DATA_indication.SrcAddress.ShortAddr;

                                            //params.APSDE_DATA_request.asduLength; TxData
                                            //params.APSDE_DATA_request.ProfileId; unchanged
                                            params.APSDE_DATA_request.RadiusCounter = DEFAULT_RADIUS;
                                            params.APSDE_DATA_request.DiscoverRoute = ROUTE_DISCOVERY_ENABLE;
                                            params.APSDE_DATA_request.TxOptions.Val = 0;
                                            params.APSDE_DATA_request.SrcEndpoint = EP_ZIB;
                                            //params.APSDE_DATA_request.ClusterId; unchanged

                                            currentPrimitive = APSDE_DATA_request;
                                        }
                                        else
                                        {
                                            // We are not sending an acknowledge, so reset the transmit message pointer.
                                            TxData = TX_DATA_START;
                                        }
                                    }
                                    // TODO read to the end of the transaction.
                                } // each transaction
                            } // frame type
                            break;
                        default:
                            // If the command type was something that requested an acknowledge, we could send back
                            // KVP_INVALID_ENDPOINT here.
                            break;
                    }

                    APLDiscardRx();
                }
                break;

            case APSDE_DATA_confirm:
                if (params.APSDE_DATA_confirm.Status)
                {
                    TRACE("Error " );
                    PrintChar( params.APSDE_DATA_confirm.Status );
                    TRACE(" sending message.\r\n" );
                }
                else
                {
                    TRACE(" Message sent successfully.\r\n" );
                }
                currentPrimitive = NO_PRIMITIVE;
                break;

            case NO_PRIMITIVE:
				// dds (ConsoleInterrupt): Console commands -> Let's check them before trying to join a network, giving the chance to change the channel if we can't join the default channel mask
				if (myStatusFlags.bits.bConsoleKeyReceived && /*ZigBeeStatus.flags.bits.bDataRequestComplete &&*/ ZigBeeReady())
				{
					myStatusFlags.bits.bConsoleKeyReceived = FALSE;
					switch (g_byteLastConsoleKey) // commands based on the last destination address
					{
						case 'r': // reset
							InitInternalVariables();
							currentPrimitive = NLME_RESET_request;
							break;
	
						case 'c': // channel
							{
								char szChannel[3] = {0};
								int indexBytesChannel = 0;
								int iChannel;
								
								TRACE("2 Digit Channel#: ");
								
								while (indexBytesChannel < 2) // Get two bytes
								{
									while(!ConsoleIsGetReady()) CLRWDT(); // feed the watch dog
									szChannel[indexBytesChannel] = ConsoleGet();
									if( szChannel[indexBytesChannel] >= '0' && szChannel[indexBytesChannel] <= '9' ) // a number
									{
											#ifdef DEBUG_ACTIVE
												ConsolePut(szChannel[indexBytesChannel++]);
											#endif
									}
								}
	
								TRACE("\r\n");
	
								iChannel = atoi(szChannel);
								if( iChannel && (iChannel < 11 || iChannel > 26)) // Channel 0 is allowed for a full scan
								{
									TRACE("Invalid channel ignored: ");
									PrintChar((BYTE)iChannel);
									TRACE("\r\n");
									break;
								}
	
								// reset the internal variable states, 
								// but keep the last selected channel, to be used in the NLME_NETWORK_FORMATION_request (above)
								// remember to clear it in NLME_NETWORK_FORMATION_confirm 
								
								InitInternalVariables();
								
								if(iChannel)
								{
									g_dwLastChannelMask = (DWORD)1 << iChannel;
								}
								else
								{
									g_dwLastChannelMask = POSSIBLE_CHANNEL_MASK; // All channels allowed
								}
								
								TRACE("New channel mask: 0x");
								PrintChar(*((BYTE*)(&g_dwLastChannelMask)+3));
								PrintChar(*((BYTE*)(&g_dwLastChannelMask)+2));
								PrintChar(*((BYTE*)(&g_dwLastChannelMask)+1));
								PrintChar((BYTE)g_dwLastChannelMask);
								TRACE("\r\n");
								
								currentPrimitive = NLME_RESET_request;
								break;
							}
					}

					// We've processed any console key press, so re-enable interrupts.
					RCIE = 1; // dds (ConsoleInterrupt)
				}            
                if (!ZigBeeStatus.flags.bits.bNetworkJoined)
                {
                    if (!ZigBeeStatus.flags.bits.bTryingToJoinNetwork)
                    {
                        if (ZigBeeStatus.flags.bits.bTryOrphanJoin)
                        {
                            TRACE("Trying to join network as an orphan...\r\n" );
                            params.NLME_JOIN_request.JoinAsRouter           = FALSE;
                            params.NLME_JOIN_request.RejoinNetwork          = TRUE;
                            params.NLME_JOIN_request.PowerSource            = NOT_MAINS_POWERED;
                            params.NLME_JOIN_request.RxOnWhenIdle           = FALSE;
                            params.NLME_JOIN_request.MACSecurity            = FALSE;
                            params.NLME_JOIN_request.ScanDuration           = 8;
                            params.NLME_JOIN_request.ScanChannels.Val       = g_dwLastChannelMask? g_dwLastChannelMask: ALLOWED_CHANNELS; // dds: If I had selected another channel, then use it;
                            currentPrimitive = NLME_JOIN_request;
                        }
                        else
                        {
                            TRACE("Trying to join network as a new device...\r\n" );
                            params.NLME_NETWORK_DISCOVERY_request.ScanDuration          = 8; // dds: raised to 8
                            params.NLME_NETWORK_DISCOVERY_request.ScanChannels.Val      = g_dwLastChannelMask? g_dwLastChannelMask: ALLOWED_CHANNELS; // dds: If I had selected another channel, then use it;;
                            currentPrimitive = NLME_NETWORK_DISCOVERY_request;
                        }
                    }
                }
                else
                {
                    // See if I can do my own internal tasks.  We don't want to try to send a message
                    // if we just asked for one.
                    if (ZigBeeStatus.flags.bits.bDataRequestComplete && ZigBeeReady())
                    {

                        // ************************************************************************
                        // Place all processes that can send messages here.  Be sure to call
                        // ZigBeeBlockTx() when currentPrimitive is set to APSDE_DATA_request.
                        // ************************************************************************
                        
						if (myStatusFlags.bits.bWakeupSwitchToggled) // dds: changed this key to be my "wake-up" key, to give a chance to use the keyboard
						{
							myStatusFlags.bits.bWakeupSwitchToggled = FALSE;
							iIndex = 60;
							while(iIndex-- && !myStatusFlags.bits.bConsoleKeyReceived)
							{
							Delay10KTCYx( 200 ); // Delay 3 seconds to give a chance of a console key press
							}
                        }

                        // We've processed any key press, so re-enable interrupts.
                        RBIE = 1;
                    }

                    // If we don't have to execute a primitive, see if we need to request data from
                    // our parent, or if we can go to sleep.
                    if (currentPrimitive == NO_PRIMITIVE)
                    {
                        if (!ZigBeeStatus.flags.bits.bDataRequestComplete)
                        {
                            // We have not received all data from our parent.  If we are not waiting
                            // for an answer from a data request, send a data request.
                            if (!ZigBeeStatus.flags.bits.bRequestingData)
                            {
                                if (ZigBeeReady())
                                {
                                    // Our parent still may have data for us.
                                    params.NLME_SYNC_request.Track = FALSE;
                                    currentPrimitive = NLME_SYNC_request;
                                    TRACE("Requesting data...\r\n" );
                                }
                            }
                        }
                        else
                        {
                            if (!ZigBeeStatus.flags.bits.bHasBackgroundTasks && myProcessesAreDone())
                            {
                                // We do not have a primitive to execute, we've extracted all messages
                                // that our parent has for us, the stack has no background tasks,
                                // and all application-specific processes are complete.  Now we can
                                // go to sleep.  Make sure that the UART is finished, turn off the transceiver,
                                // and make sure that we wakeup from key press.
                                TRACE("Going to sleep...\r\n" );
                                while (!ConsoleIsPutReady());
                                APLDisable(); // disable the transceiver
                                RBIE = 1;
                                SLEEP();
                                NOP();


                                // We just woke up from sleep, due to WDT timeout. Turn on the transceiver and
                                // request data from our parent.
                                APLEnable(); // enable the transceiver
                                params.NLME_SYNC_request.Track = FALSE;
                                currentPrimitive = NLME_SYNC_request;
                                TRACE("Requesting data after sleep...\r\n" );
                            }
                        }
                    }
                }
                break;

            default:
                PrintChar( currentPrimitive );
                TRACE(" Unhandled primitive.\r\n" );
                currentPrimitive = NO_PRIMITIVE;
                break;
        }

        // *********************************************************************
        // Place any non-ZigBee related processing here.  Be sure that the code
        // will loop back and execute ZigBeeTasks() in a timely manner.
        // *********************************************************************
    }
}

/*******************************************************************************
myProcessesAreDone

This routine should contain any tests that are required by the application to
confirm that it can go to sleep.  If the application can go to sleep, this
routine should return TRUE.  If the application is still busy, this routine
should return FALSE.
*******************************************************************************/

BOOL myProcessesAreDone( void )
{
    return (myStatusFlags.bits.bWakeupSwitchToggled==FALSE) && (myStatusFlags.bits.bLightSwitchToggled==FALSE) && (myStatusFlags.bits.bConsoleKeyReceived==FALSE);
}

/*******************************************************************************
HardwareInit

All port directioning and SPI must be initialized before calling ZigBeeInit().

For demonstration purposes, required signals are configured individually.
*******************************************************************************/
void HardwareInit(void)
{
    //-------------------------------------------------------------------------
    // This section is required to initialize the PICDEM Z for the transceiver
    // and the ZigBee Stack.
    //-------------------------------------------------------------------------
	
	OpenADC(	ADC_FOSC_32 & ADC_RIGHT_JUST & ADC_12_TAD,
				ADC_CH4 & ADC_INT_OFF & ADC_VREFPLUS_VDD & ADC_VREFMINUS_VSS, // CH4/AN4/RA5/pin7
				15 );
	Delay10TCYx( 5 ); // Delay for 50TCY


    SPIInit();

    #if (RF_CHIP == MRF24J40)
        // Start with MRF24J40 disabled and not selected
        PHY_CS              = 1;
        PHY_RESETn          = 1;

        // Set the directioning for the MRF24J40 pin connections.
        PHY_CS_TRIS         = 0;
        PHY_RESETn_TRIS     = 0;

        // Initialize the interrupt.
        INTCON2bits.INTEDG0 = 0;
    #elif (RF_CHIP==UZ2400)
        // Start with UZ2400 disabled and not selected
        PHY_SEN             = 1;
        PHY_RESETn          = 1;

        // Set the directioning for the UZ2400 pin connections.
        PHY_SEN_TRIS        = 0;
        PHY_RESETn_TRIS     = 0;

        // Initialize the interrupt.
        INTCON2bits.INTEDG0 = 0;
    #elif (RF_CHIP==CC2420)
        // CC2420 I/O assignments with respect to PIC:
        //NOTE: User must make sure that pin is capable of correct digital operation.
        //      This may require modificaiton of which pins are digital and analog.
        //NOTE: The stack requires that the SPI interface be located on LATC3 (SCK),
        //      RC4 (SO), and LATC5 (SI).
        //NOTE: The appropriate config bit must be set such that FIFOP is the CCP2
        //      input pin. The stack uses the CCP2 interrupt.

        // Start with CC2420 disabled and not selected
        PHY_CSn             = 1;
        PHY_VREG_EN         = 0;
        PHY_RESETn          = 1;

        // Set the directioning for the CC2420 pin connections.
        PHY_FIFO_TRIS       = 1;    // FIFO      (Input)
        PHY_SFD_TRIS        = 1;    // SFD       (Input - Generates interrupt on falling edge)
        PHY_FIFOP_TRIS      = 1;    // FIFOP     (Input - Used to detect overflow, CCP2 interrupt)
        PHY_CSn_TRIS        = 0;    // CSn       (Output - to select CC2420 SPI slave)
        PHY_VREG_EN_TRIS    = 0;    // VREG_EN   (Output - to enable CC2420 voltage regulator)
        PHY_RESETn_TRIS     = 0;    // RESETn    (Output - to reset CC2420)
    #endif

    // Initialize the SPI pins and directions
    LATC3               = 1;    // SCK
    LATC5               = 1;    // SDO
    TRISC3              = 0;    // SCK
    TRISC4              = 1;    // SDI
    TRISC5              = 0;    // SDO

    // Initialize the SPI module
    SSPSTAT = 0xC0;
    SSPCON1 = 0x20;

    //-------------------------------------------------------------------------
    // This section is required for application-specific hardware
    // initialization.
    //-------------------------------------------------------------------------

    // D1 and D2 are on RA0 and RA1 respectively, and CS of the TC77 is on RA2.
    // Make PORTA digital I/O.
    ADCON1 = 0x0F;

    // Deselect the TC77 temperature sensor (RA2)
    LATA = 0x04;

    // Make RA0, RA1, RA2 and RA4 outputs.
    TRISA = 0xE0;

    // Clear the RBIF flag (INTCONbits.RBIF)
    RBIF = 0;

    // Enable PORTB pull-ups (INTCON2bits.RBPU)
    RBPU = 0;

    // Make the PORTB switch connections inputs.
    TRISB4 = 1;
    TRISB5 = 1;
}


//******************************************************************************
// ReadADC
//******************************************************************************
static BYTE GetADCString( char *pstrValueRead )
{
	int iVolts;
	float fVolts;
	char szTrace[50];
	
	TRACE("GetADCString: Initializaing the ADC on channel 4\r\n");
	
	ConvertADC(); // Start conversion
	while( BusyADC() ); // Wait for completion
	iVolts = ReadADC();
	//CloseADC(); // Disable A/D converter

	fVolts = (iVolts/1024)*(3.3);
	// sprintf(pstrValueRead, (ROM char*)"%f", fVolts);
	sprintf(pstrValueRead, (ROM char*)"%d", iVolts);
	sprintf(szTrace, (ROM char*)"iVolts='%d' fVolts='%f' pstrValueRead='%s'", iVolts, fVolts, pstrValueRead);
	#ifdef DEBUG_ACTIVE
		ConsolePutString((BYTE *)szTrace);
	#endif
	return strlen(pstrValueRead);
}

/*******************************************************************************
User Interrupt Handler

The stack uses some interrupts for its internal processing.  Once it is done
checking for its interrupts, the stack calls this function to allow for any
additional interrupt processing.
*******************************************************************************/

void UserInterruptHandler(void)
{

    // *************************************************************************
    // Place any application-specific interrupt processing here
    // *************************************************************************

    // Is this a interrupt-on-change interrupt?
    if ( RBIF == 1 )
    {
        // Record which button was pressed so the main() loop can
        // handle it
        if (WAKEUP_SWITCH == 0)
            myStatusFlags.bits.bWakeupSwitchToggled = TRUE;

        if (LIGHT_SWITCH == 0)
            myStatusFlags.bits.bLightSwitchToggled = TRUE;

        // Disable further RBIF until we process it
        RBIE = 0;

        // Clear mis-match condition and reset the interrupt flag
        LATB = PORTB;

        RBIF = 0;
    }
	
	if (ConsoleIsGetReady()) // Tests the console's RCIF
	{
		g_byteLastConsoleKey = ConsoleGet();
		myStatusFlags.bits.bConsoleKeyReceived = TRUE;
		RCIE = 0; // disable further RCIF until we process it
		RCIF = 0;
	}
}
