/*****************************************************************************
 *
 *              Simple SRAM Dynamic Memory Allocation
 *
 *****************************************************************************
 * FileName:        sralloc.c
 * Dependencies:
 * Processor:       PIC18F with CAN
 * Compiler:        C18 02.20.00 or higher
 * Linker:          MPLINK 03.40.00 or higher
 * Company:         Microchip Technology Incorporated
 *
 * Software License Agreement
 *
 * The software supplied herewith by Microchip Technology Incorporated
 * (the "Company") is intended and supplied to you, the Company's
 * customer, for use solely and exclusively with products manufactured
 * by the Company.
 *
 * The software is owned by the Company and/or its supplier, and is
 * protected under applicable copyright laws. All rights are reserved.
 * Any use in violation of the foregoing restrictions may subject the
 * user to criminal sanctions under applicable laws, as well as to
 * civil liability for the breach of the terms and conditions of this
 * license.
 *
 * THIS SOFTWARE IS PROVIDED IN AN "AS IS" CONDITION. NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 * TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 *
 * This is a simple dynamic memory allocation module. The following are the
 * supported services:
 *
 * unsigned char * NEAR SRAMalloc(NEAR unsigned char nBytes)
 * void SRAMfree(unsigned char * NEAR pSRAM)
 * void SRAMInitHeap(void)
 *
 * This version of the dynamic memory allocation limits the segment size
 * to 126 bytes. This is specifically designed such to enable better
 * performance by limiting pointer manipulation.
 *
 *
 * How it works:
 * The model is based on a simple form of a linked list. A block of memory
 * refered to as the dynamic heap is split into segments. Each segment
 * has a single byte header that references the next segment in the list
 * as well as indicating whether the segment is allocated. Consiquently
 * the reference implicitly identifies the length of the segment.
 *
 * This method also enables the possibility of allowing a large number
 * of memory allocations. The maximum is limited by the defined heap size.
 *
 * SRAMalloc() is used to split or merge segments to be allocated.
 * SRAMfree() is used to release segments.
 *
 * Example:
 *  ----------
 *  |  0x7F  |  0x200   Header Seg1
 *  |        |
 *  |        |
 *  |        |
 *  |        |
 *  |        |
 *  |        |
 *  |  0x89  |  0x27F   Header Seg2 (allocated)
 *  |        |
 *  |        |
 *  |  0x77  |  0x288   Header Seg3
 *  |        |
 *  |        |
 *  |        |
 *  |        |
 *  |        |
 *  |        |
 *  |        |
 *  |  0x00  |  0x2FF   Tail
 *  ----------
 *
 *
 *  Bit 7   Bit 6   Bit 5   Bit 4   Bit 3   Bit 2   Bit 1   Bit 0
 *
 *  Alloc   ------------- reference to next Header --------------
 *
 *
 * Recomendations:
 * Although this model will allow dynamic allocation down to a single byte,
 * doing so sacrifices performance. With more segments within the heap, more
 * time is required to attempt to allocate memory. Plus every segment requires
 * a header byte; therefore, smaller segments require more memory. There is
 * also the possibility of fragmentation, which could ultimately doom an
 * application by reducing the largest allocatable block of memory. Thus the
 * recomendation is to allocate at least 8 bytes of memory.
 *
 *
 *
 * Author               Date        Version     Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Ross Fosler          05/25/03    v1.03       ... First release
 *
 * Nilesh Rajbharti     11/1/04 Pre-release version
 * DF/KO                04/29/05 Microchip ZigBee Stack v1.0-2.0
 * DF/KO                07/18/05 Microchip ZigBee Stack v1.0-3.0
 * DF/KO                07/27/05 Microchip ZigBee Stack v1.0-3.1
 * DF/KO                08/19/05 Microchip ZigBee Stack v1.0-3.2
 * DF/KO                01/09/06 Microchip ZigBee Stack v1.0-3.5
 * DF/KO                08/31/06 Microchip ZigBee Stack v1.0-3.6
 *****************************************************************************/


#ifndef NULL
#define NULL    0
#endif


/*********************************************************************
 * Function:        unsigned char * SRAMalloc(unsigned char length)
 *
 * PreCondition:    A memory block must be allocated in the linker,
 *                  and the memory headers and tail must already be
 *                  set via the function SRAMInitHeap().
 *
 * Input:           unsigned char nBytes - Number of bytes to allocate.
 *
 * Output:          unsigned char * - A pointer to the requested block
 *                  of memory.
 *
 * Side Effects:
 *
 * Overview:        This functions allocates a chunk of memory from
 *                  the heap. The maximum segment size for this
 *                  version is 126 bytes. If the heap does not have
 *                  an available segment of sufficient size it will
 *                  attempt to create a segment; otherwise a NULL
 *                  pointer is returned. If allocation is succeessful
 *                  then a pointer to the requested block is returned.
 *
 * Note:            The calling function must maintain the pointer
 *                  to correctly free memory at runtime.
 ********************************************************************/
unsigned char * SRAMalloc(unsigned char nBytes);



/*********************************************************************
 * Function:        void SRAMfree(unsigned char * pSRAM)
 *
 * PreCondition:    The pointer must have been returned from a
 *                  previously allocation via SRAMalloc().
 *
 * Input:           unsigned char * pSRAM - pointer to the allocated
 *
 * Output:          void
 *
 * Side Effects:
 *
 * Overview:        This function de-allocates a previously allocated
 *                  segment of memory.
 *
 * Note:            The pointer must be a valid pointer returned from
 *                  SRAMalloc(); otherwise, the segment may not be
 *                  successfully de-allocated, and the heap may be
 *                  corrupted.
 ********************************************************************/
void SRAMfree(unsigned char * pSRAM);

#define free(p) {                                   \
                    SRAMfree( (unsigned char *)p ); \
                    p = NULL;                       \
                }

/*********************************************************************
 * Function:        void SRAMInitHeap(void)
 *
 * PreCondition:
 *
 * Input:           void
 *
 * Output:          void
 *
 * Side Effects:
 *
 * Overview:        This function initializes the dynamic heap. It
 *                  inserts segment headers to maximize segment space.
 *
 * Note:            This function must be called at least one time.
 *                  And it could be called more times to reset the
 *                  heap.
 ********************************************************************/
void SRAMInitHeap(void);
