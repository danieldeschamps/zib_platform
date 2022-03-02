/*********************************************************************
 *
 *                  PHY Generic Header File
 *
 *********************************************************************
 * FileName:        zPHY.h
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

#ifndef _zPHY
#define _zPHY
#include "generic.h"

typedef struct _SAVED_BITS
{
    unsigned int bGIEH :1;
} SAVED_BITS;

typedef struct _PHY_ERROR
{
    BYTE failedToMallocRx :1;
} PHY_ERROR;

extern SAVED_BITS savedBits;

#define PHY_CSn_1()\
{\
    PHY_CSn = 1;\
    GIEH = savedBits.bGIEH;\
}

#define PHY_CSn_0()\
{\
    savedBits.bGIEH = GIEH;\
    GIEH = 0;\
    PHY_CSn = 0;\
}

#define iPHY_CSn_1()\
{\
    PHY_CSn = 1;\
}

#define iPHY_CSn_0()\
{\
    PHY_CSn = 0;\
}


ZIGBEE_PRIMITIVE PHYTasks(ZIGBEE_PRIMITIVE inputPrimitive);
void PHYInit(void);
void PHYSetOutputPower( BYTE power);

typedef enum _PHY_enums
{
phyBUSY =                  0x00,
phyBUSY_RX =               0x01,
phyBUSY_TX =               0x02,
phyFORCE_TRX_OFF =         0x03,
phyIDLE =                  0x04,
phyINVALID_PARAMETER =     0x05,
phyRX_ON =                 0x06,
phySUCCESS =               0x07,
phyTRX_OFF =               0x08,
phyTX_ON =                 0x09,
phyUNSUPPORTED_ATTRIBUTE = 0x0A
} PHY_enums;

typedef struct _PHY_PIB
{
    BYTE phyCurrentChannel;
    WORD phyChannelsSupported;
    union _phyTransmitPower
    {
        BYTE val;
        struct _phyTransmitPower_bits
        {
            unsigned int nominalTransmitPower :6;
            unsigned int tolerance :2;
        } bits;
    } phyTransmitPower;
    BYTE phyCCAMode;
} PHY_PIB;

typedef union _PHYPendingTasks
{
    struct
    {
        unsigned int PLME_SET_TRX_STATE_request_task :1;
        unsigned int PHY_RX :1;
    } bits;
    BYTE Val;
} PHY_PENDING_TASKS;
extern PHY_PIB phyPIB;

BYTE PHYHasBackgroundTasks(void);

#if RF_CHIP == CC2420
    #include "zPHY_CC2420.h"
#elif (RF_CHIP == MRF24J40) || (RF_CHIP == UZ2400)
    #include "zPHY_MRF24J40.h"

#endif


#endif
