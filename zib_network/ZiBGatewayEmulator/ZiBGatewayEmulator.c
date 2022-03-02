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

#include "..\..\common\ZiBManagerGatewayProtocol.h"
#include "..\ZigBeeStack\zZiBPlatform.h"

#include <delays.h>
#include <stdio.h>


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
		BYTE	bSerialMsgReceived         : 1;
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

DWORD g_dwLastChannelMask;

int iDiscoveryEntryCount;
EP_DISCOVERY_ENTRY tableDiscovery[MAX_DISCOVERY_ENTRY];

int iIndex; // index for general purposes

typedef enum
{
ERxStateWaitingStartByte,
ERxStateWaitingEndOfMessage
}  ERxState;

ERxState g_smRx;
BYTE g_byteMsgBuffer[50];
int g_iMsgIndex;

int g_iRand;

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
	iDiscoveryEntryCount = 0;
	memset((void*)&tableDiscovery, 0, sizeof(tableDiscovery));
	g_smRx = ERxStateWaitingStartByte;
	g_iMsgIndex = 0;
	g_iRand = 1; // Initial seed
}

void main(void)
{
    CLRWDT();
    ENABLE_WDT();

    currentPrimitive = NO_PRIMITIVE;
    
    ConsoleInit();
    HardwareInit();
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
		if (myStatusFlags.bits.bSerialMsgReceived == TRUE)
		{
			ZiBMessage msg;
			BYTE* pWriter;
			ZiBNode nodeList[2];
			BYTE clusterList[2];
			char szTemperature[10];
			ZiBMessage* pMsg = (ZiBMessage*)&g_byteMsgBuffer;

			myStatusFlags.bits.bSerialMsgReceived = FALSE;
			switch (pMsg->code)
			{
				case ZIB_MSG_CODE_RESET_REQ:
				{
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
					LED_LIGHT ^= 1;
				}
					break;				
				case ZIB_MSG_CODE_PAN_ID_REQ:
				{
				    msg.length = ZIB_MSG_HEADER_SIZE 
				    	+ sizeof(msg.type.pan_id_conf);
				    msg.code = ZIB_MSG_CODE_PAN_ID_CONF;
				    msg.type.pan_id_conf.status = ZIB_ERROR_SUCCESS;
				    msg.type.pan_id_conf.pan_id = 0xABCD;
					
				    ConsolePut(ZIB_COMM_START_BYTE);
				    pWriter = (BYTE*)&msg;
				    for (iIndex=0; iIndex<msg.length;iIndex++)
				    {
					    ConsolePut(pWriter[iIndex]);
					}
					LED_LIGHT ^= 1;
				}
					break;
				case ZIB_MSG_CODE_NODE_LIST_REQ:
				{
					nodeList[0].mac_addr[7] = 0xAA;
					nodeList[0].mac_addr[6] = 0xBB;
					nodeList[0].mac_addr[5] = 0xCC;
					nodeList[0].mac_addr[4] = 0xDD;
					nodeList[0].mac_addr[3] = 0xEE;
					nodeList[0].mac_addr[2] = 0xFF;
					nodeList[0].mac_addr[1] = 0x00;
					nodeList[0].mac_addr[0] = 0x01;
					nodeList[0].nwk_addr = 0x796F;
					nodeList[0].profile_id = MY_PROFILE_ID;
					nodeList[0].device_id = ZIB_DTP_DEV_ID;
			
					nodeList[1].mac_addr[7] = 0xAA;
					nodeList[1].mac_addr[6] = 0xBB;
					nodeList[1].mac_addr[5] = 0xCC;
					nodeList[1].mac_addr[4] = 0xDD;
					nodeList[1].mac_addr[3] = 0xEE;
					nodeList[1].mac_addr[2] = 0xFF;
					nodeList[1].mac_addr[1] = 0x00;
					nodeList[1].mac_addr[0] = 0x02;
					nodeList[1].nwk_addr = 0x7970;
					nodeList[1].profile_id = MY_PROFILE_ID;
					nodeList[1].device_id = ZIB_DTP_DEV_ID;
			
				    msg.type.node_list_conf.count = 2;
				    msg.length = ZIB_MSG_HEADER_SIZE 
				    	+ sizeof(msg.type.node_list_conf) -1
				    	+ (msg.type.node_list_conf.count*sizeof(ZiBNode));
				    msg.code = ZIB_MSG_CODE_NODE_LIST_CONF;
				    msg.type.node_list_conf.status = ZIB_ERROR_SUCCESS;
					
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
					LED_LIGHT ^= 1;
				}
					break;
				case ZIB_MSG_CODE_EP_LIST_REQ:
				{
				    msg.type.ep_list_conf.count = 2;
				    msg.length = ZIB_MSG_HEADER_SIZE 
					    + sizeof(msg.type.ep_list_conf) -1
				    	+ (msg.type.ep_list_conf.count*sizeof(BYTE));
				    msg.code = ZIB_MSG_CODE_EP_LIST_CONF;
				    msg.type.ep_list_conf.status = ZIB_ERROR_SUCCESS;
				    msg.type.ep_list_conf.nwk_addr = pMsg->type.ep_list_req.nwk_addr;
					
				    ConsolePut(ZIB_COMM_START_BYTE);
				    // transmit the header and the node count
				    pWriter = (BYTE*)&msg;
				    for (iIndex=0; iIndex < (ZIB_MSG_HEADER_SIZE + sizeof(msg.type.ep_list_conf) -1); iIndex++)
				    {
					    ConsolePut(pWriter[iIndex]);
					}
					
					// transmit the ep list
				    ConsolePut(0x00);
			   	    ConsolePut(0x01);
					LED_LIGHT ^= 1;
				}
					break;
				case ZIB_MSG_CODE_CLUSTER_LIST_REQ:
				{
					clusterList[0] = ZIB_DTP_ANALOG_CLUSTER;
					clusterList[1] = ZIB_DTP_SENSORS_CLUSTER;
			
				    msg.type.cluster_list_conf.count = 2;
				    msg.length = ZIB_MSG_HEADER_SIZE
					    + sizeof(msg.type.cluster_list_conf) -1
				    	+ (msg.type.cluster_list_conf.count*sizeof(BYTE));
				    msg.code = ZIB_MSG_CODE_CLUSTER_LIST_CONF;
				    msg.type.cluster_list_conf.status = ZIB_ERROR_SUCCESS;
				    msg.type.cluster_list_conf.nwk_addr = pMsg->type.cluster_list_req.nwk_addr;
				    msg.type.cluster_list_conf.ep = pMsg->type.cluster_list_req.ep;
				    
				    ConsolePut(ZIB_COMM_START_BYTE);
				    // transmit the header and the node count
				    pWriter = (BYTE*)&msg;
				    for (iIndex=0; iIndex < (ZIB_MSG_HEADER_SIZE + sizeof(msg.type.cluster_list_conf) -1); iIndex++)
				    {
					    ConsolePut(pWriter[iIndex]);
					}
					
					// transmit the cluster list
					pWriter = (BYTE*)clusterList;
				    for (iIndex=0; iIndex < (msg.type.cluster_list_conf.count*sizeof(BYTE)); iIndex++)
				    {
					    ConsolePut(pWriter[iIndex]);
					}
					LED_LIGHT ^= 1;
				}
					break;
				
				case ZIB_MSG_CODE_GET_ATTRIBUTE_VALUE_REQ:
				{
					srand(g_iRand);
					
					
					if (pMsg->type.get_attribute_value_req.cluster == ZIB_DTP_ANALOG_CLUSTER && pMsg->type.get_attribute_value_req.attribute_id == ATTRIBUTE_ZIB_DTP_ANALOG_VOLTS)
					{
						g_iRand = rand()/16383*20; // limit to 20V
						sprintf(szTemperature, (ROM char*)"%d V", g_iRand);
					}
					else if (pMsg->type.get_attribute_value_req.cluster == ZIB_DTP_SENSORS_CLUSTER && pMsg->type.get_attribute_value_req.attribute_id == ATTRIBUTE_ZIB_DTP_SENSORS_TEMPERATURE)
					{
						g_iRand = rand()/16383*100; // limit to 100 C
						sprintf(szTemperature, (ROM char*)"%d C", g_iRand);
					}
					else
					{
						strcpy(szTemperature, (ROM char*)"0000");
					}
					
				    msg.length = ZIB_MSG_HEADER_SIZE 
				    	+ sizeof(msg.type.get_attribute_value_conf) -1
				    	+ strlen(szTemperature);
				    msg.code = ZIB_MSG_CODE_GET_ATTRIBUTE_VALUE_CONF;
				    msg.type.get_attribute_value_conf.status = ZIB_ERROR_SUCCESS;
				    msg.type.get_attribute_value_conf.nwk_addr = pMsg->type.get_attribute_value_req.nwk_addr;
				    msg.type.get_attribute_value_conf.ep = pMsg->type.get_attribute_value_req.ep;
				    msg.type.get_attribute_value_conf.cluster = pMsg->type.get_attribute_value_req.cluster;
				    msg.type.get_attribute_value_conf.attribute_id = pMsg->type.get_attribute_value_req.attribute_id;
				    msg.type.get_attribute_value_conf.size = strlen(szTemperature);
					
				    // transmit the beginning
				    ConsolePut(ZIB_COMM_START_BYTE);
				    pWriter = (BYTE*)&msg;
				    for (iIndex=0; iIndex<(ZIB_MSG_HEADER_SIZE + sizeof(msg.type.get_attribute_value_conf) -1);iIndex++)
				    {
					    ConsolePut(pWriter[iIndex]);
					}
					
					// transmit the string
					pWriter = (BYTE*) szTemperature;
				    for (iIndex=0; iIndex < msg.type.get_attribute_value_conf.size; iIndex++)
				    {
					    ConsolePut(pWriter[iIndex]);
					}
					LED_LIGHT ^= 1;
				}
					break;
				
				default:
					break;
			}
			
			RCIE = 1;
			RCIF = 1;
		}
	}
	
	while(1) CLRWDT();
    // dds: test end
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
		BYTE byteRx = ConsoleGet();
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

