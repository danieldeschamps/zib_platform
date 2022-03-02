/*********************************************************************
 *
 *                  ZigBee APL Header File
 *
 *********************************************************************
 * FileName:        zAPL.h
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

#include "ZigbeeTasks.h"
#include "sralloc.h"

#include "MSPI.h"
#include "zNVM.h"
#include "zPHY.h"
#include "zMAC.h"
#include "zNWK.h"
#include "zAPS.h"
#include "zZDO.h"

#define APL_FRAME_COMMAND_MASK      0x0F
#define APL_FRAME_COMMAND_SET       0x01
#define APL_FRAME_COMMAND_EVENT     0x02
#define APL_FRAME_COMMAND_GETACK    0x04
#define APL_FRAME_COMMAND_SETACK    0x05
#define APL_FRAME_COMMAND_EVENTACK  0x06
#define APL_FRAME_COMMAND_GET_RES   0x08
#define APL_FRAME_COMMAND_SET_RES   0x09
#define APL_FRAME_COMMAND_EVENT_RES 0x0A

#define APL_FRAME_COUNT_MASK        0x0F

#define APL_FRAME_DATA_TYPE_MASK            0xF0
#define APL_FRAME_DATA_TYPE_NO_DATA         0x00  // Attribute has no associated data
#define APL_FRAME_DATA_TYPE_UINT8           0x01  // 8-bit unsigned integer
#define APL_FRAME_DATA_TYPE_INT8            0x02  // 8-bit signed integer
#define APL_FRAME_DATA_TYPE_UINT16          0x03  // 16-bit unsigned integer
#define APL_FRAME_DATA_TYPE_INT16           0x04  // 16-bit signed integer
#define APL_FRAME_DATA_TYPE_SEMI_PRECISE    0x0B  // 16-bit IEEE 754 floating point
#define APL_FRAME_DATA_TYPE_ABS_TIME        0x0C  // 32-bit unsigned absolute time since midnight 1st January 2000 in seconds
#define APL_FRAME_DATA_TYPE_REL_TIME        0x0D  // 32-bit unsigned relative time in milliseconds
#define APL_FRAME_DATA_TYPE_CSTRING         0x0E  // 1 byte length followed by array of characters encoded by a particular language/character set
#define APL_FRAME_DATA_TYPE_OSTRING         0x0F  // 1 byte length followed by an array of application defined octets

#define APL_FRAME_TYPE_MASK         0xF0
#define APL_FRAME_TYPE_KVP          0x10
#define APL_FRAME_TYPE_MSG          0x20



// ZigBee Application Framework KVP error codes
typedef enum _AF_ERROR_CODES
{
    KVP_SUCCESS                         = 0x00,
    KVP_INVALID_ENDPOINT                = 0x01,
    KVP_UNSUPPORTED_ATTRIBUTE           = 0x03,
    KVP_INVALID_COMMAND_TYPE            = 0x04,
    KVP_INVALID_ATTRIBUTE_DATA_LENGTH   = 0x05,
    KVP_INVALID_ATTRIBUTE_DATA          = 0x06
} AF_ERROR_CODES;





#define APLEnable()                 MACEnable()
#define APLDisable()                MACDisable()
#define APLDiscardRx()              APSDiscardRx()
#define APLGet()                    APSGet()
#define APLGetTransId()             aplTransId++

extern BYTE     aplTransId;


// ******************************************************************************
// Compatibility Definitions

#define TRANS_COMMAND_TYPE_MASK APL_FRAME_COMMAND_MASK
#define TRANS_SET               APL_FRAME_COMMAND_SET
#define TRANS_EVENT             APL_FRAME_COMMAND_EVENT
#define TRANS_GET_ACK           APL_FRAME_COMMAND_GETACK
#define TRANS_SET_ACK           APL_FRAME_COMMAND_SETACK
#define TRANS_EVENT_ACK         APL_FRAME_COMMAND_EVENTACK
#define TRANS_GET_RESP          APL_FRAME_COMMAND_GET_RES
#define TRANS_SET_RESP          APL_FRAME_COMMAND_SET_RES
#define TRANS_EVENT_RESP        APL_FRAME_COMMAND_EVENT_RES

#define TRANS_FRAME_TYPE_KVP    APL_FRAME_TYPE_KVP
#define TRANS_FRAME_TYPE_MSG    APL_FRAME_TYPE_MSG


#define TRANS_DATA_TYPE_MASK    APL_FRAME_DATA_TYPE_MASK

#define TRANS_NO_DATA           APL_FRAME_DATA_TYPE_NO_DATA
#define TRANS_UINT8             APL_FRAME_DATA_TYPE_UINT8
#define TRANS_INT8              APL_FRAME_DATA_TYPE_INT8
#define TRANS_UINT16            APL_FRAME_DATA_TYPE_UINT16
#define TRANS_INT16             APL_FRAME_DATA_TYPE_INT16
#define TRANS_SEMI_PRECISE      APL_FRAME_DATA_TYPE_SEMI_PRECISE
#define TRANS_ABS_TIME          APL_FRAME_DATA_TYPE_ABS_TIME
#define TRANS_REL_TIME          APL_FRAME_DATA_TYPE_REL_TIME
#define TRANS_ZSTRING           APL_FRAME_DATA_TYPE_CSTRING
#define TRANS_OSTRING           APL_FRAME_DATA_TYPE_OSTRING
