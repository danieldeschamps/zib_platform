/*********************************************************************
 *
 *                  ZigBee non-volatile memory storage
 *
 *********************************************************************
 * FileName:        zNVM.c
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
 * Nilesh Rajbharti     7/12/04 Rel 0.9
 * Nilesh Rajbharti     11/1/04 Pre-release version
 * DF/KO                04/29/05 Microchip ZigBee Stack v1.0-2.0
 * DF/KO                07/18/05 Microchip ZigBee Stack v1.0-3.0
 * DF/KO                07/27/05 Microchip ZigBee Stack v1.0-3.1
 * DF/KO                08/19/05 Microchip ZigBee Stack v1.0-3.2
 * DF/KO                01/09/06 Microchip ZigBee Stack v1.0-3.5
 * DF/KO                08/31/06 Microchip ZigBee Stack v1.0-3.6
 *********************************************************************

 This file contains all the routines necessary to access data that
 should be stored in non-volatile memory.

 These routines store the neighbor and binding tables in flash program
 memory.  If these tables are moved to a different type of memory, only
 this file and zNVM.H need to be changed.

 *****************************************************************************/

#include <string.h>
#include "zigbee.h"
//#include "Debug.h"
#include "sralloc.h"
#include "zNVM.h"
#include "zNWK.h"
#include "zAPS.h"

//#include "console.h"

#if defined(WIN32)
    #include "physim.h"
#endif

//******************************************************************************
// Compilation options
//******************************************************************************

// Uncomment ENABLE_DEBUG line to enable debug mode for this file.
// Or you may also globally enable debug by defining this macro
// in zigbee.def file or from compiler command-line.
#ifndef ENABLE_DEBUG
//#define ENABLE_DEBUG
#endif

// The definitions for ERASE_BLOCK_SIZE and WRITE_BLOCK_SIZE have been moved
// to zigbee.def, since they can change based on the selected processor.

// To help reduce the number of times program memory is written, enable this
// option to see if the program memory already matches the values we are trying
// to write to it.
#define CHECK_BEFORE_WRITE

// Uncomment VERIFY_WRITE if you would like a verification performed on program
// memory writes.  Note that this option will also perform the CHECK_BEFORE_WRITE
// function.  Be aware that if there is a problem with the device where it is
// unable to successfully perform the program memory write, and infinite loop will
// occur.
//#define VERIFY_WRITE

//******************************************************************************
// RAM and ROM Variables
//******************************************************************************

#pragma romdata macaddress=0x00002A
// Our MAC address.
ROM LONG_ADDR macLongAddr = {MAC_LONG_ADDR_BYTE0, MAC_LONG_ADDR_BYTE1, MAC_LONG_ADDR_BYTE2, MAC_LONG_ADDR_BYTE3,
                             MAC_LONG_ADDR_BYTE4, MAC_LONG_ADDR_BYTE5, MAC_LONG_ADDR_BYTE6, MAC_LONG_ADDR_BYTE7};
#pragma romdata


// Since we are using non-volatile memory for storage with no hardcoded
// memory address, it is possible that these memory regions may be
// placed inbetween executable code. In that case when we update this
// memory, we would erase part of executable code.
// To avoid that there are two ERASE_BLOCK_SIZE filler block around the storage.
// This will make sure that we are well away from executable code.
ROM BYTE filler[ERASE_BLOCK_SIZE] = {0x00};     // Does not need initializing, but the HI-TECH compiler will not place it in ROM memory otherwise

// Binding table information.
#if defined(I_SUPPORT_BINDINGS)
    ROM WORD            bindingValidityKey = {0x0000};
    ROM BINDING_RECORD  apsBindingTable[MAX_BINDINGS] = {0x00};           // Does not need initializing, but the HI-TECH compiler will not place it in ROM memory otherwise
    ROM BYTE            bindingTableUsageMap[BINDING_USAGE_MAP_SIZE] = {0x00};      // Does not need initializing, but the HI-TECH compiler will not place it in ROM memory otherwise
    ROM BYTE            bindingTableSourceNodeMap[BINDING_USAGE_MAP_SIZE] = {0x00}; // Does not need initializing, but the HI-TECH compiler will not place it in ROM memory otherwise

    // RAM copy of current records.
    BINDING_RECORD      currentBindingRecord;

    // To remember current table entry pointer.
    ROM BINDING_RECORD  *pCurrentBindingRecord;
#endif

// Neighbor Table information
NEIGHBOR_RECORD         currentNeighborRecord;                      // Node information.
NEIGHBOR_TABLE_INFO     currentNeighborTableInfo;               // Info about the neighbor table and the node's children.
ROM NEIGHBOR_TABLE_INFO neighborTableInfo = {0x00};         // Initialize to something other than the valid key.
ROM NEIGHBOR_RECORD     neighborTable[MAX_NEIGHBORS] = {0x00};  // Does not need initializing, but the HI-TECH compiler will not place it in ROM memory otherwise
ROM NEIGHBOR_RECORD     *pCurrentNeighborRecord;

// Routing information
#if defined(I_SUPPORT_ROUTING) && !defined(USE_TREE_ROUTING_ONLY)
    ROM ROUTING_ENTRY   routingTable[ROUTING_TABLE_SIZE] = {0x00};   // Does not need initializing, but the HI-TECH compiler will not place it in ROM memory otherwise

    // RAM copy of the current routing entry
    ROUTING_ENTRY       currentRoutingEntry;

    // Pointer to the current routing entry in ROM
    ROM ROUTING_ENTRY   *pCurrentRoutingEntry;
#endif

// APS Address Map Table
#if MAX_APS_ADDRESSES > 0
    ROM WORD            apsAddressMapValidityKey = {0x0000};
    ROM APS_ADDRESS_MAP apsAddressMap[MAX_APS_ADDRESSES];
    APS_ADDRESS_MAP     currentAPSAddress;
#endif


ROM BYTE filler2[ERASE_BLOCK_SIZE] = {0x00};    // Does not need initializing, but the HI-TECH compiler will not place it in ROM memory otherwise

//******************************************************************************
// Macros
//******************************************************************************

#define NVMRead(dest, src, count)   memcpypgm2ram(dest, src, count)

/*********************************************************************
 * Function:        void NVMWrite(NVM_ADDR *dest, BYTE *src, BYTE count)
 *
 * PreCondition:    None
 *
 * Input:           *dest - pointer to the location in ROM where the first
 *                      source byte is to be stored
 *                  *src - pointer to the location in RAM of the first
 *                      byte to be written to ROM
 *                  count - the number of bytes to be written
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This routine writes the specified number of bytes
 *                  into program memory.
 *
 * Note:            Some devices have a limited number of write cycles.
 *                  When using these devices, ensure that the number
 *                  of write cycles over the lifetime of the application
 *                  will not exceed the specification.  It is also
 *                  recommended that the CHECK_BEFORE_WRITE option is
 *                  enabled to further reduce the number of write cycles.
 ********************************************************************/

void NVMWrite(NVM_ADDR *dest, BYTE *src, BYTE count)
{
    NVM_ADDR *pEraseBlock;
//    BYTE memBlock[ERASE_BLOCK_SIZE];
    BYTE    *memBlock;
    BYTE *pMemBlock;
    BYTE writeIndex;
    BYTE writeStart;
    BYTE writeCount;
    BYTE oldGIEH;
    SWORD oldTBLPTR;

#if defined(VERIFY_WRITE)
    while (memcmppgm2ram( src, (rom void *)dest, count ))
#elif defined(CHECK_BEFORE_WRITE)
    if (memcmppgm2ram( src, (rom void *)dest, count ))
#endif
    {
        if ((memBlock = SRAMalloc( ERASE_BLOCK_SIZE )) == NULL)
            return;

        #if 0
            TRACE("NVMWrite at " );
            TRACE_CHAR( (BYTE)(((WORD)dest>>8)&0xFF) );
            TRACE_CHAR( (BYTE)((WORD)dest&0xFF) );
            TRACE(" count " );
            TRACE_CHAR( count );
            TRACE("\r\n" );
        #endif

        //DEBUG_OUT("NVM: Writing a block of memory...\r\n");

        // First of all get nearest "left" erase block boundary
        pEraseBlock = (NVM_ADDR*)((long)dest & (long)(~(ERASE_BLOCK_SIZE-1)));
        writeStart = (BYTE)((BYTE)dest & (BYTE)(ERASE_BLOCK_SIZE-1));

        while( count )
        {
            // Now read the entire erase block size into RAM.
            NVMRead(memBlock, (ROM void*)pEraseBlock, ERASE_BLOCK_SIZE);

            // Erase the block.
            // Erase flash memory, enable write control.
            EECON1 = 0x94;

            oldGIEH = 0;
            if ( GIEH )
                oldGIEH = 1;
            GIEH = 0;

            #if defined(MCHP_C18)
                TBLPTR = (unsigned short long)pEraseBlock;
            #elif defined(HITECH_C18)
                TBLPTR = (void*)pEraseBlock;
            #endif

            CLRWDT();

            EECON2 = 0x55;
            EECON2 = 0xaa;
            EEWR = 1;
            NOP();

            EEWREN = 0;

            oldTBLPTR = TBLPTR;

            if ( oldGIEH )
                GIEH = 1;

            // Modify 64-byte block of RAM buffer as per what is required.
            pMemBlock = &memBlock[writeStart];
            while( writeStart < ERASE_BLOCK_SIZE && count )
            {
                *pMemBlock++ = *src++;

                count--;
                writeStart++;
            }

            // After first block write, next start would start from 0.
            writeStart = 0;

            // Now write entire 64 byte block in one write block at a time.
            writeIndex = ERASE_BLOCK_SIZE / WRITE_BLOCK_SIZE;
            pMemBlock = memBlock;
            while( writeIndex )
            {

                oldGIEH = 0;
                if ( GIEH )
                    oldGIEH = 1;
                GIEH = 0;

                TBLPTR = oldTBLPTR;

                // Load individual block
                writeCount = WRITE_BLOCK_SIZE;
                while( writeCount-- )
                {
                    TABLAT = *pMemBlock++;

                    TBLWTPOSTINC();
                }

                // Start the write process: reposition tblptr back into memory block that we want to write to.
                #if defined(MCHP_C18)
                    _asm tblrdpostdec _endasm
                #elif defined(HITECH_C18)
                    asm(" tblrd*-");
                #endif

                // Write flash memory, enable write control.
                EECON1 = 0x84;

                CLRWDT();

                EECON2 = 0x55;
                EECON2 = 0xaa;
                EEWR = 1;
                NOP();
                EEWREN = 0;

                // One less block to write
                writeIndex--;

                TBLPTR++;

                oldTBLPTR = TBLPTR;

                if ( oldGIEH )
                    GIEH = 1;
            }

            // Go back and do it all over again until we write all
            // data bytes - this time the next block.
    #if !defined(WIN32)
            pEraseBlock += ERASE_BLOCK_SIZE;
    #endif
        }

        SRAMfree( memBlock );
    }
}

/*********************************************************************
 * Function:        void ClearNVM( NVM_ADDR *dest, WORD count )
 *
 * PreCondition:    None
 *
 * Input:           *dest - pointer to the location in ROM where to
 *                      start clearing memory
 *                  count - the number of bytes to be cleared
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This routine clears the specified number of bytes
 *                  of program memory.
 *
 * Note:            Some devices have a limited number of write cycles.
 *                  When using these devices, ensure that the number
 *                  of write cycles over the lifetime of the application
 *                  will not exceed the specification.  It is also
 *                  recommended that the CHECK_BEFORE_WRITE option is
 *                  enabled to further reduce the number of write cycles.
 *
 *                  This function is not very efficient.  If this function
 *                  is called in time-critical code, it should be
 *                  reworked to clear the maximim number of bytes possible.
 ********************************************************************/

void ClearNVM( NVM_ADDR *dest, WORD count )
{
    WORD    i;
    BYTE    dummy = 0;

    for (i=0; i<count; i++)
    {
#if defined(VERIFY_WRITE)
        while (memcmppgm2ram( &dummy, (rom void *)dest, 1 ))
#elif defined(CHECK_BEFORE_WRITE)
        if (memcmppgm2ram( &dummy, (rom void *)dest, 1 ))
#endif
        {
            NVMWrite( dest, &dummy, 1 );
        }
        dest++;
        CLRWDT();
    }
}
