/*********************************************************************
 *
 *                  Master SPI routintes
 *
 *********************************************************************
 * FileName:        MSPI.c
 * Dependencies:
 * Processor:       PIC18
 * Complier:        MCC18 v1.00.50 or higher
 *                  HITECH PICC-18 V8.10PL1 or higher
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
 * Nilesh Rajbharti     7/12/04 Original
 * Nilesh Rajbharti     11/1/04 Pre-release version
 * DF/KO                04/29/05 Microchip ZigBee Stack v1.0-2.0
 * DF/KO                07/18/05 Microchip ZigBee Stack v1.0-3.0
 * DF/KO                07/27/05 Microchip ZigBee Stack v1.0-3.1
 * DF/KO                01/09/06 Microchip ZigBee Stack v1.0-3.5
 * DF/KO                08/31/06 Microchip ZigBee Stack v1.0-3.6
 ********************************************************************/

/*
 * MSSP - Master Synchronous Serial Port (Pg 161 PIC18F 4620.pdf)
 *
 * Pins:
 * Serial Data Out (SDO) – RC5/SDO
 * Serial Data In (SDI) – RC4/SDI/SDA
 * Serial Clock (SCK) – RC3/SCK/SCL
 *
 * Registers:
 * SSPSTAT - Status register
 * SSPCON1 - Control register 1
 * SSPIF - Interrupt flag
 * SSPBUF - Receive/Transmit Buffer
 * SSPSR - Shifts the data in and out of the device, MSb first.
 * 
 */

#include "MSPI.h"
#include "Zigbee.def"

// @name	SPIPut
// @params	BYTE v - Data byte to be put in the SIMO line
// @brief	Puts v in the SIMO line and pulses 8 clocks in the SCK line
//			If it is desired to get the response from the slave, use SPIGet after
// @return	-
void SPIPut(BYTE v)
{
    SSPIF = 0; // Always clear the interrupt in software
    do
    {
        WCOL = 0;
        SSPBUF = v; // If something is written in the buffer while a transmission/reception is in progress, then WCOL is set
    } while( WCOL ); // WCOL=SSPCON1<bit7>=0 => the byte was written to the buffer and no colision occured (wait the end of transmission)

    while( SSPIF == 0 ); // The interrupt is set after the transmission is completed
}

// @name	SPIGet
// @params	-
// @brief	Puts 0x00 in the SIMO line and pulses 8 clocks in the SCK line
// @return	Returns the data received from the slave, through the SOMI line
BYTE SPIGet(void)
{
    SPIPut(0x00);	// I think: CC2420 SNOP strobe => Make the transceiver return the status bits
    				//          But if the transceiver had received a read command before, it will ignore the 0x00 in it's SI line and keep sending the data which was requested to be read
    				// I think: We have to write something in the buffer in order to make the SCLK to pulse 8 clocks and get 8 bits from the SOMI line
    return SSPBUF;
}

// The ZMD44101 requires SS to be driven high between reading bytes,
// so these functions are not used and should not be compiled.
#if !defined(USE_ZMD44101)
void SPIGetArray(BYTE *buffer, BYTE len)
{
    while( len-- )
        *buffer++ = SPIGet();
}

void SPIPutArray(BYTE *buffer, BYTE len)
{
    while( len-- )
        SPIPut(*buffer++);
}
#endif



