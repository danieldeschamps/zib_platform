/*
    Microchip ZigBee Stack

    Demo Coordinator

    This demonstration shows how a ZigBee coordinator can be set up.  This demo allows
    the PICDEM Z Demostration Board to act as either a "Switching Load Controller"
    (e.g. a light) or a "Switching Remote Control" (e.g. a switch) as defined by
    the Home Controls, Lighting profile.  It is designed to interact with a
    second PICDEM Z programmed with the Demo RFD project.

    To give the PICDEM Z "switch" capability, uncomment the I_AM_SWITCH definition
    below.  To give the PICDEM Z "light" capability, uncomment the I_AM_LIGHT
    definition below.  The PICDEM Z may have both capabilities enabled.  Be sure
    that the corresponding Demo RFD device is programmed with complementary
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
 * FileName:        Coordinator.c
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

#define I_AM_LIGHT
#define I_AM_SWITCH
#define EP_TEMPERATURE_RFD  3  // For demonstration only.  We really should discover this.

//******************************************************************************
// Constants
//******************************************************************************

#define TEMPERATURE_SWITCH          RB4
#define LIGHT_SWITCH                RB5

#define LED_UNNUSED          		LATA0
#define LED_LIGHT          			LATA1

#define BIND_STATE_BOUND            0
#define BIND_STATE_TOGGLE           1
#define BIND_STATE_UNBOUND          1
#define BIND_WAIT_DURATION          (5*ONE_SECOND)

#define LIGHT_OFF                   0x00
#define LIGHT_ON                    0xFF
#define LIGHT_TOGGLE                0xF0


//******************************************************************************
// Function Prototypes
//******************************************************************************

void HardwareInit( void );
void InitInternalVariables(void);

//******************************************************************************
// Application Variables
//******************************************************************************

// uChip
ZIGBEE_PRIMITIVE    currentPrimitive;
SHORT_ADDR          destinationAddress;

static union
{
    struct
    {
        BYTE    bBroadcastSwitchToggled    : 1;
        BYTE    bTemperatureSwitchToggled  : 1;
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
#define bBindSwitchToggled      bBroadcastSwitchToggled

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
//******************************************************************************
//******************************************************************************

void InitInternalVariables(void)
{
	// uChip
	myStatusFlags.Val = STATUS_FLAGS_INIT;
	destinationAddress.Val = 0x796F;    // Default to first RFD
	LED_UNNUSED = 0;
	LATA4 = 0;

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

    // If you are going to send data to a terminal, initialize the UART.
    ConsoleInit();

    ConsolePutROMString( (ROM char *)"\r\n\r\n\r\n*************************************\r\n" );
    ConsolePutROMString( (ROM char *)"Microchip ZigBee(TM) Stack - v1.0-3.6\r\n\r\n" );
    ConsolePutROMString( (ROM char *)"ZigBee Coordinator, ZiB Platform\r\n\r\n" );
    #if (RF_CHIP == MRF24J40)
        ConsolePutROMString( (ROM char *)"Transceiver-MRF24J40\r\n\r\n" );
    #elif (RF_CHIP==UZ2400)
        ConsolePutROMString( (ROM char *)"Transceiver-UZ2400\r\n\r\n" );
    #elif (RF_CHIP==CC2420)
        ConsolePutROMString( (ROM char *)"Transceiver-CC2420\r\n\r\n" );
    #else
        ConsolePutROMString( (ROM char *)"Transceiver-Unknown\r\n\r\n" );
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
            case NLME_NETWORK_FORMATION_confirm:
                if (!params.NLME_NETWORK_FORMATION_confirm.Status)
                {
					ConsolePutROMString( (ROM char *)"PAN " );
                    PrintChar( macPIB.macPANId.byte.MSB );
                    PrintChar( macPIB.macPANId.byte.LSB );
                    ConsolePutROMString( (ROM char *)" started successfully.\r\n" );
                    params.NLME_PERMIT_JOINING_request.PermitDuration = 0xFF;   // No Timeout
                    currentPrimitive = NLME_PERMIT_JOINING_request;
                }
                else
                {
                    PrintChar( params.NLME_NETWORK_FORMATION_confirm.Status );
                    ConsolePutROMString( (ROM char *)" Error forming network.  Trying again...\r\n" );
                    currentPrimitive = NO_PRIMITIVE;
                }
                break;

            case NLME_PERMIT_JOINING_confirm:
                if (!params.NLME_PERMIT_JOINING_confirm.Status)
                {
                    ConsolePutROMString( (ROM char *)"Joining permitted.\r\n" );
                    currentPrimitive = NO_PRIMITIVE;
                }
                else
                {
                    PrintChar( params.NLME_PERMIT_JOINING_confirm.Status );
                    ConsolePutROMString( (ROM char *)" Join permission unsuccessful. We cannot allow joins.\r\n" );
                    currentPrimitive = NO_PRIMITIVE;
                }
                break;

            case NLME_JOIN_indication: // dds: devices which join directly to the coordinator are signaled here. (not the router joiners)
                ConsolePutROMString( (ROM char *)"Node " );
                PrintChar( params.NLME_JOIN_indication.ShortAddress.byte.MSB );
                PrintChar( params.NLME_JOIN_indication.ShortAddress.byte.LSB );
                ConsolePutROMString( (ROM char *)" just joined.\r\n" );
                
				tableDiscovery[iDiscoveryEntryCount].addr.Val = params.NLME_JOIN_indication.ShortAddress.Val;
                iDiscoveryEntryCount++;

                ConsolePutROMString( (ROM char *)"Node Count= " );
                PrintChar( iDiscoveryEntryCount );
                ConsolePutROMString( (ROM char *)" \r\n Node List = " );
                for( iIndex=0 ; iIndex < iDiscoveryEntryCount ; iIndex++ )
                {
					PrintChar( tableDiscovery[iIndex].addr.byte.MSB );
					PrintChar( tableDiscovery[iIndex].addr.byte.LSB );
	                ConsolePutROMString( (ROM char *)" " );
	            }
				ConsolePutROMString( (ROM char *)"\r\n" );

                currentPrimitive = NO_PRIMITIVE;
                break;

            case NLME_LEAVE_indication:
                if (!memcmppgm2ram( &params.NLME_LEAVE_indication.DeviceAddress, (ROM void *)&macLongAddr, 8 ))
                {
                    ConsolePutROMString( (ROM char *)"We have left the network.\r\n" );
                }
                else
                {
                    ConsolePutROMString( (ROM char *)"Another node has left the network.\r\n" );
                }
                currentPrimitive = NO_PRIMITIVE;
                break;

            case NLME_RESET_confirm:
                ConsolePutROMString( (ROM char *)"ZigBee Stack has been reset.\r\n" );
                currentPrimitive = NO_PRIMITIVE;
                break;

            case APSDE_DATA_confirm:
                if (params.APSDE_DATA_confirm.Status)
                {
                    ConsolePutROMString( (ROM char *)"Error " );
                    PrintChar( params.APSDE_DATA_confirm.Status );
                    ConsolePutROMString( (ROM char *)" sending message.\r\n" );
                }
                else
                {
                    ConsolePutROMString( (ROM char *)" Message sent successfully.\r\n" );
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
                    BYTE        errorCode;
                    BYTE        frameHeader;
                    BYTE        sequenceNumber;
                    BYTE        transaction;
                    BYTE        transByte;

                    currentPrimitive = NO_PRIMITIVE;
                    frameHeader = APLGet(); // dds: AF Frame Pg 77 Figure 19 (Transaction count + Frame type)

                    switch (params.APSDE_DATA_indication.DstEndpoint)
                    {
                        case EP_ZDO:
                            ConsolePutROMString( (ROM char *)"  Receiving ZDO cluster " );
                            PrintChar( params.APSDE_DATA_indication.ClusterId );
                            ConsolePutROMString( (ROM char *)"\r\n" );

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

                                        #ifndef USE_BINDINGS
                                        case NWK_ADDR_rsp:
                                            if (APLGet() == SUCCESS)
                                            {
                                                ConsolePutROMString( (ROM char *)"Receiving NWK_ADDR_rsp.\r\n" );

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
                                        #endif

										// dds: case nodes discovery stuff
										case SIMPLE_DESC_rsp: // dds: In cluster pg 115 Table 67
											ConsolePutROMString( (ROM char *)"SIMPLE_DESC_rsp: Status=<" );
											switch( APLGet() ) // Status byte
											{
												case SUCCESS:
													{
														BYTE *pDescriptorWriter,byteEP;
														int iCounter, indexDiscoveryEntry, indexDescriptorEntry;
														char szDescriptorLog[64];
														SHORT_ADDR addrRsp;

														ConsolePutROMString( (ROM char *)"Success>\r\n" );
														
														// NWKAddrOfInterest
														addrRsp.byte.LSB = APLGet();
														addrRsp.byte.MSB = APLGet();
														transByte+=2;
														
														// Find the proper discovery entry (created when the node joins)
														for (iIndex=0; iIndex<iDiscoveryEntryCount; iIndex++)
														{
															if (addrRsp.Val == tableDiscovery[iIndex].addr.Val)
															{
																indexDiscoveryEntry = iIndex;
																break;
															}
														}
														
														// Descriptor Length
														APLGet();
														transByte++;
														
														// Simple Descriptor: Table 29 pg 73
														byteEP = APLGet(); // Endpoint number
														transByte++;
														
														// Search for this EP entry, and overwrite if existent
														indexDescriptorEntry = -1;
														for (iIndex=0; iIndex < MAX_DESCRIPTOR_ENTRY && iIndex < tableDiscovery[indexDiscoveryEntry].iDescriptorEntryCount; iIndex++)
														{
															if (tableDiscovery[indexDiscoveryEntry].descriptor[iIndex].Endpoint == byteEP)
															{
																indexDescriptorEntry = iIndex; // found
																break;
															}
														}
														
														// Choose the position to write
														if (indexDescriptorEntry == -1) // did not find this EP simple descriptor
														{
															if (tableDiscovery[indexDiscoveryEntry].iDescriptorEntryCount == MAX_DESCRIPTOR_ENTRY) // the table is full
															{
																indexDescriptorEntry = 0; // if the table is full, overwrite the first position
															}
															else
															{
																indexDescriptorEntry = tableDiscovery[indexDiscoveryEntry].iDescriptorEntryCount++; // use the next available position and increment the counter
															}
														}
														
														pDescriptorWriter = (BYTE*)&tableDiscovery[indexDiscoveryEntry].descriptor[indexDescriptorEntry];
														
														*pDescriptorWriter++ = byteEP;
														for (iIndex=0; iIndex<6; iIndex++, transByte++)
														{
															*pDescriptorWriter++ = APLGet(); // From 'Application profile identifier' to 'Application input cluster count'
														}
														
														iCounter = *(pDescriptorWriter-1);
														for (iIndex=0; iIndex<iCounter; iIndex++, transByte++)
														{
															*pDescriptorWriter++ = APLGet(); // 'Application input cluster list'
														}

														pDescriptorWriter = (BYTE*)&(tableDiscovery[indexDiscoveryEntry].descriptor[indexDescriptorEntry].AppOutClusterCount); // Jump spaces in the cluster array
														*pDescriptorWriter++ = APLGet(); // 'Application output cluster count'

														iCounter = *(pDescriptorWriter-1);
														for (iIndex=0; iIndex<iCounter; iIndex++, transByte++)
														{
															*pDescriptorWriter++ = APLGet(); // Application output cluster list
														}

														ConsolePutROMString( (ROM char *)"Node <" );
														PrintChar (tableDiscovery[indexDiscoveryEntry].addr.byte.MSB);
														PrintChar (tableDiscovery[indexDiscoveryEntry].addr.byte.LSB);
														ConsolePutROMString( (ROM char *)"> InClusters: " );
														iCounter=tableDiscovery[indexDiscoveryEntry].descriptor[indexDescriptorEntry].AppInClusterCount;
														for (iIndex=0; iIndex<iCounter; iIndex++)
														{
															PrintChar (tableDiscovery[indexDiscoveryEntry].descriptor[indexDescriptorEntry].AppInClusterList[iIndex]);
															ConsolePutROMString( (ROM char *)" " );
														}
														
														ConsolePutROMString( (ROM char *)"| OutClusters: " );
														iCounter=tableDiscovery[indexDiscoveryEntry].descriptor[indexDescriptorEntry].AppOutClusterCount;
														for (iIndex=0; iIndex<iCounter; iIndex++)
														{
															PrintChar (tableDiscovery[indexDiscoveryEntry].descriptor[indexDescriptorEntry].AppOutClusterList[iIndex]);
															ConsolePutROMString( (ROM char *)" " );
														}
														ConsolePutROMString( (ROM char *)"\r\n" );													
													}
													break;

												case ZDO_INVALID_EP:
													ConsolePutROMString( (ROM char *)"Invalid EP>\r\n" );
													break;

												case ZDO_NOT_ACTIVE:
													ConsolePutROMString( (ROM char *)"Not Active>\r\n" );
													break;

												case ZDO_DEVICE_NOT_FOUND:
													ConsolePutROMString( (ROM char *)"Device not found>\r\n" );
													break;

												default:
													ConsolePutROMString( (ROM char *)"Unknown>\r\n" );
													break;
											}
											break;

										case ACTIVE_EP_rsp: // dds: In cluster pg 116 Table 68
											ConsolePutROMString( (ROM char *)"ACTIVE_EP_rsp: Status=<" );
											switch( APLGet() ) // Status byte
											{
												case SUCCESS:
													{
														// BYTE *pDescriptorWriter;
														int iCounter;
														// char szDescriptorLog[64];
														SHORT_ADDR addrRsp;

														ConsolePutROMString( (ROM char *)"Success>\r\n" );

														// NWKAddrOfInterest
														addrRsp.byte.LSB = APLGet();
														addrRsp.byte.MSB = APLGet();
														transByte+=2;

														/*for (iIndex=0; iIndex<iDiscoveryEntryCount; iIndex++)
														{
															if (addrRsp.Val == tableDiscovery[iIndex].addr.Val)
															{
																indexDiscoveryEntry = iIndex;
																break;
															}
														}*/

														// ActiveEPCount
														iCounter = APLGet();
														transByte++;

														// ActiveEPList
														TRACE("Active EPs: ");
														for (iIndex=0; iIndex < iCounter; iIndex++, transByte++)
														{
															PrintChar(APLGet()); // dds: TODO - Store this information
															TRACE(" ");
														}
														TRACE("\r\n");
														
														
														/*pDescriptorWriter = (BYTE*)&tableDiscovery[indexDiscoveryEntry].descriptor;
														for (iIndex=0; iIndex<7; iIndex++, transByte++)
														{
															*pDescriptorWriter++ = APLGet(); // From Endpoint to Application input cluster count
														}

														iCounter = *(pDescriptorWriter-1);
														for (iIndex=0; iIndex<iCounter; iIndex++, transByte++)
														{
															*pDescriptorWriter++ = APLGet(); // Application input cluster list
														}

														pDescriptorWriter = (BYTE*)&tableDiscovery[indexDiscoveryEntry].descriptor.AppOutClusterCount; // Jump spaces in the cluster array
														*pDescriptorWriter++ = APLGet(); // Application output cluster count

														iCounter = *(pDescriptorWriter-1);
														for (iIndex=0; iIndex<iCounter; iIndex++, transByte++)
														{
															*pDescriptorWriter++ = APLGet(); // Application output cluster list
														}

														ConsolePutROMString( (ROM char *)"Node <" );
														PrintChar (tableDiscovery[indexDiscoveryEntry].addr.byte.MSB);
														PrintChar (tableDiscovery[indexDiscoveryEntry].addr.byte.LSB);
														ConsolePutROMString( (ROM char *)"> InClusters: " );
														iCounter=tableDiscovery[indexDiscoveryEntry].descriptor.AppInClusterCount;
														for (iIndex=0; iIndex<iCounter; iIndex++)
														{
															PrintChar (tableDiscovery[indexDiscoveryEntry].descriptor.AppInClusterList[iIndex]);
															ConsolePutROMString( (ROM char *)" " );
														}

														ConsolePutROMString( (ROM char *)"| OutClusters: " );
														iCounter=tableDiscovery[indexDiscoveryEntry].descriptor.AppOutClusterCount;
														for (iIndex=0; iIndex<iCounter; iIndex++)
														{
															PrintChar (tableDiscovery[indexDiscoveryEntry].descriptor.AppOutClusterList[iIndex]);
															ConsolePutROMString( (ROM char *)" " );
														}
														ConsolePutROMString( (ROM char *)"\r\n" );	*/												
													}
													break;

												case ZDO_DEVICE_NOT_FOUND:
													ConsolePutROMString( (ROM char *)"Device not found>\r\n" );
													break;

												default:
													ConsolePutROMString( (ROM char *)"Unknown>\r\n" );
													break;
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

                        case EP_TEMPERATURE:
                            if ((frameHeader & APL_FRAME_TYPE_MASK) == APL_FRAME_TYPE_KVP)
                            {
                                frameHeader &= APL_FRAME_COUNT_MASK;
                                for (transaction=0; transaction<frameHeader; transaction++)
                                {
                                    sequenceNumber          = APLGet();
                                    command                 = APLGet();
                                    attributeId.byte.LSB    = APLGet();
                                    attributeId.byte.MSB    = APLGet();
                                    errorCode               = APLGet();

                                    if (errorCode != KVP_SUCCESS)
                                    {
                                        ConsolePutROMString( (ROM char *)"Received error message.\r\n" );
                                    }
                                    else
                                    {
                                        //dataType = (command & APL_FRAME_DATA_TYPE_MASK) >> 4;
                                        command &= APL_FRAME_COMMAND_MASK;

                                        if ((params.APSDE_DATA_indication.ClusterId == Temperature_CLUSTER) &&
                                            (attributeId.Val == Temperature_DegCStr))
                                        {
                                            if (command == APL_FRAME_COMMAND_GET_RES)
                                            {
                                                // Data type for this attibute must be APL_FRAME_DATA_TYPE_ZSTRING
                                                ConsolePutROMString( (ROM char *)"Received " );
                                                data = APLGet();    // Length of the string
                                                while (data--)
                                                {
                                                    ConsolePut( APLGet() );
                                                }
                                                ConsolePutROMString( (ROM char *)"\r\n" );
                                            }
                                        }
                                    }
                                    // TODO read to the end of the transaction.
                                } // each transaction
                            } // frame type
                            break;
                        case EP_LIGHT:
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

                                    if ((params.APSDE_DATA_indication.ClusterId == OnOffSRC_CLUSTER) &&
                                        (attributeId.Val == OnOffSRC_OnOff))
                                    {
                                        if ((command == APL_FRAME_COMMAND_SET) ||
                                            (command == APL_FRAME_COMMAND_SETACK))
                                        {
											// dds: KVP command frames: ZB Spec pg 84 (set response command frame)
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
                                                    ConsolePutROMString( (ROM char *)"Turning light off.\r\n" );
                                                    LED_LIGHT = 0;
                                                    TxBuffer[TxData++] = SUCCESS;
                                                    break;
                                                case LIGHT_ON:
                                                    ConsolePutROMString( (ROM char *)"Turning light on.\r\n" );
                                                    LED_LIGHT = 1;
                                                    TxBuffer[TxData++] = SUCCESS;
                                                    break;
                                                case LIGHT_TOGGLE:
                                                    ConsolePutROMString( (ROM char *)"Toggling light.\r\n" );
                                                    LED_LIGHT ^= 1;
                                                    TxBuffer[TxData++] = SUCCESS;
                                                    break;
                                                default:
                                                    PrintChar( data );
                                                    ConsolePutROMString( (ROM char *)" Invalid light message.\r\n" );
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
                                            params.APSDE_DATA_request.SrcEndpoint = EP_LIGHT;
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

            case NO_PRIMITIVE:
                if (!ZigBeeStatus.flags.bits.bNetworkFormed)
                {
                    if (!ZigBeeStatus.flags.bits.bTryingToFormNetwork)
                    {
                        ConsolePutROMString( (ROM char *)"Trying to start network...\r\n" );
                        params.NLME_NETWORK_FORMATION_request.ScanDuration          = 8;
						params.NLME_NETWORK_FORMATION_request.ScanChannels.Val      = g_dwLastChannelMask? g_dwLastChannelMask: ALLOWED_CHANNELS; // dds: If I had selected another channel, then use it
                        params.NLME_NETWORK_FORMATION_request.PANId.Val             = 0xFFFF;
                        params.NLME_NETWORK_FORMATION_request.BeaconOrder           = MAC_PIB_macBeaconOrder;
                        params.NLME_NETWORK_FORMATION_request.SuperframeOrder       = MAC_PIB_macSuperframeOrder;
                        params.NLME_NETWORK_FORMATION_request.BatteryLifeExtension  = MAC_PIB_macBattLifeExt;
                        currentPrimitive = NLME_NETWORK_FORMATION_request;
                    }
                }
                else
                {
                    if (ZigBeeReady())
                    {

                        // ************************************************************************
                        // Place all processes that can send messages here.  Be sure to call
                        // ZigBeeBlockTx() when currentPrimitive is set to APSDE_DATA_request.
                        // ************************************************************************

                        if ( myStatusFlags.bits.bTemperatureSwitchToggled )
                        {
                            // Send a temperature read request to the other node
                            myStatusFlags.bits.bTemperatureSwitchToggled = FALSE;
                            ZigBeeBlockTx();

                            TxBuffer[TxData++] = APL_FRAME_TYPE_KVP | 1;    // KVP, 1 transaction
                            TxBuffer[TxData++] = APLGetTransId();
                            TxBuffer[TxData++] = APL_FRAME_COMMAND_GETACK | (Temperature_DegCStr_DATATYPE << 4);
                            TxBuffer[TxData++] = Temperature_DegCStr & 0xFF;         // Attribute ID LSB
                            TxBuffer[TxData++] = (Temperature_DegCStr >> 8) & 0xFF;  // Attribute ID MSB

                            // We are sending direct
                            params.APSDE_DATA_request.DstAddrMode = APS_ADDRESS_16_BIT;
                            params.APSDE_DATA_request.DstEndpoint = EP_TEMPERATURE_RFD;
                            params.APSDE_DATA_request.DstAddress.ShortAddr = destinationAddress;

                            //params.APSDE_DATA_request.asduLength; TxData
                            params.APSDE_DATA_request.ProfileId.Val = MY_PROFILE_ID;
                            params.APSDE_DATA_request.RadiusCounter = DEFAULT_RADIUS;
                            params.APSDE_DATA_request.DiscoverRoute = ROUTE_DISCOVERY_ENABLE;
                            params.APSDE_DATA_request.TxOptions.Val = 0x04; // dds: acknowledged transmission
                            params.APSDE_DATA_request.TxOptions.Val = 0;
                            params.APSDE_DATA_request.SrcEndpoint = EP_TEMPERATURE;
                            params.APSDE_DATA_request.ClusterId = Temperature_CLUSTER;

                            ConsolePutROMString( (ROM char *)" Requesting temperature...\r\n" );
                            
                            currentPrimitive = APSDE_DATA_request;
                        }
                       
                        if ( myStatusFlags.bits.bLightSwitchToggled )
                        {
                            // Send a light toggle message to the other node.
                            myStatusFlags.bits.bLightSwitchToggled = FALSE;
                            ZigBeeBlockTx();

						// dds: APS Payload: Zigbee spec pg 54
						// dds: KVP command frames: pg 83 (SET/SET with ack)
							TxBuffer[TxData++] = APL_FRAME_TYPE_KVP | 1;    // KVP, 1 transaction
                            TxBuffer[TxData++] = APLGetTransId();
                            TxBuffer[TxData++] = APL_FRAME_COMMAND_SET | (APL_FRAME_DATA_TYPE_UINT8 << 4);
                            TxBuffer[TxData++] = OnOffSRC_OnOff & 0xFF;         // Attribute ID LSB
                            TxBuffer[TxData++] = (OnOffSRC_OnOff >> 8) & 0xFF;  // Attribute ID MSB
                            TxBuffer[TxData++] = LIGHT_TOGGLE;

						// dds: Interface parameters (APSDE-SAP)
                            // We are sending direct
                            params.APSDE_DATA_request.DstAddrMode = APS_ADDRESS_16_BIT;
                            params.APSDE_DATA_request.DstEndpoint = EP_LIGHT;
                            params.APSDE_DATA_request.DstAddress.ShortAddr = destinationAddress;


                            //params.APSDE_DATA_request.asduLength; TxData
                            params.APSDE_DATA_request.ProfileId.Val = MY_PROFILE_ID;
                            params.APSDE_DATA_request.RadiusCounter = DEFAULT_RADIUS;
                            params.APSDE_DATA_request.DiscoverRoute = ROUTE_DISCOVERY_ENABLE;
                            params.APSDE_DATA_request.TxOptions.Val = 0x04; // dds: acknowledged transmission
                            params.APSDE_DATA_request.SrcEndpoint = EP_LIGHT;
                            params.APSDE_DATA_request.ClusterId = OnOffSRC_CLUSTER;

                            ConsolePutROMString( (ROM char *)" Trying to send light switch message.\r\n" );
                            
                            currentPrimitive = APSDE_DATA_request;
                        }
                        else // dds: nao pode tirar esse else
                        if (myStatusFlags.bits.bBroadcastSwitchToggled) // not used
                        {
                            myStatusFlags.bits.bBroadcastSwitchToggled = FALSE;

                            // Send NWK_ADDR_req message
                            ZigBeeBlockTx();

                            TxBuffer[TxData++] = APL_FRAME_TYPE_MSG | 1;    // KVP, 1 transaction
                            TxBuffer[TxData++] = APLGetTransId();
                            TxBuffer[TxData++] = 10; // Transaction Length

                            // IEEEAddr
                            TxBuffer[TxData++] = 0x65; // dds: todo - check this
                            TxBuffer[TxData++] = 0x00;
                            TxBuffer[TxData++] = 0x00;
                            TxBuffer[TxData++] = 0x00;
                            TxBuffer[TxData++] = 0x00;
                            TxBuffer[TxData++] = 0xa3;
                            TxBuffer[TxData++] = 0x04;
                            TxBuffer[TxData++] = 0x00;

                            // RequestType
                            TxBuffer[TxData++] = 0x00;

                            // StartIndex
                            TxBuffer[TxData++] = 0x00;

                            params.APSDE_DATA_request.DstAddrMode = APS_ADDRESS_16_BIT;
                            params.APSDE_DATA_request.DstEndpoint = EP_ZDO;
                            params.APSDE_DATA_request.DstAddress.ShortAddr.Val = 0xFFFF;

                            //params.APSDE_DATA_request.asduLength; TxData
                            params.APSDE_DATA_request.ProfileId.Val = ZDO_PROFILE_ID;
                            params.APSDE_DATA_request.RadiusCounter = DEFAULT_RADIUS;
                            params.APSDE_DATA_request.DiscoverRoute = ROUTE_DISCOVERY_ENABLE;
                            params.APSDE_DATA_request.TxOptions.Val = 0x04; // dds: acknowledged transmission
                            params.APSDE_DATA_request.SrcEndpoint = EP_ZDO;
                            params.APSDE_DATA_request.ClusterId = NWK_ADDR_req;

                            ConsolePutROMString( (ROM char *)" Trying to send NWK_ADDR_req.\r\n" );
                            
                            currentPrimitive = APSDE_DATA_request;
                        }
						
						// dds (ConsoleInterrupt): Console commands
						if (myStatusFlags.bits.bConsoleKeyReceived)
						{
							myStatusFlags.bits.bConsoleKeyReceived = FALSE;
							if( g_byteLastConsoleKey >= 0x30 && g_byteLastConsoleKey <=0x39 ) // a number
							{
								int iNodeIndex = g_byteLastConsoleKey-0x31;

								if( iNodeIndex <= iDiscoveryEntryCount )
									destinationAddress = tableDiscovery[iNodeIndex].addr;
								else
									destinationAddress.Val = 0x796F; // Default to first RFD
									
								ConsolePutROMString( (ROM char *)" Set next destinationAddress to <" );
								PrintChar( destinationAddress.byte.MSB );
								PrintChar( destinationAddress.byte.LSB );
								ConsolePutROMString( (ROM char *)">\r\n" );
							}
							else // not a number
							{
								switch (g_byteLastConsoleKey) // commands based on the last destination address
								{
									case 'l': // light toggle
										myStatusFlags.bits.bLightSwitchToggled = TRUE;
										break;
									case 'd': // discovery
										// todo: Send service discovery for each device/Active EP and store in a list

										ZigBeeBlockTx(); // dds: Always block TX when setting currentPrimitive to APSDE_DATA_request
	
									// AF Raw Data
										// See ZbSpec pg 77 (AF frame formats, Left most is LSB, TX first)
										TxBuffer[TxData++] = APL_FRAME_TYPE_MSG | 1;    // MSG, 1 transaction
										TxBuffer[TxData++] = APLGetTransId();
										TxBuffer[TxData++] = 3; // transaction length
	
										// See 1.4.3.1.5.1 (pg 94) -> Simple_Desc_req primitive format
										TxBuffer[TxData++] = destinationAddress.byte.LSB; // LSB
										TxBuffer[TxData++] = destinationAddress.byte.MSB; // MSB
										TxBuffer[TxData++] = EP_LIGHT; // endpoint todo: test broadcast EP
	
									// APS interface (don't worry about its raw data) -> ZbSpec pg 38
										// We are sending direct
										params.APSDE_DATA_request.DstAddrMode = APS_ADDRESS_16_BIT;
										params.APSDE_DATA_request.DstAddress.ShortAddr.Val = destinationAddress.Val;
										params.APSDE_DATA_request.DstEndpoint = EP_ZDO;
										params.APSDE_DATA_request.ProfileId.Val = MY_PROFILE_ID;
										params.APSDE_DATA_request.ClusterId = SIMPLE_DESC_req;
										params.APSDE_DATA_request.SrcEndpoint = EP_ZDO;
										//params.APSDE_DATA_request.asduLength;
										// Asdu -> TxData
										params.APSDE_DATA_request.TxOptions.Val = 0x00; // dds: acknowledged transmission does not work for this message
										params.APSDE_DATA_request.DiscoverRoute = ROUTE_DISCOVERY_ENABLE;
										params.APSDE_DATA_request.RadiusCounter = DEFAULT_RADIUS;
	
										ConsolePutROMString( (ROM char *)" Requesting RFD <" );
										PrintChar( destinationAddress.byte.MSB );
										PrintChar( destinationAddress.byte.LSB );
										ConsolePutROMString( (ROM char *)"> simple descriptor...\r\n" );
	
										currentPrimitive = APSDE_DATA_request;
	
										break;

									case 'e': // end-points (active EPs)

										ZigBeeBlockTx(); // dds: Always block TX when setting currentPrimitive to APSDE_DATA_request

										// AF Raw Data
										// See ZbSpec pg 77 (AF frame formats, Left most is LSB, TX first)
										TxBuffer[TxData++] = APL_FRAME_TYPE_MSG | 1;    // MSG, 1 transaction
										TxBuffer[TxData++] = APLGetTransId();
										TxBuffer[TxData++] = 2; // transaction length

										// See 1.4.3.1.5.1 (pg 95) -> Active_EP_req primitive format
										TxBuffer[TxData++] = destinationAddress.byte.LSB; // LSB
										TxBuffer[TxData++] = destinationAddress.byte.MSB; // MSB

										// APS interface (don't worry about its raw data) -> ZbSpec pg 38
										// We are sending direct
										params.APSDE_DATA_request.DstAddrMode = APS_ADDRESS_16_BIT;
										params.APSDE_DATA_request.DstAddress.ShortAddr.Val = destinationAddress.Val;
										params.APSDE_DATA_request.DstEndpoint = EP_ZDO;
										params.APSDE_DATA_request.ProfileId.Val = MY_PROFILE_ID;
										params.APSDE_DATA_request.ClusterId = ACTIVE_EP_req;
										params.APSDE_DATA_request.SrcEndpoint = EP_ZDO;
										//params.APSDE_DATA_request.asduLength;
										// Asdu -> TxData
										params.APSDE_DATA_request.TxOptions.Val = 0x00; // dds: acknowledged transmission does not work for this message
										params.APSDE_DATA_request.DiscoverRoute = ROUTE_DISCOVERY_ENABLE;
										params.APSDE_DATA_request.RadiusCounter = DEFAULT_RADIUS;

										ConsolePutROMString( (ROM char *)" Requesting RFD <" );
										PrintChar( destinationAddress.byte.MSB );
										PrintChar( destinationAddress.byte.LSB );
										ConsolePutROMString( (ROM char *)"> Active EP list...\r\n" );

										currentPrimitive = APSDE_DATA_request;

										break;
									
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
													ConsolePut(szChannel[indexBytesChannel++]);
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
							}
						}
						
                        // Re-enable interrupts.
                        RBIE = 1;
						RCIE = 1; // dds (ConsoleInterrupt)
                    }
                }
                break;

            default:
                PrintChar( currentPrimitive );
                ConsolePutROMString( (ROM char *)" Unhandled primitive.\r\n" );
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
HardwareInit

All port directioning and SPI must be initialized before calling ZigBeeInit().

For demonstration purposes, required signals are configured individually.
*******************************************************************************/
void HardwareInit(void)
{

    //-------------------------------------------------------------------------
    // This section is required to initialize the PICDEM Z for the CC2420
    // and the ZigBee Stack.
    //-------------------------------------------------------------------------

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

    // Is this an interrupt-on-change interrupt?
    if ( RBIF == 1 )
    {
        // Record which button was pressed so the main() loop can
        // handle it
        if (TEMPERATURE_SWITCH == 0)
            myStatusFlags.bits.bTemperatureSwitchToggled = TRUE;

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
		myStatusFlags.bits.bConsoleKeyReceived = 1;
		RCIE = 0; // disable further RCIF until we process it
		RCIF = 0;
	}
}
