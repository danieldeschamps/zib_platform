/*********************************************************************
 *
 *                  Console Routines
 *
 *********************************************************************
 * FileName:        Console.h
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
 * Nilesh Rajbharti     7/12/04 Rel 0.9
 * Nilesh Rajbharti     11/1/04 Pre-release version
 * DF/KO                04/29/05 Microchip ZigBee Stack v1.0-2.0
 * DF/KO                07/18/05 Microchip ZigBee Stack v1.0-3.0
 * DF/KO                07/27/05 Microchip ZigBee Stack v1.0-3.1
 * DF/KO                08/19/05 Microchip ZigBee Stack v1.0-3.2
 * DF/KO                09/08/05 Microchip ZigBee Stack v1.0-3.3
 * DF/KO                01/09/06 Microchip ZigBee Stack v1.0-3.5
 * DF/KO                08/31/06 Microchip ZigBee Stack v1.0-3.6
 ********************************************************************/
#ifndef  _CONSOLE_H_
#define _CONSOLE_H_

#include "generic.h"
#include "Compiler.h"

#if defined(WIN32)
    extern void _DebugOut(BYTE c);
    extern void _DebugOutString(BYTE *s);
    extern void _DebugPutROMString(char *s);
    extern BYTE _DebugGetString(char *buffer, BYTE bufferLen);
    extern BOOL _DebugIsGetReady(void);
    extern BYTE _DebugGet();

    #define ConsoleInit()
    #define ConsolePut(c)             _DebugOut(c)
    #define ConsolePutString(s)       _DebugOutString(s)
    #define ConsolePutROMString(s)    _DebugOutString(s)
    #define ConsoleGetString(b, l)    _DebugGetString(b, l)
    #define ConsoleIsGetReady()       _DebugIsGetReady()
    #define ConsoleIsPutReady()       (1)
    #define ConsoleGet()              _DebugGet()
    #define TRACE(string) ConsolePutROMString( (ROM char*) string)

#else

#if 1   // Useful for disabling the console (saving power)
    void ConsoleInit(void);
    #define ConsoleIsPutReady()     (TRMT)
    void ConsolePut(BYTE c);
    void ConsolePutString(BYTE *s);
    void ConsolePutROMString(ROM char* str);

    #define ConsoleIsGetReady()     (RCIF)
    BYTE ConsoleGet(void);
    BYTE ConsoleGetString(char *buffer, BYTE bufferLen);
    #ifdef DEBUG_ACTIVE
    	#define TRACE(string) ConsolePutROMString( (ROM char*) string)
    	#define TRACE_INT(integer) PrintInt(integer)
    	#define TRACE_CHAR(byte) PrintChar(byte)
    #else
    	#define TRACE(string)
    	#define TRACE_INT(integer)
    	#define TRACE_CHAR(byte)
    #endif
#else
    #define ConsoleInit()
    #define ConsoleIsPutReady() 1
    #define ConsolePut(c)
    #define ConsolePutString(s)
    #define ConsolePutROMString(str)

    #define ConsoleIsGetReady() 1
    #define ConsoleGet()        'a'
    #define ConsoleGetString(buffer, bufferLen) 0
    #define TRACE(string)
#endif

#endif

//TODO: remove test code
//<test code>
void PrintChar(BYTE);
//</test code>
void PrintInt(int toPrint);

#endif

