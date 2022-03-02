/*********************************************************************
 *
 *                  PHY Header File for the Microchip MRF24J40
 *
 *********************************************************************
 * FileName:        zPHY_MRF24J40.h
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
 * DF/KO                08/31/06 Microchip ZigBee Stack v1.0-3.6
 ********************************************************************/

#if !defined(_ZPHYMRF24J40_H_)
#define _ZPHYMRF24J40_H_

#if !defined(PHY_CSn)
    #if defined(PHY_CS)
        #define PHY_CSn PHY_CS
        #define PHY_CSn_TRIS PHY_CS_TRIS
    #elif defined (PHY_SEN)
        #define PHY_CSn PHY_SEN
        #define PHY_CSn_TRIS PHY_SEN_TRIS
    #else
        #error Transceiver chip select line is not defined.
    #endif
#endif

BYTE PHYGetLongRAMAddr(WORD address);
BYTE PHYGetShortRAMAddr(BYTE address);
void PHYSetShortRAMAddr(BYTE address, BYTE value);
void PHYSetLongRAMAddr(WORD address, BYTE value);

//MRF24J40 short address registers
#define RXMCR (0x00)
#define PANIDL (0x01)
#define PANIDH (0x02)
#define SADRL (0x03)
#define SADRH (0x04)
#define EADR0 (0x05)
#define EADR1 (0x06)
#define EADR2 (0x07)
#define EADR3 (0x08)
#define EADR4 (0x09)
#define EADR5 (0x0a)
#define EADR6 (0x0b)
#define EADR7 (0x0c)
#define RXFLUSH (0x0d)
#define TXSTATE0 (0x0e)
#define TXSTATE1 (0x0f)
#define ORDER (0x10)
#define TXMCR (0x11)
#define ACKTMOUT (0x12)
#define SLALLOC (0x13)
#define SYMTICKL (0x14)
#define SYMTICKH (0x15)
#define PAONTIME (0x16)
#define PAONSETUP (0x17)
#define FFOEN (0x18)
#define CSMACR (0x19)
#define TXBCNTRIG (0x1a)
#define TXNMTRIG (0x1b)
#define TXG1TRIG (0x1c)
#define TXG2TRIG (0x1d)
#define ESLOTG23 (0x1e)
#define ESLOTG45 (0x1f)
#define ESLOTG67 (0x20)
#define TXPEND (0x21)
#define TXBCNINTL (0x22)
#define FRMOFFSET (0x23)
#define TXSR (0x24)
#define TXLERR (0x25)
#define GATE_CLK (0x26)
#define TXOFFSET (0x27)
#define HSYMTMR0 (0x28)
#define HSYMTMR1 (0x29)
#define SOFTRST (0x2a)
#define BISTCR (0x2b)
#define SECCR0 (0x2c)
#define SECCR1 (0x2d)
#define TXPEMISP (0x2e)
#define SECISR (0x2f)
#define RXSR (0x30)
#define ISRSTS (0x31)
#define INTMSK (0x32)
#define GPIO (0x33)
#define GPIODIR (0x34)
#define SLPACK (0x35)
#define PWRCTL (0x36)
#define SECCR2 (0x37)
//#define  (0x38)
#define BBREG1 (0x39)
#define BBREG2 (0x3a)
#define BBREG3 (0x3b)
#define BBREG4 (0x3c)
#define BBREG5 (0x3d)
#define BBREG6 (0x3e)
#define RSSITHCCA (0x3f)

//MRF24J40 long address registers
#define RFCTRL0 (0x200)
#define RFCTRL1 (0x201)
#define RFCTRL2 (0x202)
#define RFCTRL3 (0x203)
#define RFCTRL4 (0x204)
#define RSSI (0x205)
#define RFCTRL6 (0x206)
#define RFCTRL7 (0x207)
#define RFCTRL8 (0x208)
#define CAL1 (0x209)
#define CAL2 (0x20a)
#define CAL3 (0x20b)
#define SFCNTRH (0x20c)
#define SFCNTRM (0x20d)
#define SFCNTRL (0x20e)
#define RFSTATE (0x20f)
#define BATTERYTH (0x210)
#define CLKIRQCR (0x211)
#define SRCADRMODE (0x212)
#define SRCADDR0 (0x213)
#define SRCADDR1 (0x214)
#define SRCADDR2 (0x215)
#define SRCADDR3 (0x216)
#define SRCADDR4 (0x217)
#define SRCADDR5 (0x218)
#define SRCADDR6 (0x219)
#define SRCADDR7 (0x21a)
#define RXFRAMESTATE (0x21b)
#define SECSTATUS (0x21c)
#define STCCMP (0x21d)
#define HLEN (0x21e)
#define FLEN (0x21f)
#define SCLKDIV (0x220)
//#define reserved (0x221)
#define WAKETIMEL (0x222)
#define WAKETIMEH (0x223)
#define TXREMCNTL (0x224)
#define TXREMCNTH (0x225)
#define TXMAINCNTL (0x226)
#define TXMAINCNTM (0x227)
#define TXMAINCNTH0 (0x228)
#define TXMAINCNTH1 (0x229)
#define RFMANUALCTRLEN (0x22a)
#define RFMANUALCTRL (0x22b)
#define RFRXCTRL RFMANUALCTRL
#define TxDACMANUALCTRL (0x22c)
#define RFMANUALCTRL2 (0x22d)
#define TESTRSSI (0x22e)
#define TESTMODE (0x22f)

#define NORMAL_TX_FIFO  (0x000)
#define BEACON_TX_FIFO  (0x080)
#define GTS1_TX_FIFO    (0x100)
#define GTS2_TX_FIFO    (0x180)

#define RX_FIFO         (0x300)

#define SECURITY_FIFO   (0x280)

#endif
