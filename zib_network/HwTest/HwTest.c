//******************************************************************************
// Header Files
//******************************************************************************

// Include the main ZigBee header file.
#include "zAPL.h"

// public libs
#include <delays.h>

// If you are going to send data to a terminal, include this file.
#include "console.h"
#include "cc2420_test.h"


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

#define BIND_SWITCH                 RB5
#define LIGHT_SWITCH                RB4

#define BIND_INDICATION             LATA0
#define MESSAGE_INDICATION          LATA1

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
void DisplayMainMenu(void);
void BlinkLD1(int iTimes);
void BlinkLD2(int iTimes);

#define sleep(s)\
	{\
		int iDelay = s*CLOCK_FREQ/200/10000;\
		while (iDelay--)\
			Delay10KTCYx(10);\
	}

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
void DisplayMainMenu(void)
{
    TRACE("\r\n\r\n\r\n*************************************\r\n");
    TRACE("Microchip ZigBee(TM) Stack - v1.0-3.6\r\n");
    TRACE("DTP Hardware Tester - by Lele Deschamps\r\n");
    TRACE("PIC18_LF_4620 + CC_2420\r\n\r\n");	
	TRACE("1 - Blink LD1, then LD2\r\n");
	TRACE("2 - Test CC24020\r\n");
	TRACE("3 - Start Zigbee Coordinator\r\n");
	TRACE("PRESS S1 - Blink LD1\r\n");
	TRACE("PRESS S2 - Blink LD2\r\n");
	TRACE("Please select one of the hardware test steps: ");
}

void BlinkLD1(int iTimes)
{
	int iBlinkCount;

	iBlinkCount=iTimes;
	while (iBlinkCount--)
	{
		BIND_INDICATION = 1;
		sleep(0.25);
		BIND_INDICATION = 0;
		sleep(0.25);
	}
}

void BlinkLD2(int iTimes)
{
	while (iTimes--)
	{
		MESSAGE_INDICATION = 1;
		sleep(0.25);
		MESSAGE_INDICATION = 0;
		sleep(0.25);
	}
}

void InitInternalVariables(void)
{
	// uChip
	myStatusFlags.Val = STATUS_FLAGS_INIT;
	destinationAddress.Val = 0x796F;    // Default to first RFD
	BIND_INDICATION = !myStatusFlags.bits.bIsBound;
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
    // ENABLE_WDT();

    ConsoleInit(); // initialize the UART
    HardwareInit(); // Initialize the hardware - must be done before initializing ZigBee.
    
	// Test the hardware
   	GIEL = 1; // dds (ConsoleInterrupt): Peripheral Interrupt Enable bit
	RCIE = 1; // dds (ConsoleInterrupt): Enable RS-232 receive Interrupt
	GIEH = 1;

	DisplayMainMenu();
	while(1)
	{
		BOOL bGetOut=FALSE;
		if ( myStatusFlags.bits.bLightSwitchToggled )
		{
			myStatusFlags.bits.bLightSwitchToggled = FALSE;
			TRACE("\r\n\r\nBlinking LD1...\r\n");
			BlinkLD1(10);
			DisplayMainMenu();
		}
		
		if (myStatusFlags.bits.bBroadcastSwitchToggled)
		{
		   myStatusFlags.bits.bBroadcastSwitchToggled = FALSE;
		   TRACE("\r\n\r\nBlinking LD2...\r\n");
		   BlinkLD2(10);
		   DisplayMainMenu();
		}
                     
		// dds (ConsoleInterrupt): Console commands
		if (myStatusFlags.bits.bConsoleKeyReceived)
		{
			TRACE("\r\n\r\n");
			myStatusFlags.bits.bConsoleKeyReceived = FALSE;
			switch (g_byteLastConsoleKey) // commands based on the last destination address
			{
				case '1': // blink ld1, ld2
				{
					TRACE("Blinking LD1...\r\n");
					BlinkLD1(10);
					TRACE("Blinking LD2...\r\n");
					BlinkLD2(10);
					
					DisplayMainMenu();
					break;
				}
				case '2':
				{
					PHYInitLog();
					TRACE("Press any key to continue...");
					while(!ConsoleIsGetReady());
					
					DisplayMainMenu();
					break;
				}
				case '3':
				{
					bGetOut = TRUE;
					break;
				}
			}
		}
	
	    // Re-enable interrupts.
	    RBIE = 1;
		RCIE = 1;
		
		if(bGetOut)
			break;
	}

	// from now on, normal coordinator operation
	CLRWDT();
    ENABLE_WDT();
    
    // disable interrupts to initialize the stack and variables
    RBIE = 0;
    IPEN = 0;
    GIEH = 0;
	GIEL = 0;
	RCIE = 0;
    
    currentPrimitive = NO_PRIMITIVE;
    ZigBeeInit(); // initialize the ZigBee Stack.

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

                                        #ifdef USE_BINDINGS
                                        case END_DEVICE_BIND_rsp:
                                            switch( APLGet() )
                                            {
                                                case SUCCESS:
                                                    ConsolePutROMString( (ROM char *)"End device bind/unbind successful!\r\n" );
                                                    myStatusFlags.bits.bIsBound ^= 1;
                                                    BIND_INDICATION = !myStatusFlags.bits.bIsBound;
                                                    break;
                                                case ZDO_NOT_SUPPORTED:
                                                    ConsolePutROMString( (ROM char *)"End device bind/unbind not supported.\r\n" );
                                                    break;
                                                case END_DEVICE_BIND_TIMEOUT:
                                                    ConsolePutROMString( (ROM char *)"End device bind/unbind time out.\r\n" );
                                                    break;
                                                case END_DEVICE_BIND_NO_MATCH:
                                                    ConsolePutROMString( (ROM char *)"End device bind/unbind failed - no match.\r\n" );
                                                    break;
                                                default:
                                                    ConsolePutROMString( (ROM char *)"End device bind/unbind invalid response.\r\n" );
                                                    break;
                                            }
                                            myStatusFlags.bits.bTryingToBind = 0;
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
                                                    MESSAGE_INDICATION = 0;
                                                    TxBuffer[TxData++] = SUCCESS;
                                                    break;
                                                case LIGHT_ON:
                                                    ConsolePutROMString( (ROM char *)"Turning light on.\r\n" );
                                                    MESSAGE_INDICATION = 1;
                                                    TxBuffer[TxData++] = SUCCESS;
                                                    break;
                                                case LIGHT_TOGGLE:
                                                    ConsolePutROMString( (ROM char *)"Toggling light.\r\n" );
                                                    MESSAGE_INDICATION ^= 1;
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

                        #ifdef I_AM_SWITCH
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
                            #ifdef USE_BINDINGS
                                // We are sending indirect
                                params.APSDE_DATA_request.DstAddrMode = APS_ADDRESS_NOT_PRESENT;
                            #else
                                // We are sending direct
                                params.APSDE_DATA_request.DstAddrMode = APS_ADDRESS_16_BIT;
                                params.APSDE_DATA_request.DstEndpoint = EP_LIGHT;
                                params.APSDE_DATA_request.DstAddress.ShortAddr = destinationAddress;
                            #endif

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
                        else
                        #endif
                        #ifdef USE_BINDINGS
                        if (myStatusFlags.bits.bBindSwitchToggled)
                        {
                            myStatusFlags.bits.bBindSwitchToggled = FALSE;
                            if (myStatusFlags.bits.bTryingToBind)
                            {
                                ConsolePutROMString( (ROM char *)" End Device Binding already in progress.\r\n" );
                            }
                            else
                            {
                                myStatusFlags.bits.bTryingToBind = 1;

                                // Send down an end device bind primitive.
                                ConsolePutROMString( (ROM char *)" Trying to perform end device binding.\r\n" );
                                currentPrimitive = ZDO_END_DEVICE_BIND_req;

                                params.ZDO_END_DEVICE_BIND_req.ProfileID.Val = MY_PROFILE_ID;
                                #ifdef I_AM_LIGHT
                                    params.ZDO_END_DEVICE_BIND_req.NumInClusters = 1; // dds: number of input clusters is hardcoded?
                                     // dds: initialize in memory an array of clusters Id and send the addras as an argument
                                    if ((params.ZDO_END_DEVICE_BIND_req.InClusterList=SRAMalloc(1)) != NULL)
                                    {
                                        *params.ZDO_END_DEVICE_BIND_req.InClusterList = OnOffSRC_CLUSTER;
                                    }
                                    else
                                    {
                                        myStatusFlags.bits.bTryingToBind = 0;
                                        ConsolePutROMString( (ROM char *)" Could not send down ZDO_END_DEVICE_BIND_req.\r\n" );
                                        currentPrimitive = NO_PRIMITIVE;
                                    }
                                #else
                                    params.ZDO_END_DEVICE_BIND_req.NumInClusters = 0; // dds: I don't have InClusters if not a light
                                    params.ZDO_END_DEVICE_BIND_req.InClusterList = NULL;
                                #endif
                                #ifdef I_AM_SWITCH
                                    params.ZDO_END_DEVICE_BIND_req.NumOutClusters = 1;
                                    if ((params.ZDO_END_DEVICE_BIND_req.OutClusterList=SRAMalloc(1)) != NULL)
                                    {
                                        *params.ZDO_END_DEVICE_BIND_req.OutClusterList = OnOffSRC_CLUSTER;
                                    }
                                    else
                                    {
                                        myStatusFlags.bits.bTryingToBind = 0;
                                        ConsolePutROMString( (ROM char *)" Could not send down ZDO_END_DEVICE_BIND_req.\r\n" );
                                        currentPrimitive = NO_PRIMITIVE;
                                    }
                                #else
                                    params.ZDO_END_DEVICE_BIND_req.NumOutClusters = 0;
                                    params.ZDO_END_DEVICE_BIND_req.OutClusterList = NULL;
                                #endif
                                params.ZDO_END_DEVICE_BIND_req.LocalCoordinator = macPIB.macShortAddress;
                                params.ZDO_END_DEVICE_BIND_req.endpoint = EP_LIGHT;
                            }
                        }
                        #else
                        if (myStatusFlags.bits.bBroadcastSwitchToggled)
                        {
                            myStatusFlags.bits.bBroadcastSwitchToggled = FALSE;

                            // Send NWK_ADDR_req message
                            ZigBeeBlockTx();

                            TxBuffer[TxData++] = APL_FRAME_TYPE_MSG | 1;    // KVP, 1 transaction
                            TxBuffer[TxData++] = APLGetTransId();
                            TxBuffer[TxData++] = 10; // Transaction Length

                            // IEEEAddr
                            TxBuffer[TxData++] = 0x65;
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
                        #endif
						
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
													indexBytesChannel++;
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
	if (RBIF == 1) // is this an interrupt-on-change interrupt?
	{
		if (BIND_SWITCH == 0)
			myStatusFlags.bits.bBindSwitchToggled = TRUE;
		
		if (LIGHT_SWITCH == 0)
			myStatusFlags.bits.bLightSwitchToggled = TRUE;
		
		RBIE = 0; // disable further RBIF until we process it
		LATB = PORTB; // clear mis-match condition and reset the interrupt flag
		RBIF = 0; // reset the interrupt
	}
	
	if (ConsoleIsGetReady()) // Tests the console's RCIF
	{
		g_byteLastConsoleKey = ConsoleGet();
		myStatusFlags.bits.bConsoleKeyReceived = TRUE;
		RCIE = 0; // disable further RCIF until we process it
		RCIF = 0; // reset the interrupt
	}
}
