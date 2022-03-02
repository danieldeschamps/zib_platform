/*********************************************************************
 *
 *                  TC77 Temperature Sensor Task File
 *
 *********************************************************************
 * FileName:        TC77.c
 * Dependencies:
 * Processor:       PIC18F
 * Complier:        MCC18 v2.30 or higher
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
 * HiTech PICC18 Compiler Options excluding device selection:
 *                  -FAKELOCAL -G -Zg -E -C
 *
 * Author               Date    Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Howard Schlunder     1/4/05  Initial file creation
 * DF/KO                04/29/05 Microchip ZigBee Stack v1.0-2.0
 * DF/KO                01/09/06 Microchip ZigBee Stack v1.0-3.5
 * DF/KO                08/31/06 Microchip ZigBee Stack v1.0-3.6
 ********************************************************************/

#include "generic.h"
#include "TC77.h"
#include "Console.h"
#include "string.h"
#include "MSPI.h"

#define TC77_CS             LATA2

/*********************************************************************
 * Function:        BYTE GetTC77String( char *buffer )
 *
 * PreCondition:    None
 *
 * Input:           buffer - pointer where to store the string.
 *
 * Output:          The length of the returned string.
 *
 * Side Effects:    None
 *
 * Overview:        This function reads the TC77 and returns a formatted
 *                  temperature value at *buffer.  If we get a 0x00
 *                  or 0xFF, we report that we are having a problem
 *                  with the temperature sensor.
 *
 * Note:
 ********************************************************************/
BYTE GetTC77String( char *buffer )
{
    typedef union _SIGNED_WORD_VAL
    {
        unsigned int uVal;
        int Val;
        struct
        {
            BYTE LSB;
            BYTE MSB;
        } byte;
    } SIGNED_WORD_VAL;

    BYTE            GIEHsave;
    char            *ptr;
    char            *pTempString;
    SIGNED_WORD_VAL RawTemp;
    signed long     ScaledTemp;
    BYTE            strLen;
    char            tempString[10];

    // Read the temperature from the TC77
    //
    // The TC77 gives a left aligned 13 bit signed integer
    // where 0 = 0 degrees Celcius, 1 = 0.0625 degrees,
    // 2 = 0.125 degrees, -1 = -0.0625 degrees,
    // -2 = -0.125 degrees, 400 = 25 degrees, etc.
    // To convert it to a usable value, one should
    // right align it and then sign extend it.  For human
    // display purposes, one should either convert it to a
    // floating point number and multiply it by 0.0625
    // degrees C.  Alternatively, it may be left as an integer
    // and multiplied by 625.  The result would be scaled by
    // a factor of 10,000.

    // Disable interrupts while we talk to the temperature sensor.
    GIEHsave = GIEH;
    GIEH = 0;

    // Read data from the TC77
    TC77_CS = 0;                    // Select TC77
    RawTemp.byte.MSB = SPIGet();    // Get high 8 bits
    RawTemp.byte.LSB = SPIGet();    // Get low 5 bits + "Bit 2" + 2 don't care bits (LSb side)
    TC77_CS = 1;                    // Unselect TC77

    // Restore interrupts.
    GIEH = GIEHsave;

    if ((RawTemp.uVal == 0x0000) || (RawTemp.uVal == 0xFFFF))
    {
        strcpypgm2ram( buffer, (rom char *)"Problem reading TC77." );
    }
    else
    {
        // Right align the 13 bit temperature reading
        RawTemp.uVal >>= 3;

        // Sign extend the value from 13 bits to 16 bits
        if(((BYTE_VAL*)&RawTemp.byte.MSB)->bits.b4)
        {
            RawTemp.byte.MSB |= 0xE0;
        }

        // Convert the TC77 integer to a scaled one with units 0.0001 degrees C
        ScaledTemp = (long)RawTemp.Val * (long)625;

        // Convert the integer to a string and find its length
        pTempString = ltoa( ScaledTemp, tempString );
        strLen = strlen( tempString );

        // Write the negative sign if needed. We are going to
        // format small negative numbers a little differently.
        ptr = buffer;
        if(tempString[0] == '-')
        {
            *ptr++ = '-';
            strLen--;
            pTempString++;
        }

        // Write "0." and then the number if this number is too close to zero; eg: 0, 0.0625, -0.125
        if(strLen < 5)
        {
            *ptr++ = '0';
            *ptr++ = '.';
            memcpy( (void *)ptr, (void *)tempString, strLen );
            *ptr += strLen;
        }
        // Otherwise, write the full number and write
        // the decimal place in digit 5's position;
        // eg: 24.1250
        else
        {
            for(;strLen >= 5; strLen--)
                *ptr++ = *pTempString++;
            *ptr++ = '.';
            for(;strLen > 0; strLen--)
                *ptr++ = *pTempString++;
        }

        // Write °C"
        *ptr++ = ' ';
        *ptr++ = 'C';
        *ptr = '\0';
    }

    return strlen( buffer );
}

