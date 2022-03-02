/*********************************************************************
 *
 *                  Zigbee non-volatile memory storage header
 *
 *********************************************************************
 * FileName:        zNVM.h
 * Dependencies:
 * Processor:       PIC18
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
 * Author               Date        Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Nilesh Rajbharti     10/15/04    Original Version
 * Nilesh Rajbharti     11/1/04     Pre-release version
 * DF/KO                04/29/05 Microchip ZigBee Stack v1.0-2.0
 * DF/KO                07/18/05 Microchip ZigBee Stack v1.0-3.0
 * DF/KO                07/27/05 Microchip ZigBee Stack v1.0-3.1
 * DF/KO                08/19/05 Microchip ZigBee Stack v1.0-3.2
 * DF/KO                01/09/06 Microchip ZigBee Stack v1.0-3.5
 * DF/KO                08/31/06 Microchip ZigBee Stack v1.0-3.6
 ********************************************************************/
#ifndef _ZNVM_H_
#define _ZNVM_H_

#include "zigbee.h"
#include "zNWK.h"
#include "zAPS.h"
#include "zZDO.h"
#include "compiler.h"
#include <string.h>         // for memcpy-type functions

/*******************************************************************************
Defines
*******************************************************************************/

#if defined(I_SUPPORT_BINDINGS)
    #if (MAX_BINDINGS % 8) == 0
        #define BINDING_USAGE_MAP_SIZE  (MAX_BINDINGS/8)
    #else
        #define BINDING_USAGE_MAP_SIZE  (MAX_BINDINGS/8 + 1)
    #endif
#endif

/*******************************************************************************
Typedefs
*******************************************************************************/

typedef ROM BYTE NVM_ADDR;

/*******************************************************************************
Variable Definitions
*******************************************************************************/

#if defined(I_SUPPORT_BINDINGS)
    extern ROM BINDING_RECORD       apsBindingTable[MAX_BINDINGS];
    extern ROM BYTE                 bindingTableUsageMap[BINDING_USAGE_MAP_SIZE];
    extern ROM BYTE                 bindingTableSourceNodeMap[BINDING_USAGE_MAP_SIZE];
    extern ROM WORD                 bindingValidityKey;
    extern ROM BINDING_RECORD       *pCurrentBindingRecord;
    extern BINDING_RECORD           currentBindingRecord;
#endif
extern ROM NODE_DESCRIPTOR          Config_Node_Descriptor;
extern ROM NODE_POWER_DESCRIPTOR    Config_Power_Descriptor;
extern ROM NODE_SIMPLE_DESCRIPTOR   Config_Simple_Descriptors[];
extern NEIGHBOR_RECORD              currentNeighborRecord;
extern NEIGHBOR_TABLE_INFO          currentNeighborTableInfo;
extern ROM LONG_ADDR                macLongAddr;
extern ROM NEIGHBOR_RECORD          neighborTable[MAX_NEIGHBORS];
extern ROM NEIGHBOR_TABLE_INFO      neighborTableInfo;
extern ROM NEIGHBOR_RECORD          *pCurrentNeighborRecord;
#if defined(I_SUPPORT_ROUTING) && !defined(USE_TREE_ROUTING_ONLY)
    extern ROM ROUTING_ENTRY        routingTable[ROUTING_TABLE_SIZE];
    extern ROUTING_ENTRY            currentRoutingEntry;
    extern ROM ROUTING_ENTRY        *pCurrentRoutingEntry;
#endif

#if MAX_APS_ADDRESSES > 0
    extern ROM WORD                 apsAddressMapValidityKey;
    extern ROM APS_ADDRESS_MAP      apsAddressMap[MAX_APS_ADDRESSES];
    extern APS_ADDRESS_MAP          currentAPSAddress;
#endif

/*******************************************************************************
General Purpose Definitions
*******************************************************************************/

#define GetMACAddress( x )              NVMRead( x, (ROM void*)&macLongAddr, sizeof(LONG_ADDR) )
#define GetMACAddressByte( y, x )       NVMRead( x, (ROM void*)((int)&macLongAddr + (int)y), 1 )
#define PutMACAddress( x )              NVMWrite((NVM_ADDR*)&macLongAddr, (BYTE*)x, sizeof(LONG_ADDR))

#define ProfileGetNodeDesc(p)           NVMRead(p, (ROM void*)&Config_Node_Descriptor, sizeof(NODE_DESCRIPTOR))
#define ProfileGetNodePowerDesc(p)      NVMRead(p, (ROM void*)&Config_Power_Descriptor, sizeof(NODE_POWER_DESCRIPTOR))
#define ProfileGetSimpleDesc(p,i)       NVMRead(p, (ROM void*)&(Config_Simple_Descriptors[i]), sizeof(NODE_SIMPLE_DESCRIPTOR))

//#define PutBTR( x,i )                   NVMWrite((ROM void*)&BTT[i],(BYTE*)x,sizeof(BTR))
//#define GetBTR(p,i)                     NVMRead(p, (ROM void*)&BTT[i], sizeof(BTR))
//#define SetBTR(x,i)                     PutBTR(x,i)

#define GetAPSAddress( x, y )           NVMRead( x, (ROM void*)y, sizeof(APS_ADDRESS_MAP) )
#define GetAPSAddressValidityKey( x )   NVMRead( x, (ROM void*)&apsAddressMapValidityKey, sizeof(WORD) )
#define PutAPSAddress( x, y )           NVMWrite( (NVM_ADDR*)x, (BYTE*)y, sizeof(APS_ADDRESS_MAP) )
#define PutAPSAddressValidityKey( x )   NVMWrite( (NVM_ADDR *)&apsAddressMapValidityKey,(BYTE *)x, sizeof(WORD) )

#define NeedNewBindingMapByte(b)        ((b&0x07) == 0)
#define BindingBitMapBit(t)             (1 << (t&0x07))

#define BindingIsUsed(y,t)              ((y & BindingBitMapBit(t)) != 0)
#define MarkBindingUsed(y,t)            y |= BindingBitMapBit(t);
#define MarkBindingUnused(y,t)          y &= ~BindingBitMapBit(t);

#define ClearBindingTable()             ClearNVM( (NVM_ADDR *)bindingTableUsageMap, BINDING_USAGE_MAP_SIZE )
//#define ClearBindingTable()           {ClearNVM( (NVM_ADDR *)bindingTableUsageMap, sizeof(bindingTableUsageMap) ); \
//                                      ClearNVM( (NVM_ADDR *)apsBindingTable, sizeof(apsBindingTable) );}
#define GetBindingRecord( x, y )        NVMRead( x, y, sizeof(BINDING_RECORD) )
#define GetBindingSourceMap(x, y)       NVMRead( x, (ROM void*)&bindingTableSourceNodeMap[y>>3], 1 )
#define GetBindingUsageMap(x, y)        NVMRead( x, (ROM void*)&bindingTableUsageMap[y>>3], 1 )
#define GetBindingValidityKey( x )      NVMRead( x, (ROM void*)&bindingValidityKey, sizeof(WORD) )
#define PutBindingRecord( x, y )        NVMWrite( x, y, sizeof(BINDING_RECORD) )
#define PutBindingSourceMap(x, y)       NVMWrite( (NVM_ADDR *)&bindingTableSourceNodeMap[y>>3], x, 1 )
#define PutBindingUsageMap(x, y)        NVMWrite( (NVM_ADDR *)&bindingTableUsageMap[y>>3], x, 1 )
#define PutBindingValidityKey( x )      NVMWrite( (NVM_ADDR *)&bindingValidityKey,(BYTE *)x, sizeof(WORD) )

#define GetNeighborRecord( x, y )       NVMRead( x, y, sizeof(NEIGHBOR_RECORD) )
#define GetNeighborTableInfo()          NVMRead(&currentNeighborTableInfo,              \
                                            (ROM void*)&neighborTableInfo,              \
                                             sizeof(NEIGHBOR_TABLE_INFO))
#define PutNeighborRecord( x, y )       NVMWrite( x, y, sizeof(NEIGHBOR_RECORD) )
#define PutNeighborTableInfo()          NVMWrite((NVM_ADDR*)&neighborTableInfo,         \
                                            (void*)&currentNeighborTableInfo,           \
                                            sizeof(NEIGHBOR_TABLE_INFO))

#define GetRoutingEntry( x, y )         NVMRead( x, (ROM void*)y, sizeof(ROUTING_ENTRY) )
#define PutRoutingEntry( x, y )         NVMWrite( (NVM_ADDR*)x, (BYTE*)y, sizeof(ROUTING_ENTRY) )


/*******************************************************************************
Private Prototypes

These definitions are required for the above macros to work.  They should not be
used directly; only the definitions above should be used.
*******************************************************************************/
void NVMWrite(NVM_ADDR *dest, BYTE *src, BYTE count);
#define NVMRead(dest, src, count)   memcpypgm2ram(dest, src, count)
void ClearNVM( NVM_ADDR *dest, WORD count );


#endif

