/*
 * This software is made available under the Creative Commons BY-SA license.
 *  Any copies of this software may be freely distributed as long as proper attribution is made,
 *   this license retained, and any copies made available under this license.
 */



#ifndef __MMC_H
#define __MMC_H

// debugging info
//#define __UART_DEBUG


#include "inc/hw_memmap.h"
#include "inc/hw_ssi.h"
#include "inc/hw_types.h"
#include "driverlib/ssi.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#ifdef __UART_DEBUG
#include "utils/uartstdio.h"
#endif


// UART peripheral details (TODO - extend/complete)
#define UART_BASE GPIO_PORTA_BASE
#define UART_PINS (GPIO_PIN_0 | GPIO_PIN_1)

// SPI ("SSI") peripheral details
#define SPI_SYSCTL SYSCTL_PERIPH_SSI0
#define SPI_SYSCTL_PORT SYSCTL_PERIPH_GPIOA
#define SPI_BASE SSI0_BASE
#define SSI_GPIO_BASE GPIO_PORTA_BASE
#define SSI_TX GPIO_PIN_5
#define SSI_RX GPIO_PIN_4
#define SSI_CLK GPIO_PIN_2

// CS for the SD card
#define CS_BASE GPIO_PORTA_BASE
//#define CS_PIN GPIO_PIN_3
#define CS_PIN GPIO_PIN_7

// status LEDs
#define LED_BASE GPIO_PORTF_BASE
#define LED_SYSCTL SYSCTL_PERIPH_GPIOF
#define ERROR_LED GPIO_PIN_1
#define BUSY_LED GPIO_PIN_2
#define SUCCESS_LED GPIO_PIN_3

// SPI speed (in Hz)
#define SPI_SPEED 250000
//#define SPI_SPEED 8000000

// MMC command defines
// Borrowed from elm-chan's examples
#define CMD0	(0)			/* GO_IDLE_STATE */
#define CMD1	(1)			/* SEND_OP_COND (MMC) */
#define	ACMD41	(41)	/* SEND_OP_COND (SDC) */ //(0x80+41)
#define CMD8	(8)			/* SEND_IF_COND */
#define CMD9	(9)			/* SEND_CSD */
#define CMD10	(10)		/* SEND_CID */
#define CMD12	(12)		/* STOP_TRANSMISSION */
#define ACMD13	(0x80+13)	/* SD_STATUS (SDC) */
#define CMD16	(16)		/* SET_BLOCKLEN */
#define CMD17	(17)		/* READ_SINGLE_BLOCK */
#define CMD18	(18)		/* READ_MULTIPLE_BLOCK */
#define CMD23	(23)		/* SET_BLOCK_COUNT (MMC) */
#define	ACMD23	(0x80+23)	/* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24	(24)		/* WRITE_BLOCK */
#define CMD25	(25)		/* WRITE_MULTIPLE_BLOCK */
#define CMD55	(55)		/* APP_CMD */
#define CMD58	(58)		/* READ_OCR */

// Function declarations

// Pushes out one byte over SPI and reads in the buffers
unsigned char tradeByte(unsigned char b);

// Sends a formatted command to the SD card
// Has precalculated CRC for CMD8 and CMD0, otherwise sends 0xFF
// arg is a 32-bit argument to the 8-bit cmd 
unsigned char sendCommand(unsigned char cmd, unsigned int arg);

unsigned char sendCommand_m(unsigned char cmd, unsigned int arg);
// for now, only initializes only one card with the parameters in this header
// modifiying this to support multiple SD cards should be trivial
int initializeCard();

// sends a request for a single block to the SD card
// reads into buf - does not check array bounds (assumes 512B)
// addr should be a scaled address, and will be send directly to the SD
int readSingleBlock(unsigned char *buf, unsigned int addr);


// 
int writeSingleBlock(unsigned char *buf, unsigned int addr);


// just throws up the red LED and loops
void errorAndHand();

#endif // __MMC_H
