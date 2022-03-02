/*********************************************************************
 *
 *                  ZigBee Symbol Timer Header File
 *
 *********************************************************************
 * FileName:        SymbolTime.h
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

#include "zigbee.def"
#include  "zigbee.h"
#include "generic.h"

/* this section is based on the Timer 0 module of the PIC18 family */
#if(FREQUENCY_BAND == 2400)
    #if(CLOCK_FREQ <= 250000)
        #define CLOCK_DIVIDER 1
        #define CLOCK_DIVIDER_SETTING 0x08 /* no prescalar */
        #define SYMBOL_TO_TICK_RATE 250000
    #elif(CLOCK_FREQ <= 500000)
        #define CLOCK_DIVIDER 2
        #define CLOCK_DIVIDER_SETTING 0x00
        #define SYMBOL_TO_TICK_RATE 500000
    #elif(CLOCK_FREQ <= 1000000)
        #define CLOCK_DIVIDER 4
        #define CLOCK_DIVIDER_SETTING 0x01
        #define SYMBOL_TO_TICK_RATE 1000000
    #elif(CLOCK_FREQ <= 2000000)
        #define CLOCK_DIVIDER 8
        #define CLOCK_DIVIDER_SETTING 0x02
        #define SYMBOL_TO_TICK_RATE 2000000
    #elif(CLOCK_FREQ <= 4000000)
        #define CLOCK_DIVIDER 16
        #define CLOCK_DIVIDER_SETTING 0x03
        #define SYMBOL_TO_TICK_RATE 4000000
    #elif(CLOCK_FREQ <= 8000000)
        #define CLOCK_DIVIDER 32
        #define CLOCK_DIVIDER_SETTING 0x04
        #define SYMBOL_TO_TICK_RATE 8000000
    #elif(CLOCK_FREQ <= 16000000)
        #define CLOCK_DIVIDER 64
        #define CLOCK_DIVIDER_SETTING 0x05
        #define SYMBOL_TO_TICK_RATE 16000000
    #elif(CLOCK_FREQ <= 3200000)
        #define CLOCK_DIVIDER 128
        #define CLOCK_DIVIDER_SETTING 0x06
        #define SYMBOL_TO_TICK_RATE 32000000
    #else
        #define CLOCK_DIVIDER 256
        #define CLOCK_DIVIDER_SETTING 0x07
        #define SYMBOL_TO_TICK_RATE 32000000
    #endif

    #define ONE_SECOND ((CLOCK_FREQ/1000 * 62500) / (SYMBOL_TO_TICK_RATE / 1000))
    /* SYMBOLS_TO_TICKS to only be used with input (a) as a constant, otherwise you will blow up the code size */
    #define SYMBOLS_TO_TICKS(a) ((CLOCK_FREQ/1000 * a) / (SYMBOL_TO_TICK_RATE / 1000))
#endif

#define TickGetDiff(a,b) (a.Val - b.Val)

typedef union _TICK
{
    DWORD Val;
    struct _TICK_bytes
    {
        BYTE b0;
        BYTE b1;
        BYTE b2;
        BYTE b3;
    } byte;
    BYTE v[4];
} TICK;

void InitSymbolTimer(void);
TICK TickGet(void);

extern volatile BYTE timerExtension1;
extern volatile BYTE timerExtension2;
