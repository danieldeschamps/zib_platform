/*********************************************************************
 *
 * This program uses subroutines from Microchip USB C18 Firmware Version 1.0
 *
 *********************************************************************
 * FileName:        main.c
 * Dependencies:    See INCLUDES section below
 * Processor:       PIC18
 * Compiler:        C18 2.30.01+
 * Author:          Robert B. Lang 
 * This software works with the hardware described in the article
 * USB the Easy Way by Robert Lang and subroutines from MICROCHIP 
 * Communication Device Class (CDC) Firmware Revision 1 at 
 * http://www.microchip.com/stellent/idcplg?IdcService=SS_GET_PAGE&nodeId=2124&param=en022625
 *
 * THIS SOFTWARE IS PROVIDED IN AN “AS IS” CONDITION. NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 * TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE AUTHOR SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
/** I N C L U D E S **********************************************************/
#include <p18cxxx.h>
#include "system\typedefs.h"                        // Required
#include "system\usb\usb.h"                         // Required

// Configuration bits are defined in PIC18 Configuration Settings Addendum
// DS51537A page 36-39
// 48 MHz CPU clock derived from = 20 Mhz crystal input
// Full speed USB clock= 96 MHz/2
// Oscillator selection bits=HS, PLL enabled, HS used by USB
//
/*
#pragma config PLLDIV=5, CPUDIV=OSC1_PLL2, USBDIV=2
#pragma config IESO=OFF, FCMEM=OFF, FOSC=HSPLL_HS 
#pragma config PWRT=OFF, BOR=ON, BORV=21, VREGEN=ON, MCLRE=ON
#pragma config PBADEN=OFF, STVREN=ON, LVP=OFF, ICPRT=OFF 
#pragma config XINST=OFF, DEBUG=OFF, WDT=OFF
*/
/** U S B ***********************************************************/
#define usb_bus_sense       1
#define self_power          1

/** L E D ***********************************************************/
#define mInitAllLEDs()      LATB &= 0x00; TRISB &= 0x00;
#define mLED_1              LATBbits.LATB0
#define mLED_2              LATBbits.LATB1
#define mLED_3              LATBbits.LATB2
#define mLED_4              LATBbits.LATB3
#define mLED_5              LATBbits.LATB4
#define mLED_1_On()         mLED_1 = 1;
#define mLED_2_On()         mLED_2 = 1;
#define mLED_3_On()         mLED_3 = 1;
#define mLED_4_On()         mLED_4 = 1;
#define mLED_5_On()         mLED_5 = 1;
#define mLED_1_Off()        mLED_1 = 0;
#define mLED_2_Off()        mLED_2 = 0;
#define mLED_3_Off()        mLED_3 = 0;
#define mLED_4_Off()        mLED_4 = 0;
#define mLED_5_Off()        mLED_5 = 0;
#define mLED_1_Toggle()     mLED_1 = !mLED_1;
#define mLED_2_Toggle()     mLED_2 = !mLED_2;
#define mLED_3_Toggle()     mLED_3 = !mLED_3;
#define mLED_4_Toggle()     mLED_4 = !mLED_4;
#define mLED_5_Toggle()     mLED_5 = !mLED_5;
#define OLEN 128  // size of USART buffers in characters
#define ILEN 128

/** V A R I A B L E S ********************************************************/
#pragma udata
char usb_led_status;
static volatile unsigned char oend=0;
static volatile unsigned char ostart=0;
static volatile unsigned char iend=0;
static volatile unsigned char istart=0;
static volatile unsigned char bsendfull=0;
static char usb_input_buffer[64];  // USER USB WORKING BUFFERS
static char usb_output_buffer[64];

#pragma udata usbram7a=0x700  //SET UP MEMORY BLOCK FOR SERIAL BUFFERS
volatile char serial_input_buffer[128];
volatile char serial_output_buffer[128];
#pragma udata

/** P R I V A T E  P R O T O T Y P E S ***************************************/
void UserInit(void);
void usart_handler(void);
unsigned char USB_to_USART_handler(void);
void USART_to_USB_handler(void);
void USBTasks(void);

#pragma code int_vector=0x08   // SET INTERRUPT VECTOR LOCATION
void int_vector (void)
{
  _asm goto usart_handler _endasm
}
#pragma code
#pragma interrupt usart_handler
//**************************************************************************
// usart_handler(void) USART interrupts are handled and 
// one character is read or written.
//**************************************************************************
void usart_handler(void)
{
   unsigned char c;
// UART RECEIVE INTERRUPT 
 	if ((PIE1bits.RCIE==1) && (PIR1bits.RCIF==1  )) 
 { // UART receive interrupt get processed first
   //PORTB=8; //turn on led just to know receive interrupt is working// tirado fora por MAW
MOREDATA:
    if (RCSTAbits.FERR | RCSTAbits.OERR) 
	    { // were there framing or overflow errors 
	    goto EXIT;
	    }	
 	c=RCREG; // read char from UART receive register
 	 RCREG=0;
//this will also clear receive interrupt flag when read
 	if (istart + ILEN != iend)  // is there room in the input buffer?
 	{
 	 serial_input_buffer[iend++ & (ILEN-1)]=c; // load char in input buffer
 	 if (PIR1bits.RCIF==1) goto MOREDATA;
 	} 
 	
	else
EXIT:	 PORTB=PORTB;  // light serial error if no room in buffer
 }
else 
 {
// UART TRANSMIT INTERRUPT  	
	if (( PIE1bits.TXIE==1) && (PIR1bits.TXIF==1  ))
	{ 
		// UART transmit register empty?
 	 	if (ostart != oend)  // is there something in output buffer?
 	        {  // Yes, then send it
    TXREG=serial_output_buffer[ostart++ & (OLEN-1)]; // load char from buffer to UART transmit register
   // this will also set bTXIF after transmission. 

    bsendfull=0; // output buffer is no longer full
//   PORTB=4;  //turn on led to show transmit interrupt is working
	
  	        }
 	    else  // Nothing to send, disable transmit interrupt
 	   PIE1bits.TXIE=0;

    } 
}}

#pragma code
//***************************************************************************
//   BlinkUSBStatus(void) turns on and off LEDs corresponding to
//   the USB device state.
//***************************************************************************
void BlinkUSBStatus(void)
{
    static word led_count=0;
    if(led_count == 0)led_count = 10000U;
    led_count--;
    #define mLED_Both_Off()         {mLED_1_Off();mLED_2_Off();}
    #define mLED_Both_On()          {mLED_1_On();mLED_2_On();}
    #define mLED_Only_1_On()        {mLED_1_On();mLED_2_Off();}
    #define mLED_Only_2_On()        {mLED_1_Off();mLED_2_On();}

    if(UCONbits.SUSPND == 1)
    {
        if(led_count==0)
        {
            mLED_1_Toggle();
            mLED_2 = mLED_1;        // Both blink at the same time
        }//end if
    }
    else
    {
        if(usb_device_state == DETACHED_STATE)
        {
            mLED_Both_Off();
        }
        else if(usb_device_state == ATTACHED_STATE)
        {
            mLED_Both_On();
        }
        else if(usb_device_state == POWERED_STATE)
        {
            mLED_Only_1_On();
        }
        else if(usb_device_state == DEFAULT_STATE)
        {
            mLED_Only_2_On();
        }
        else if(usb_device_state == ADDRESS_STATE)
        {
            if(led_count == 0)
            {
                mLED_1_Toggle();
                mLED_2_Off();
            }//end if
        }
        else if(usb_device_state == CONFIGURED_STATE)
        {
            if(led_count==0)
            {
                mLED_1_Toggle();
                mLED_2 = !mLED_1;       // Alternate blink                
            }//end if
        }//end if(...)
    }//end if(UCONbits.SUSPND...)
}//end BlinkUSBStatus

//*****************************************************************************	
// void DataTransfer(void) Handles USB and USART transfers
//*****************************************************************************
void DataTransfer(void)
{	unsigned char RET;
	if (usb_led_status) BlinkUSBStatus(); // Show USB status in LEDs
    if((usb_device_state < CONFIGURED_STATE)||(UCONbits.SUSPND==1)) return;
   
    RET=USB_to_USART_handler(); // USB data to RS-232
    USBTasks();  // USB Tasks
    USART_to_USB_handler();     // RS-232 data to USB 
   
}  //end DataTransfer	

//*****************************************************************************	
// display_leds(unsigned char ledval)  Controls LED display based on data received
// from USB or USART.
//*****************************************************************************
void display_leds(unsigned char ledval)
{	           
  switch (ledval)
       {
         case '0': 
         if (usb_led_status==0)
         {
          PORTB=0; // clear all leds
          usb_led_status=1;
         }
         else
         {
         usb_led_status=0;
         PORTB=0; // clear all leds
         }
         break; 
         case '1':  mLED_1_Toggle();
         break;
         case '2':  mLED_2_Toggle();
         break; 
         case '3':  mLED_3_Toggle();
         break;
         case '4':  mLED_4_Toggle();
         break; 
         case '5':  mLED_5_Toggle();
       }  
} //end display_leds

//*****************************************************************************
//  InitializeSystem(void) is a centralize initialization routine.
//  All required USB initialization routines are called from here.
//*****************************************************************************
static void InitializeSystem(void)
{
    ADCON1 |= 0x0F;                 // Default all pins to digital
    mInitializeUSBDriver();         // See usbdrv.h
    UserInit();                     // Initialize LEDs and USART
}  //end InitializeSystem

//***************************************************************************
//  InitializeUSART(void) initializates the USART to 115,200 bits per second.  
//***************************************************************************
void InitializeUSART(void)
{
    INTCONbits.GIEL=0;  // DISable global interrupts low	
	INTCONbits.GIEH=0; // disable global interrupts high temporarily 
	RCONbits.IPEN=1 ; //ENABLE PERIPHERAL INTERRUPT PRIORITIES
	IPR1bits.RCIP=1;
	IPR1bits.TXIP=1;
    TRISCbits.TRISC7=1; // RX
    TRISCbits.TRISC6=0; // TX//
    //TXSTA = 0x24;       // TX enable, BRGH=1 115200bps
	TXSTA = 0x20;       // TX enable - MAW BRGH=0 - 19200bps / 9600 bps
    RCSTA = 0x90;       // continuous RX
	BAUDCON = 0x00;     // MAW - BRG16 = 0
//   baud  spbrgh:spbrg for cpu clock of 48Mhz    
//   9600  4E2
//  19200  271
//  31250  180
//  38400  138
//  57600   D0
// 115200   68
    SPBRG = 19;  // MAW 19200
	//SPBRG = 12;	//MAW 115200
	//SPBRG = 39; // MAW 9600	
    //RCSTAbits.CREN=0;  // clear continous receive bit
    RCSTAbits.CREN=1;	/* set continous receive bit */  
}  //end InitializeUSART

//****************************************************************************
//   EnableInterrupts(void) enables USART interrupts. 
//****************************************************************************

void EnableInterrupts(void)
{
// enable uart interrupts      
     PIE1bits.RCIE=1; // enable uart receive interrupt    
     PIE1bits.TXIE=1;   // enable uart transmit interrupt
     INTCONbits.GIEH=1;  // enable global interrupts high  
}

//*****************************************************************************
// serial_receive receives byte from serial port                    
//*****************************************************************************
unsigned char serial_receive(void) 
{			if( iend == istart )  // no data in buffer
			return 0x00;
	     	return serial_input_buffer[istart++ & (ILEN-1)]; 
}

//*****************************************************************************
// serial_rxbytes returns number of bytes in rx buffer             
//*****************************************************************************
unsigned char serial_rxbytes(void)
{        
	    if  (iend >= istart)  
		return  (iend - istart); 
		else
		return ILEN -(( istart - iend)& (ILEN-1)) ;
	
}
//*****************************************************************************	
// serial_send send byte of data received rom USB over serial port
//*****************************************************************************
void serial_send(unsigned char c)  
{		while( bsendfull );
		INTCONbits.GIEH=0;	// Disable all Interrupts
		serial_output_buffer[(oend++) & (OLEN-1)] = c; //store char in buffer
		PIE1bits.TXIE=1; // enable transmit interrupt to clear out buffer
		if( ((oend ^ ostart) & (OLEN-1)) == 0 ) bsendfull = 1;
	   	INTCONbits.GIEH=1;	// Enable all Interrupts
} 

//****************************************************************************
// hex( unsigned char value) outputs an unsigned char as
//  two ascii characters representing its hex value.
//****************************************************************************
void hex( unsigned char value)
{
	static unsigned char TEST[2];
	static unsigned char HV;
	static unsigned char i;
	TEST[0]=value>>4;  // get high order nibble
	TEST[1]=value & 0x0F;  //mask out high order nibble leaving low
	for (i=0; i<2;i++) //output two hex characters per char
	{
	 switch (TEST[i])  // assign the ascii
	 {
case 1:
	 HV='1';
	 break;
case 2:
	 HV='2';
	 break;
case 3:
	 HV='3';
	 break;
case 4:
	 HV='4';
	 break;
case 5:
	 HV='5';
	 break;
case 6:
	 HV='6';
	 break;
case 7:
	 HV='7';
	 break;
case 8:
	 HV='8';
	 break;
case 9:
	 HV='9';
	 break;	
case 0xA:
	 HV='a';
	 break;
case 0xB:
	 HV='b';
	 break;
case 0xC:
	 HV='c';
	 break;
case 0xD:
	 HV='d';
	 break;
case 0xE:
	 HV='e';
	 break;
case 0xF:
	 HV='f';
	 break;
default:
     HV='0';	 	 	 	 
	}
	serial_send(HV); // output ascii character
	}
	serial_send(' '); // output blank between words 
	}
	
//*****************************************************************************
// serial_txbytes_free returns number of free bytes in tx buffer  
//*****************************************************************************
unsigned char serial_txbytes_free(void)
{	if (oend >= ostart)
        return OLEN - (oend - ostart); 		
		else
		return OLEN + (oend - ostart);
}

//*****************************************************************************
// USART_to_USB_handler(void) This routine takes data from serial port 
// and sends to USB 
//*****************************************************************************
void USART_to_USB_handler(void)
{	volatile unsigned char nn;
	unsigned char index;
    unsigned char mychar;
    unsigned char delay;
// Is there something in the serial inbuffer and is USB input stream available?
	nn=serial_rxbytes();
	if (nn>0)
	{
	if (mUSBUSARTIsTxTrfReady())
	  {	for (index=0;index<nn;index++)
		{
		mychar=serial_receive();//get char from USART input buffer 
			 for (delay=20;delay>0;delay--) //MAW
		 	 	mLED_3_Toggle(); // MAW
		 	 mLED_3_Off(); // MAW
	    	// if (mychar == 0) return; // this should never happen with ASCII data
	   	//display_leds(mychar); // display received data in leds - TIRADO FORA BY MAW
	    usb_output_buffer[index] = mychar;  // and load in usb buffer
		}
         mUSBUSARTTxRam((byte *) usb_output_buffer,nn); //queue usb buffer to send
      }
 //     hex(nn);  // output bytes sent for debugging only
    } 
}  // END OF USART_to_USB_handler

//************************************************************************
// unsigned char USB_to_USART_handler(void)
// This routine takes data from the USB and sends to serial port.
//************************************************************************
unsigned char USB_to_USART_handler(void)
// returns 1 data is received and sent successfully
// returns 0 if no data 
{	unsigned char count;
	unsigned char index;
	unsigned char delay;
 if (getsUSBUSART(usb_input_buffer,CDC_BULK_OUT_EP_SIZE)) // 0 if no data ready
        {
          count=mCDCGetRxLength();
           // Special bytes turn on LEDS
          //for (index=0;index<count;index++)  //TIRADO POR MAW
          //display_leds(usb_input_buffer[index]); // TIRADO FORA BY MAW
                     
	while( serial_txbytes_free() < count ) {} // wait here until USART buffer partially unloaded
    for (index=0;index<count;index++)
     {serial_send( usb_input_buffer[index]);}  // send data to usart (load in usart output buffer)
	count=0;
	for (delay=20;delay>0;delay--) //MAW
		mLED_4_Toggle(); // MAW
	mLED_4_Off(); // MAW
	return 1; // data sent
	}
	return 0; // no data  
}// USB_to_USART_handler

//*****************************************************************************
// void USBTasks(void)  Service loop for USB tasks.
//*****************************************************************************
void USBTasks(void)
{
    /*
     * Servicing Hardware
     */
    USBCheckBusStatus();   // Must use polling method in usbdrv.c
    if(UCFGbits.UTEYE!=1)
        USBDriverService(); // Interrupt or polling method in usbdrv.c
    
    #if defined(USB_USE_CDC)
    CDCTxService();  // this handles all USB CDC traffic  
    #endif
}// end USBTasks

//*****************************************************************************
// UserInit(void) Flashes LEDs on initial startup and calls USART intialization routine.
//*****************************************************************************	
void UserInit(void)
{  	const unsigned char led[]={1,2,4,8,16};
	unsigned char i;
	unsigned short j;
    mInitAllLEDs();
	for (i=0; i<5;i++)
	{
	PORTB=led[i]; // flash leds to make sure chip is running
	for (j=0;j<31000;j++) {};  // to slow down flashes so we can see (31000)
    }
    PORTB=0; //clear leds
    InitializeUSART();   
}//end UserInit

//****************************************************************************
// main(void) the main program
//****************************************************************************
void main(void)
{
    InitializeSystem(); // Flash LEDs, Initialize USB, Initialize USART
    usb_led_status=1;   // Set flag to show USB status in LEDs
    EnableInterrupts();
    while(1) 
    {	
	   	USBTasks();  // USB Tasks 
	    DataTransfer(); // Show USB status in LEDs and handle USB/USART data transfers
	    
	}     
} //end main
//===================================================================================
