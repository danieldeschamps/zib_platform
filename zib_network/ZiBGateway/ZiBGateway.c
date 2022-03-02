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

// maw - Included for GATEWAY purpose 
#include "..\..\common\ZiBManagerGatewayProtocol.h"
#include <delays.h>

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
        BYTE    bTemperatureSwitchToggled  : 1;
        BYTE    bLightSwitchToggled        : 1;
        BYTE    bTryingToBind              : 1;
        BYTE    bIsBound                   : 1;
        BYTE    bDestinationAddressKnown   : 1;
		BYTE	bSerialMsgReceived         : 1;
    } bits;
    BYTE Val;
} myStatusFlags;
#define STATUS_FLAGS_INIT       0x00
#define TOGGLE_BOUND_FLAG       0x08
#define bBindSwitchToggled      bBroadcastSwitchToggled

// dds:
#define MAX_DISCOVERY_ENTRY		2 // Network Nodes - Cannot raise this number or errors will occur
#define MAX_DESCRIPTOR_ENTRY	2 // EPs inside the nodes

typedef struct _EP_DISCOVERY_ENTRY
{
	SHORT_ADDR addr;
	NODE_SIMPLE_DESCRIPTOR descriptor[MAX_DESCRIPTOR_ENTRY];
	int iDescriptorEntryCount;
} EP_DISCOVERY_ENTRY;

DWORD g_dwLastChannelMask;

int iDiscoveryEntryCount;
EP_DISCOVERY_ENTRY tableDiscovery[MAX_DISCOVERY_ENTRY];

int iIndex; // index for general purposes

// RX state machine
typedef enum
{
ERxStateWaitingStartByte,
ERxStateWaitingEndOfMessage
}  ERxState;

ERxState g_smRx;
BYTE g_byteMsgBuffer[50];
int g_iMsgIndex;

// Manager communication auxiliary variables
ZiBMessage msg;     
BYTE* pWriter;
static ZiBNode nodeList[5]; //maw declared static because variable don't fit in the stack
ZiBMessage* pMsg = (ZiBMessage*)&g_byteMsgBuffer;


BOOL no_node;	//maw
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
	g_dwLastChannelMask = 0;
	g_smRx = ERxStateWaitingStartByte;
	g_iMsgIndex = 0;
	
	no_node = TRUE;
}

void put_table(void)
{	//OLHAR LINK http://forum.microchip.com/tm.aspx?m=191831&mpage=1&key=mac%2caddress&#191831

	BYTE i;
	SHORT_ADDR temp;

	i=0;  
       ConsolePutROMString((ROM char *)"\r\n            Neighbor Table\r\n");
       ConsolePutROMString((ROM char *)"--------------------------------------------\r\n");
       ConsolePutROMString((ROM char *)"|shortAddres| PanID | Numero nodo  |  Tipo  |\r\n");
   pCurrentNeighborRecord = neighborTable;
   for ( i = 0; i < MAX_NEIGHBORS; i++, pCurrentNeighborRecord++)
   {
       // Read the record into RAM.
       GetNeighborRecord(&currentNeighborRecord, (ROM void*)pCurrentNeighborRecord );

       // Make sure that this record is in use
       if ( currentNeighborRecord.deviceInfo.bits.bInUse )
       {
           ConsolePutROMString((ROM char *)"----------------------------------------------\r\n");

           ConsolePutROMString((ROM char *)"|   ");
           TRACE_CHAR(currentNeighborRecord.shortAddr.v[1]);
           TRACE_CHAR(currentNeighborRecord.shortAddr.v[0]);
           ConsolePutROMString((ROM char *)"    |  ");
           TRACE_CHAR(currentNeighborRecord.panID.v[1]);
           TRACE_CHAR(currentNeighborRecord.panID.v[0]);
           ConsolePutROMString((ROM char *)" |     ");
           TRACE_CHAR((BYTE)i+1);
           ConsolePutROMString((ROM char *)"       |  ");
           switch(currentNeighborRecord.deviceInfo.bits.deviceType)
           {
               case 0x00:
                   ConsolePutROMString((ROM char *)"Coor");
                   break;
               case 0x01:
                   ConsolePutROMString((ROM char *)"Rout");
                   break;
               case 0x02:
                   ConsolePutROMString((ROM char *)"Nodo");
                   break;
               default:
                   ConsolePutROMString((ROM char *)"    ");
                   break;
                   
           }
           ConsolePutROMString((ROM char *)"  |\n\r");
           
       }
   }
   ConsolePutROMString((ROM char *)"-----------------------------------------------\r\n");

}

void main(void)
{
    CLRWDT();
    ENABLE_WDT();

    currentPrimitive = NO_PRIMITIVE;

    // If you are going to send data to a terminal, initialize the UART.
    ConsoleInit();

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
	
    LED_UNNUSED = 0;
    LED_LIGHT = 1;
    
    while (1)
    {
        CLRWDT();
        ZigBeeTasks( &currentPrimitive );

        switch (currentPrimitive)
        {
            case NLME_NETWORK_FORMATION_confirm:
                if (!params.NLME_NETWORK_FORMATION_confirm.Status)
                {
					TRACE("PAN " );
                    TRACE_CHAR( macPIB.macPANId.byte.MSB );
                    TRACE_CHAR( macPIB.macPANId.byte.LSB );
                    TRACE(" started successfully.\r\n" );
                    params.NLME_PERMIT_JOINING_request.PermitDuration = 0xFF;   // No Timeout
                    currentPrimitive = NLME_PERMIT_JOINING_request;
                }
                else
                {
                    TRACE_CHAR( params.NLME_NETWORK_FORMATION_confirm.Status );
                    TRACE(" Error forming network.  Trying again...\r\n" );
                    currentPrimitive = NO_PRIMITIVE;
                }
                break;

            case NLME_PERMIT_JOINING_confirm:
                if (!params.NLME_PERMIT_JOINING_confirm.Status)
                {
                    TRACE("Joining permitted.\r\n" );
                    currentPrimitive = NO_PRIMITIVE;
                }
                else
                {
                    TRACE_CHAR( params.NLME_PERMIT_JOINING_confirm.Status );
                    TRACE(" Join permission unsuccessful. We cannot allow joins.\r\n" );
                    currentPrimitive = NO_PRIMITIVE;
                }
                break;

            case NLME_JOIN_indication: // dds: devices which join directly to the coordinator are signaled here. (not the router joiners)
                TRACE("Node " );
                TRACE_CHAR( params.NLME_JOIN_indication.ShortAddress.byte.MSB );
                TRACE_CHAR( params.NLME_JOIN_indication.ShortAddress.byte.LSB );
                TRACE(" just joined.\r\n" );
                for(iIndex=0;iIndex<4;iIndex++){
                	LED_UNNUSED ^= 1;
                	Delay10KTCYx(25); 
                }
                currentPrimitive = NO_PRIMITIVE;
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

            case APSDE_DATA_confirm:
                if (params.APSDE_DATA_confirm.Status)
                {	
                    TRACE("Error Teste" );
                    TRACE_CHAR( params.APSDE_DATA_confirm.Status );
                    TRACE(" sending message.\r\n" );
                }
                else
                {
                    TRACE(" Message sent successfully.\r\n" );
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
                            TRACE("  Receiving ZDO cluster " );
                            TRACE_CHAR( params.APSDE_DATA_indication.ClusterId );
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

                                        #ifndef USE_BINDINGS
                                        case NWK_ADDR_rsp:
                                            if (APLGet() == SUCCESS)
                                            {
                                                TRACE("Receiving NWK_ADDR_rsp.\r\n" );

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
											TRACE("SIMPLE_DESC_rsp: Status=<" );
											switch( APLGet() ) // Status byte
											{
												case SUCCESS:
													{
														SHORT_ADDR addrRsp;
														int iCounter;

														TRACE("Success>\r\n" );
														msg.type.cluster_list_conf.status = ZIB_ERROR_SUCCESS;
														
														// NWKAddrOfInterest
														addrRsp.byte.LSB = APLGet();
														addrRsp.byte.MSB = APLGet();
														transByte+=2;
																								
														msg.type.cluster_list_conf.nwk_addr = addrRsp.Val;
														
														
														// Descriptor Length
														APLGet();
														transByte++;
														msg.type.cluster_list_conf.ep = APLGet();
														transByte++;
														
														for (iIndex=0; iIndex<5; iIndex++, transByte++)
														{
															APLGet(); // From 'Application profile identifier' to 'Application flags'
														}
														
														iCounter = APLGet();
														transByte++;
														
														for(;iCounter>0;iCounter--,transByte++)
															APLGet();
														
														msg.type.cluster_list_conf.count = APLGet(); // 'Application output cluster count'
														
														// ConsolePut(msg.type.cluster_list_conf.count); //maw just for debug
														
														msg.length = ZIB_MSG_HEADER_SIZE
														    + sizeof(msg.type.cluster_list_conf) -1
													    	+ (msg.type.cluster_list_conf.count*sizeof(BYTE));
													    
													    msg.code = ZIB_MSG_CODE_CLUSTER_LIST_CONF;
														
														ConsolePut(ZIB_COMM_START_BYTE);
														
														pWriter = (BYTE*)&msg;
													    for (iIndex=0; iIndex < (ZIB_MSG_HEADER_SIZE + sizeof(msg.type.cluster_list_conf) -1); iIndex++)
													    {
														    ConsolePut(pWriter[iIndex]);
														}
														
														// transmit the cluster list														
														for (iIndex=0; iIndex<msg.type.cluster_list_conf.count; iIndex++, transByte++)
														{
															ConsolePut(APLGet());
														}
													}
													break;

												case ZDO_INVALID_EP:
													{
														TRACE("Invalid EP>\r\n" );
														msg.code = ZIB_MSG_CODE_CLUSTER_LIST_CONF;
														msg.length = ZIB_MSG_HEADER_SIZE+1;
														msg.type.cluster_list_conf.status = ZIB_ERROR_FAILURE;
														ConsolePut(ZIB_COMM_START_BYTE);
														pWriter = (BYTE*)&msg;
													    for (iIndex=0; iIndex < (ZIB_MSG_HEADER_SIZE + 1); iIndex++)
													    {
														    ConsolePut(pWriter[iIndex]);
														}
													}
													break;

												case ZDO_NOT_ACTIVE:
													{
														TRACE("Not Active>\r\n" );
														msg.code = ZIB_MSG_CODE_CLUSTER_LIST_CONF;
														msg.length = ZIB_MSG_HEADER_SIZE+1;
														msg.type.cluster_list_conf.status = ZIB_ERROR_FAILURE;
														ConsolePut(ZIB_COMM_START_BYTE);
														pWriter = (BYTE*)&msg;
													    for (iIndex=0; iIndex < (ZIB_MSG_HEADER_SIZE + 1); iIndex++)
													    {
														    ConsolePut(pWriter[iIndex]);
														}
													}
													break;

												case ZDO_DEVICE_NOT_FOUND:
													{
														TRACE("Device not found>\r\n" );
														msg.code = ZIB_MSG_CODE_CLUSTER_LIST_CONF;
														msg.length = ZIB_MSG_HEADER_SIZE+1;
														msg.type.cluster_list_conf.status = ZIB_ERROR_FAILURE;
														ConsolePut(ZIB_COMM_START_BYTE);
														pWriter = (BYTE*)&msg;
													    for (iIndex=0; iIndex < (ZIB_MSG_HEADER_SIZE + 1); iIndex++)
													    {
														    ConsolePut(pWriter[iIndex]);
														}
													}
													break;

												default:
													TRACE("Unknown>\r\n" );
													break;
											}
											break;

										case ACTIVE_EP_rsp: // dds: In cluster pg 116 Table 68
											TRACE("ACTIVE_EP_rsp: Status=<" );
											switch( APLGet() ) // Status byte
											{
												case SUCCESS:
													{
														SHORT_ADDR addrRsp;

														TRACE("Success>\r\n" );

														// NWKAddrOfInterest
														addrRsp.byte.LSB = APLGet();
														addrRsp.byte.MSB = APLGet();
														transByte+=2;

														
														msg.type.ep_list_conf.count = APLGet();
														transByte++;
													    msg.length = ZIB_MSG_HEADER_SIZE 
														    + sizeof(msg.type.ep_list_conf) -1
													    	+ (msg.type.ep_list_conf.count*sizeof(BYTE));
													    msg.code = ZIB_MSG_CODE_EP_LIST_CONF;
													    msg.type.ep_list_conf.status = ZIB_ERROR_SUCCESS;
													    msg.type.ep_list_conf.nwk_addr = addrRsp.Val;
														
																										
														
														ConsolePut(ZIB_COMM_START_BYTE);
								    					// transmit the header and the node count
													    pWriter = (BYTE*)&msg;
													    for (iIndex=0; iIndex < (ZIB_MSG_HEADER_SIZE + sizeof(msg.type.ep_list_conf) -1); iIndex++)
													    {
														    ConsolePut(pWriter[iIndex]);
														}
														
														// transmit the ep list
														for (iIndex=0; iIndex < msg.type.ep_list_conf.count; iIndex++, transByte++)
														{
															ConsolePut(APLGet()); // dds: TODO - Store this information
														}											
														
													}
													break;

												case ZDO_DEVICE_NOT_FOUND:
													{
														TRACE("Device not found>\r\n" );												
														msg.code = ZIB_MSG_CODE_EP_LIST_CONF;
														msg.length = ZIB_MSG_HEADER_SIZE+1;
														msg.type.cluster_list_conf.status = ZIB_ERROR_FAILURE;
														ConsolePut(ZIB_COMM_START_BYTE);
														pWriter = (BYTE*)&msg;
													    for (iIndex=0; iIndex < (ZIB_MSG_HEADER_SIZE + 1); iIndex++)
													    {
														    ConsolePut(pWriter[iIndex]);
														}
													}
													break;

												default:
													TRACE("Unknown>\r\n" );
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

                        case EP_ZIB:
                            if ((frameHeader & APL_FRAME_TYPE_MASK) == APL_FRAME_TYPE_KVP)
                            {
                                frameHeader &= APL_FRAME_COUNT_MASK;
                                for (transaction=0; transaction<frameHeader; transaction++)
                                {
                                    sequenceNumber          = APLGet();
                                    command                 = APLGet(); // Pg 78 Figure 21 - Command type identifier + Attribute data type
                                    attributeId.byte.LSB    = APLGet();
                                    attributeId.byte.MSB    = APLGet();
                                    errorCode               = APLGet();

                                    if (errorCode != KVP_SUCCESS)
                                    {
                                        TRACE("Received error message.\r\n" );
                                        // todo: send error messag to zib manager
                                    }
                                    else
                                    {
                                        //dataType = (command & APL_FRAME_DATA_TYPE_MASK) >> 4;
                                        command &= APL_FRAME_COMMAND_MASK;

                                        if ((params.APSDE_DATA_indication.ClusterId == ZIB_DTP_ANALOG_CLUSTER) &&
                                            (attributeId.Val == ATTRIBUTE_ZIB_DTP_ANALOG_VOLTS)
                                            ||
                                            (params.APSDE_DATA_indication.ClusterId == ZIB_DTP_SENSORS_CLUSTER) &&
                                            (attributeId.Val == ATTRIBUTE_ZIB_DTP_SENSORS_TEMPERATURE))
                                        {
                                            if (command == APL_FRAME_COMMAND_GET_RES)
                                            {
												msg.code = ZIB_MSG_CODE_GET_ATTRIBUTE_VALUE_CONF;
												//msg.code = 0xDD; // todo: verify why it doesn't work if we put only 4 bits (i.e. 0x0D)
												msg.type.get_attribute_value_conf.status = ZIB_ERROR_SUCCESS;
												msg.type.get_attribute_value_conf.nwk_addr = params.APSDE_DATA_indication.SrcAddress.ShortAddr.Val;
												msg.type.get_attribute_value_conf.ep = params.APSDE_DATA_indication.SrcEndpoint;
												msg.type.get_attribute_value_conf.cluster = params.APSDE_DATA_indication.ClusterId;
												msg.type.get_attribute_value_conf.attribute_id = attributeId.Val;
												msg.type.get_attribute_value_conf.size = APLGet();
												msg.length = ZIB_MSG_HEADER_SIZE 
													+ sizeof(msg.type.get_attribute_value_conf) -1
													+ msg.type.get_attribute_value_conf.size;
												
												// transmit the beginning
												ConsolePut(ZIB_COMM_START_BYTE);
												pWriter = (BYTE*)&msg;
												for (iIndex=0; iIndex<(ZIB_MSG_HEADER_SIZE + sizeof(msg.type.get_attribute_value_conf) -1);iIndex++)
												{
													ConsolePut(pWriter[iIndex]);
												}
												
												// transmit the string
												for (iIndex=0; iIndex < msg.type.get_attribute_value_conf.size; iIndex++)
												{
													ConsolePut(APLGet());
												}
                                            }
                                        }
                                    }
                                    // TODO read to the end of the transaction.
                                } // each transaction
                            } // frame type
                            break;
                        /*case EP_LIGHT:
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
                                                    TRACE("Turning light off.\r\n" );
                                                    LED_LIGHT = 0;
                                                    TxBuffer[TxData++] = SUCCESS;
                                                    break;
                                                case LIGHT_ON:
                                                    TRACE("Turning light on.\r\n" );
                                                    LED_LIGHT = 1;
                                                    TxBuffer[TxData++] = SUCCESS;
                                                    break;
                                                case LIGHT_TOGGLE:
                                                    TRACE("Toggling light.\r\n" );
                                                    LED_LIGHT ^= 1;
                                                    TxBuffer[TxData++] = SUCCESS;
                                                    break;
                                                default:
                                                    TRACE_CHAR( data );
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
                            break;*/
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
                        TRACE("Trying to start network...\r\n" );
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
                            /*// Send a temperature read request to the other node
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

                            TRACE(" Requesting temperature...\r\n" );
                            
                            currentPrimitive = APSDE_DATA_request;*/
                        }
                       
                        if ( myStatusFlags.bits.bLightSwitchToggled )
                        {
                            /*// Send a light toggle message to the other node.
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

                            TRACE(" Trying to send light switch message.\r\n" );
                            
                            currentPrimitive = APSDE_DATA_request;*/
                        }
                        else // dds: nao pode tirar esse else
                        if (myStatusFlags.bits.bBroadcastSwitchToggled) // not used
                        {
                            /*myStatusFlags.bits.bBroadcastSwitchToggled = FALSE;

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

                            TRACE(" Trying to send NWK_ADDR_req.\r\n" );
                            
                            currentPrimitive = APSDE_DATA_request;*/
                        }

                        if (myStatusFlags.bits.bSerialMsgReceived == TRUE)
						{	
							myStatusFlags.bits.bSerialMsgReceived = FALSE;
							switch (pMsg->code)
							{	
								case ZIB_MSG_CODE_RESET_REQ:
								{
									TRACE("RESET_REQ:\r\n");
								    msg.length = ZIB_MSG_HEADER_SIZE 
								    	+ sizeof(msg.type.reset_conf);
								    msg.code = ZIB_MSG_CODE_RESET_CONF;
								    msg.type.reset_conf.status = ZIB_ERROR_SUCCESS;
									
								    ConsolePut(ZIB_COMM_START_BYTE);
								    pWriter = (BYTE*)&msg;
								    for (iIndex=0; iIndex<msg.length;iIndex++)
								    {
									    ConsolePut(pWriter[iIndex]);
									}
									InitInternalVariables();
									currentPrimitive = NLME_RESET_request;
								}
									break;				
								case ZIB_MSG_CODE_PAN_ID_REQ:
								{
									TRACE("PAN_ID_REQ:\r\n");
								    msg.length = ZIB_MSG_HEADER_SIZE 
								    	+ sizeof(msg.type.pan_id_conf);
								    msg.code = ZIB_MSG_CODE_PAN_ID_CONF;
								    msg.type.pan_id_conf.status = ZIB_ERROR_SUCCESS;
									msg.type.pan_id_conf.pan_id = ((WORD)macPIB.macPANId.byte.MSB<<8 | macPIB.macPANId.byte.LSB);
								    ConsolePut(ZIB_COMM_START_BYTE);
								    pWriter = (BYTE*)&msg;
								    for (iIndex=0; iIndex<msg.length;iIndex++)
								    {
									    ConsolePut(pWriter[iIndex]);
									}
								}
									break;
								case ZIB_MSG_CODE_NODE_LIST_REQ:
								{	
									TRACE("NODE LIST REQ:\r\n");
									
								   pCurrentNeighborRecord = neighborTable;
								   for ( iIndex = 0; iIndex < MAX_NEIGHBORS; iIndex++, pCurrentNeighborRecord++)
								   {
								       // Read the record into RAM.
								       GetNeighborRecord(&currentNeighborRecord, (ROM void*)pCurrentNeighborRecord );
								
								       // Make sure that this record is in use
								       if ( currentNeighborRecord.deviceInfo.bits.bInUse )
								       {
								           nodeList[iIndex].mac_addr[7] = currentNeighborRecord.longAddr.v[7];
								           nodeList[iIndex].mac_addr[6] = currentNeighborRecord.longAddr.v[6];
										   nodeList[iIndex].mac_addr[5] = currentNeighborRecord.longAddr.v[5];
										   nodeList[iIndex].mac_addr[4] = currentNeighborRecord.longAddr.v[4];
										   nodeList[iIndex].mac_addr[3] = currentNeighborRecord.longAddr.v[3];
										   nodeList[iIndex].mac_addr[2] = currentNeighborRecord.longAddr.v[2];
										   nodeList[iIndex].mac_addr[1] = currentNeighborRecord.longAddr.v[1];
										   nodeList[iIndex].mac_addr[0] = currentNeighborRecord.longAddr.v[0];
										   nodeList[iIndex].nwk_addr = currentNeighborRecord.shortAddr.Val;
										   nodeList[iIndex].profile_id = 0xAAAA;
										   nodeList[iIndex].device_id = 0x1111;
										   msg.type.node_list_conf.count = (WORD)iIndex+1;
										   no_node = FALSE;
								       }
								       	
								   }
									if(no_node)
										msg.type.node_list_conf.count = 0x0000;
									msg.type.node_list_conf.status = ZIB_ERROR_SUCCESS;
									msg.code = ZIB_MSG_CODE_NODE_LIST_CONF;
									msg.length = ZIB_MSG_HEADER_SIZE 
								    	+ sizeof(msg.type.node_list_conf) -1
								    	+ (msg.type.node_list_conf.count*sizeof(ZiBNode));
									
								    ConsolePut(ZIB_COMM_START_BYTE);
								    // transmit the header and the node count
								    pWriter = (BYTE*)&msg;
								    for (iIndex=0; iIndex < (ZIB_MSG_HEADER_SIZE + sizeof(msg.type.node_list_conf) -1); iIndex++)
								    {
									    ConsolePut(pWriter[iIndex]);
									}
									
									// transmit the node list
									pWriter = (BYTE*)nodeList;
								    for (iIndex=0; iIndex < (msg.type.node_list_conf.count*sizeof(ZiBNode)); iIndex++)
								    {
									    ConsolePut(pWriter[iIndex]);
									}
									
								}
									break;
								case ZIB_MSG_CODE_EP_LIST_REQ:
								{
									TRACE("END POINT LIST REQ:\r\n");
									
									//LINHA PARA TESTE - REMOVER
									//pMsg->type.ep_list_req.nwk_addr = 0x796F;
																
									ZigBeeBlockTx(); // dds: Always block TX when setting currentPrimitive to APSDE_DATA_request

									// AF Raw Data
									// See ZbSpec pg 77 (AF frame formats, Left most is LSB, TX first)
									TxBuffer[TxData++] = APL_FRAME_TYPE_MSG | 1;    // MSG, 1 transaction
									TxBuffer[TxData++] = APLGetTransId();
									TxBuffer[TxData++] = 2; // transaction length
	
									// See 1.4.3.1.5.1 (pg 95) -> Active_EP_req primitive format
									TxBuffer[TxData++] = (BYTE)(pMsg->type.ep_list_req.nwk_addr);  //LSB
									TxBuffer[TxData++] = pMsg->type.ep_list_req.nwk_addr>>8;	 //MSB
									
									// APS interface (don't worry about its raw data) -> ZbSpec pg 38
									// We are sending direct
									params.APSDE_DATA_request.DstAddrMode = APS_ADDRESS_16_BIT;
									
									params.APSDE_DATA_request.DstAddress.ShortAddr.Val = pMsg->type.ep_list_req.nwk_addr;
									params.APSDE_DATA_request.DstEndpoint = EP_ZDO;
									params.APSDE_DATA_request.ProfileId.Val = MY_PROFILE_ID;
									params.APSDE_DATA_request.ClusterId = ACTIVE_EP_req;
									params.APSDE_DATA_request.SrcEndpoint = EP_ZDO;
									//params.APSDE_DATA_request.asduLength;
									// Asdu -> TxData
									params.APSDE_DATA_request.TxOptions.Val = 0x00; // dds: acknowledged transmission does not work for this message
									params.APSDE_DATA_request.DiscoverRoute = ROUTE_DISCOVERY_ENABLE;
									params.APSDE_DATA_request.RadiusCounter = DEFAULT_RADIUS;
	
									currentPrimitive = APSDE_DATA_request;														
								}
									break;
								case ZIB_MSG_CODE_CLUSTER_LIST_REQ:
								{
									TRACE("CLUSTER LIST REQ:\r\n");
									//LINHA DE TESTE, REMOVER !!!! 
									//pMsg->type.cluster_list_req.ep = 0x01; //8-light 3-temp
									//LINHA DE TESTE, REMOVER !!!! 
									//pMsg->type.ep_list_req.nwk_addr = 0x796F;
									
                                    ZigBeeBlockTx(); // dds: Always block TX when setting currentPrimitive to APSDE_DATA_request
			
                                    // AF Raw Data
                                    // See ZbSpec pg 77 (AF frame formats, Left most is LSB, TX first)
                                    TxBuffer[TxData++] = APL_FRAME_TYPE_MSG | 1;    // MSG, 1 transaction
                                    TxBuffer[TxData++] = APLGetTransId();
                                    TxBuffer[TxData++] = 3; // transaction length

                                    TxBuffer[TxData++] = (BYTE)(pMsg->type.cluster_list_req.nwk_addr);  //LSB
                                    TxBuffer[TxData++] = pMsg->type.cluster_list_req.nwk_addr>>8;       //MSB
                                    TxBuffer[TxData++] = pMsg->type.cluster_list_req.ep;

                                    // APS interface (don't worry about its raw data) -> ZbSpec pg 38
                                    // We are sending direct
                                    params.APSDE_DATA_request.DstAddrMode = APS_ADDRESS_16_BIT;
                                    params.APSDE_DATA_request.DstAddress.ShortAddr.Val = pMsg->type.cluster_list_req.nwk_addr;
                                    params.APSDE_DATA_request.DstEndpoint = EP_ZDO;
                                    params.APSDE_DATA_request.ProfileId.Val = MY_PROFILE_ID;
                                    params.APSDE_DATA_request.ClusterId = SIMPLE_DESC_req;
                                    params.APSDE_DATA_request.SrcEndpoint = EP_ZDO;
                                    //params.APSDE_DATA_request.asduLength;
                                    // Asdu -> TxData
                                    params.APSDE_DATA_request.TxOptions.Val = 0x00; // dds: acknowledged transmission does not work for this message
                                    params.APSDE_DATA_request.DiscoverRoute = ROUTE_DISCOVERY_ENABLE;
                                    params.APSDE_DATA_request.RadiusCounter = DEFAULT_RADIUS;

                                    currentPrimitive = APSDE_DATA_request;
								}
									break;
								
								case ZIB_MSG_CODE_GET_ATTRIBUTE_VALUE_REQ:
								{
									// Send a temperature read request to the other node
		                            ZigBeeBlockTx();
		                            
		                            //LINHA PARA TESTE - REMOVER
									//pMsg->type.get_attribute_value_req.nwk_addr = 0x796F;
									//LINHA DE TESTE, REMOVER !!!! 
									//pMsg->type.get_attribute_value_req.ep = 0x01; //8-light 3-temp 1 -ZiB_EP
									//LINHA DE TESTE, REMOVER !!!! 
									//pMsg->type.get_attribute_value_req.attribute_id = ATTRIBUTE_ZIB_DTP_ANALOG_VOLTS;
									//pMsg->type.get_attribute_value_req.attribute_id = ATTRIBUTE_ZIB_DTP_SENSORS_TEMPERATURE;
									//LINHA DE TESTE, REMOVER !!!! 
									//pMsg->type.get_attribute_value_req.cluster = ZIB_DTP_ANALOG_CLUSTER;
									//pMsg->type.get_attribute_value_req.cluster = ZIB_DTP_SENSORS_CLUSTER;
									
		                            TxBuffer[TxData++] = APL_FRAME_TYPE_KVP | 1;    // KVP, 1 transaction
		                            TxBuffer[TxData++] = APLGetTransId();
		                            TxBuffer[TxData++] = APL_FRAME_COMMAND_GETACK | (Temperature_DegCStr_DATATYPE << 4);
		                            TxBuffer[TxData++] = pMsg->type.get_attribute_value_req.attribute_id & 0xFF;         // Attribute ID LSB
		                            TxBuffer[TxData++] = pMsg->type.get_attribute_value_req.attribute_id & 0xFF;  		// Attribute ID MSB
		
		                            // We are sending direct
		                            params.APSDE_DATA_request.DstAddrMode = APS_ADDRESS_16_BIT;
		                            params.APSDE_DATA_request.DstEndpoint = pMsg->type.get_attribute_value_req.ep;
		                            params.APSDE_DATA_request.DstAddress.ShortAddr.Val = pMsg->type.get_attribute_value_req.nwk_addr;
									
		                            //params.APSDE_DATA_request.asduLength; TxData
		                            params.APSDE_DATA_request.ProfileId.Val = MY_PROFILE_ID;
		                            params.APSDE_DATA_request.RadiusCounter = DEFAULT_RADIUS;
		                            params.APSDE_DATA_request.DiscoverRoute = ROUTE_DISCOVERY_ENABLE;
		                            params.APSDE_DATA_request.TxOptions.Val = 0x04; // dds: acknowledged transmission 04-ack 00-non-ack
		                            params.APSDE_DATA_request.TxOptions.Val = 0;
		                            
		                            //NÃO RECEBO DO MANAGER "SrcEndpoint" e "ClusterId"
		                            /*if(pMsg->type.get_attribute_value_req.ep == EP_TEMPERATURE_RFD){
		                            	params.APSDE_DATA_request.SrcEndpoint = EP_TEMPERATURE;
		                            	params.APSDE_DATA_request.ClusterId = Temperature_CLUSTER;
		                            }
		                            	
		                            else if(pMsg->type.get_attribute_value_req.ep == EP_LIGHT){
		                            	params.APSDE_DATA_request.SrcEndpoint = EP_LIGHT;
		                            	params.APSDE_DATA_request.ClusterId = OnOffSRC_CLUSTER;
		                            }*/
		                            // dds: recebe sim!!!!!
									params.APSDE_DATA_request.SrcEndpoint = pMsg->type.get_attribute_value_req.ep;
		                            params.APSDE_DATA_request.ClusterId = pMsg->type.get_attribute_value_req.cluster;
		                            
		                            TRACE(" Requesting attribute...\r\n" );
		                            
		                            currentPrimitive = APSDE_DATA_request;

								}
									break;
								
								default:
								
								TRACE("\ncase: DEFAULT\n");
								break;
							}
								RCIE = 1;
								RCIF = 1;
						}
                    }
                }
                break;

            default:
                TRACE_CHAR( currentPrimitive );
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
    /*
    	if (ConsoleIsGetReady()) // Tests the console's RCIF
	{
		BYTE byteRx;
		byteRx = ConsoleGet();
		switch (g_smRx)
		{	
			case ERxStateWaitingStartByte:
			{
				if (byteRx == '0')
				{
					//g_smRx = ERxStateWaitingEndOfMessage;
					g_byteMsgBuffer[1] = ZIB_MSG_CODE_RESET_REQ;
					myStatusFlags.bits.bSerialMsgReceived = TRUE;
					// todo: start a timer (ZIB_COMM_TIMEOUT_AFTER_START_BYTE_MS)
				}
				else if (byteRx == '1')
				{	
					g_byteMsgBuffer[1] = ZIB_MSG_CODE_PAN_ID_REQ;
					myStatusFlags.bits.bSerialMsgReceived = TRUE;
				}
				else if (byteRx == '2')
				{	
					g_byteMsgBuffer[1] = ZIB_MSG_CODE_NODE_LIST_REQ;
					myStatusFlags.bits.bSerialMsgReceived = TRUE;
				}
				else if (byteRx == '3')
				{
					g_byteMsgBuffer[1] = ZIB_MSG_CODE_EP_LIST_REQ;
					myStatusFlags.bits.bSerialMsgReceived = TRUE;
				}
				else if (byteRx == '4')
				{
					g_byteMsgBuffer[1] = ZIB_MSG_CODE_CLUSTER_LIST_REQ;
					myStatusFlags.bits.bSerialMsgReceived = TRUE;
				}
				else if (byteRx == '5')
				{
					g_byteMsgBuffer[1] = ZIB_MSG_CODE_GET_ATTRIBUTE_VALUE_REQ;
					myStatusFlags.bits.bSerialMsgReceived = TRUE;
				}	
				else if (byteRx == '6')
				{
					//g_byteMsgBuffer[1] = ZIB_MSG_CODE_GET_ATTRIBUTE_VALUE_REQ;
					//myStatusFlags.bits.bSerialMsgReceived = TRUE;
					put_table();
				}													
				break;
			}

			case ERxStateWaitingEndOfMessage:
			{
				g_byteMsgBuffer[g_iMsgIndex++] = byteRx;

				// todo: clear the SM and the buffer if the timer expires
				if (g_iMsgIndex == g_byteMsgBuffer[0]) // did we receive all data?
				{
					if (g_iMsgIndex > ZIB_MSG_HEADER_SIZE) // Verify minimum message size
					{
						// TRACE ("Pushing a new message into the stack\n");
						// todo: alloc RAM for the message and put it into a FIFO
						myStatusFlags.bits.bSerialMsgReceived = TRUE;
						LED_UNNUSED ^= 1;
						RCIE = 0; // disable further RCIF until we process it
						RCIF = 0;
					}
					else
					{
						// TRACE("Discarding an invalid message\n");
					}

					g_iMsgIndex = 0;
					g_smRx = ERxStateWaitingStartByte;
				}
				else if (g_iMsgIndex > g_byteMsgBuffer[0])
				{
					// TRACE("Unexpected error: Got more data in the buffer then the message size! Discarding buffer\n");
					g_iMsgIndex = 0;
					g_smRx = ERxStateWaitingStartByte;
				}
				break;
			}
		}
	}
}

 */  
    

	if (ConsoleIsGetReady()) // Tests the console's RCIF
	{
		BYTE byteRx;
		byteRx = ConsoleGet();
		switch (g_smRx)
		{	
			case ERxStateWaitingStartByte:
			{
				if (byteRx == ZIB_COMM_START_BYTE)
				{
					g_smRx = ERxStateWaitingEndOfMessage;
					// todo: start a timer (ZIB_COMM_TIMEOUT_AFTER_START_BYTE_MS)
				}											
				break;
			}

			case ERxStateWaitingEndOfMessage:
			{
				g_byteMsgBuffer[g_iMsgIndex++] = byteRx;

				// todo: clear the SM and the buffer if the timer expires
				if (g_iMsgIndex == g_byteMsgBuffer[0]) // did we receive all data?
				{
					if (g_iMsgIndex > ZIB_MSG_HEADER_SIZE) // Verify minimum message size
					{
						// TRACE ("Pushing a new message into the stack\n");
						// todo: alloc RAM for the message and put it into a FIFO
						myStatusFlags.bits.bSerialMsgReceived = TRUE;
						LED_UNNUSED ^= 1;
						RCIE = 0; // disable further RCIF until we process it
						RCIF = 0;
					}
					else
					{
						// TRACE("Discarding an invalid message\n");
					}

					g_iMsgIndex = 0;
					g_smRx = ERxStateWaitingStartByte;
				}
				else if (g_iMsgIndex > g_byteMsgBuffer[0])
				{
					// TRACE("Unexpected error: Got more data in the buffer then the message size! Discarding buffer\n");
					g_iMsgIndex = 0;
					g_smRx = ERxStateWaitingStartByte;
				}
				break;
			}
		}
	}
}
