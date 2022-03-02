/*********************************************************************
 *
 *                  ZigBee Symbol Timer
 *
 *********************************************************************
 * FileName:        SymbolTime.c
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
#include "SymbolTime.h"
#include "Compiler.h"
#include "generic.h"

volatile BYTE timerExtension1;
volatile BYTE timerExtension2;

void InitSymbolTimer()
{
    T0CON = 0b00000000 | CLOCK_DIVIDER_SETTING;
    TMR0IP = 1;
    TMR0IF = 0;
    TMR0IE = 1;
    TMR0ON = 1;

    timerExtension1 = 0;
    timerExtension2 = 0;
}


/* caution: this function should never be called when interrupts are disabled */
/* if interrupts are disabled when this is called then the timer might rollover */
/* and the byte extension would not get updated. */
TICK TickGet(void)
{
    TICK currentTime;

    /* copy the byte extension */
    currentTime.byte.b2 = 0;
    currentTime.byte.b3 = 0;

    /* disable the timer to prevent roll over of the lower 16 bits while before/after reading of the extension */
    TMR0IE = 0;

    /* read the timer value */
    currentTime.byte.b0 = TMR0L;
    currentTime.byte.b1 = TMR0H;

    //if an interrupt occured after IE = 0, then we need to figure out if it was
    //before or after we read TMR0L
    if(TMR0IF)
    {
        if(currentTime.byte.b0<10)
        {
            //if we read TMR0L after the rollover that caused the interrupt flag then we need
            //to increment the 3rd byte
            currentTime.byte.b2++;  //increment the upper most
            if(timerExtension1 == 0xFF)
            {
                currentTime.byte.b3++;
            }
        }
    }

    /* copy the byte extension */
    currentTime.byte.b2 += timerExtension1;
    currentTime.byte.b3 += timerExtension2;

    /* enable the timer*/
    TMR0IE = 1;

    return currentTime;
}
