/*********************************************************************
 *
 *                  ZigBee Header File
 *
 *********************************************************************
 * FileName:        zigbee.h
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

#include "generic.h"
#include "zigbee.def"

#define MAINS_POWERED               0x00
#define NOT_MAINS_POWERED           0x01

// RF Chip Choices
#define CC2420      1
#define MRF24J40    2
#define UZ2400      3

// Frequency Band Choices
#define FB_2400GHz 2400 //2400–2483.5
#define FB_915MHz 915  //902-928
#define FB_868MHz 868  //868-868.6

typedef struct _LONG_ADDR
{
    BYTE v[8];
} LONG_ADDR;

typedef union _SHORT_ADDR
{
    struct _SHORT_ADDR_bits
    {
        BYTE LSB;
        BYTE MSB;
    } byte;
    WORD Val;
    BYTE v[2];
} SHORT_ADDR;

typedef union _ADDR
{
    LONG_ADDR LongAddr;
    SHORT_ADDR ShortAddr;
    BYTE v[8];
} ADDR;

typedef union _GTS_HEADER
{
    BYTE Val;
    struct _GTS_HEADER_bits
    {
        unsigned int GTSDescriptorCount :3;
        unsigned int :4;
        unsigned int GTSPermit :1;
    } bits;
} GTS_HEADER;

typedef SHORT_ADDR PAN_ADDR;


