//*****************************************************************************
//
// ili9341_240x320x262K.c - Display driver for the MULTI-INNO TECHNOLOGY
//                          MI0283QT-9 TFT display with an ILI9341 controller.
//							This code uses 8 bit parallel interface mode.
// Maciej Kucia July 2013
//
// 
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
// 
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
// 
//
//*****************************************************************************

//*****************************************************************************
//
//! \addtogroup display_api
//! @{
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/lm3s9b95.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom_map.h"
#include "driverlib/rom.h"
#include "utils/uartstdio.h"
#include "grlib/grlib.h"
#include "ili9341_240x320x262K.h"

//*****************************************************************************
//
// Defines the clocks per milisecond macro
//
//*****************************************************************************
#define CPmS ROM_SysCtlClockGet()/1000

//*****************************************************************************
//
// Defines the data bus PORT write/read macros
//
//*****************************************************************************
#define TFT_HW_DTA_BASE 	GPIO_PORTJ_AHB_BASE
#define TFT_HW_WRITE(a)		HWREG(TFT_HW_DTA_BASE+GPIO_O_DATA+(0xFF<<2))=a;
#define TFT_HW_READ()		HWREG(TFT_HW_DTA_BASE+GPIO_O_DATA+(0xFF<<2))
#define TFT_HW_DTA_IN() 	ROM_GPIOPinTypeGPIOInput(TFT_HW_DTA_BASE, 0xFF);
#define TFT_HW_DTA_OUT() 	ROM_GPIOPinTypeGPIOOutput(TFT_HW_DTA_BASE, 0xFF);

//*****************************************************************************
//
// Defines the read signal pin write macros
//
//*****************************************************************************
#define TFT_HW_RD_BASE 		GPIO_PORTC_AHB_BASE
#define TFT_HW_RD_PIN		GPIO_PIN_5
#define TFT_HW_RD_HI()		HWREG(TFT_HW_RD_BASE+GPIO_O_DATA+(TFT_HW_RD_PIN<<2))=TFT_HW_RD_PIN;
#define TFT_HW_RD_LO()		HWREG(TFT_HW_RD_BASE+GPIO_O_DATA+(TFT_HW_RD_PIN<<2))=0;

//*****************************************************************************
//
// Defines the write signal pin write macros
//
//*****************************************************************************
#define TFT_HW_WR_BASE 		GPIO_PORTH_AHB_BASE
#define TFT_HW_WR_PIN 		GPIO_PIN_4
#define TFT_HW_WR_HI()		HWREG(TFT_HW_WR_BASE+GPIO_O_DATA+(TFT_HW_WR_PIN<<2))=TFT_HW_WR_PIN;
#define TFT_HW_WR_LO()		HWREG(TFT_HW_WR_BASE+GPIO_O_DATA+(TFT_HW_WR_PIN<<2))=0;

//*****************************************************************************
//
// Defines the register select signal pin write macros
//
//*****************************************************************************
#define TFT_HW_RS_BASE 		GPIO_PORTG_AHB_BASE
#define TFT_HW_RS_PIN 		GPIO_PIN_7
#define TFT_HW_RS_HI()		HWREG(TFT_HW_RS_BASE+GPIO_O_DATA+(TFT_HW_RS_PIN<<2))=TFT_HW_RS_PIN;
#define TFT_HW_RS_LO()		HWREG(TFT_HW_RS_BASE+GPIO_O_DATA+(TFT_HW_RS_PIN<<2))=0;

//*****************************************************************************
//
// Defines the chip select signal pin write macros
//
//*****************************************************************************
#define TFT_HW_CS_BASE 		GPIO_PORTH_AHB_BASE
#define TFT_HW_CS_PIN 		GPIO_PIN_6
#define TFT_HW_CS_HI()		HWREG(TFT_HW_CS_BASE+GPIO_O_DATA+(TFT_HW_CS_PIN<<2))=TFT_HW_CS_PIN;
#define TFT_HW_CS_LO()		HWREG(TFT_HW_CS_BASE+GPIO_O_DATA+(TFT_HW_CS_PIN<<2))=0;

//*****************************************************************************
//
// Defines the reset signal pin write macros
//
//*****************************************************************************
#define TFT_HW_RST_BASE 	GPIO_PORTH_AHB_BASE
#define TFT_HW_RST_PIN 		GPIO_PIN_5
#define TFT_HW_RST_HI()		HWREG(TFT_HW_RST_BASE+GPIO_O_DATA+(TFT_HW_RST_PIN<<2))=TFT_HW_RST_PIN;
#define TFT_HW_RST_LO()		HWREG(TFT_HW_RST_BASE+GPIO_O_DATA+(TFT_HW_RST_PIN<<2))=0;

//*****************************************************************************
//
// Translates a 24-bit RGB color to a display driver-specific color.
//
// \param c is the 24-bit RGB color.  The least-significant byte is the blue
// channel, the next byte is the green channel, and the third byte is the red
// channel.
//
// This macro translates a 24-bit RGB color into a value that can be written
// into the display's frame buffer in order to reproduce that color, or the
// closest possible approximation of that color.
//
// \return Returns the display-driver specific color.
//
// 24-bit format: XXXX XXXX RRRR RRRR GGGG GGGG BBBB BBBB
// 16-bit format: ---- ---- ---- ---- RRRR RGGG GGGB BBBB
//  8-bit format: ---- ---- ---- ---- ---- ---- RRRG GGBB
//
//
//*****************************************************************************
#define DPYCOLORTRANSLATE16(c)  ((((c) & 0x00f80000) >> 8) |                  \
                                 (((c) & 0x0000fc00) >> 5) |                  \
                                 (((c) & 0x000000f8) >> 3))
#define DPYCOLORTRANSLATE8(c)   ((((c) & 0x00e00000) >> 16) |                 \
                                 (((c) & 0x0000e000) >> 11) |                 \
                                 (((c) & 0x000000c0) >> 6))
#define DPYCOLORTRANSLATE DPYCOLORTRANSLATE16

//*****************************************************************************
//
//! Sends data to controller
//
//! \param data is a byte to be clocked into controller.
//!
//! This function put data on bus and clock it through write signal
//! This function does not select or deselect chip
//!
//! \return None.
//
//*****************************************************************************
inline void TFT_HW_Write(char data)
{
	TFT_HW_WRITE(data);
	TFT_HW_WR_LO();
	TFT_HW_WR_HI();
}

//*****************************************************************************
//
//! Sends command to controller
//
//! \param cmd is a byte to be clocked into controller as a command.
//!
//! Function selects ILI9341 controller and sends command
//!
//! \return None.
//
//*****************************************************************************
void TFT_HW_Command(unsigned char cmd)
{
	TFT_HW_CS_LO(); 	// Select chip
	TFT_HW_RS_LO(); 	// Go to command mode
	TFT_HW_WRITE(cmd);  // Put data on bus
	TFT_HW_WR_LO();		// Clock data
	TFT_HW_WR_HI();		// ^
	TFT_HW_RS_HI();		// switch back to data mode
}

//*****************************************************************************
//
//! Read data from controller
//
//!
//! Function selects ILI9341 controller and reads data from it
//!
//! \return byte-sized data from controller.
//
//*****************************************************************************
unsigned char TFT_HW_Read(void)
{
	unsigned char ret;

	TFT_HW_DTA_IN();	// Bus port as input
	TFT_HW_RD_LO();		// Clock data
	TFT_HW_RD_HI();		// ^
	ret = TFT_HW_READ();// Read value from bus
	TFT_HW_DTA_OUT();	// Bus port as output
	return ret;			// Return value
}

//*****************************************************************************
//
//! Set LCD writing range
//
//!
//! Function selects ILI9341 controller and writes range to it
//!
//!  \param x1 is a word of left boundary
//!  \param x2 is a word of right boundary
//!  \param y1 is a word of top boundary
//!  \param y2 is a word of bottom boundary
//
//*****************************************************************************
inline void TFT_HW_SetRect(uint16_t x1, uint16_t x2, uint16_t y1, uint16_t y2)
{
	TFT_HW_Command(ILI9341_CMD_COLUMN_ADDRESS_SET);

	TFT_HW_Write((x1>>8)&0xFF);
	TFT_HW_Write(x1&0xFF);

	TFT_HW_Write((x2>>8)&0xFF);
	TFT_HW_Write(x2&0xFF);

	TFT_HW_CS_HI();

	TFT_HW_Command(ILI9341_CMD_PAGE_ADDRESS_SET);

	TFT_HW_Write((y1>>8)&0xFF);
	TFT_HW_Write(y1&0xFF);

	TFT_HW_Write((y2>>8)&0xFF);
	TFT_HW_Write(y2&0xFF);

	TFT_HW_CS_HI();
}

//*****************************************************************************
//
//! Draws a pixel on the screen.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param i32X is the X coordinate of the pixel.
//! \param i32Y is the Y coordinate of the pixel.
//! \param ui32Value is the color of the pixel.
//!
//! This function sets the given pixel to a particular color.  The coordinates
//! of the pixel are assumed to be within the extents of the display.
//!
//! \return None.
//
//*****************************************************************************
static void TFT_HW_PixelDraw(void *pvDisplayData, long i32X,
		long i32Y, unsigned long ui32Value)
{
	ui32Value = DPYCOLORTRANSLATE(ui32Value);

	TFT_HW_SetRect(i32X,i32X,i32Y,i32Y);
	TFT_HW_Command(ILI9341_CMD_MEMORY_WRITE);
	TFT_HW_Write((ui32Value>>8 )&0xFF);
	TFT_HW_Write((ui32Value>>0 )&0xFF);
	TFT_HW_CS_HI();
}

//*****************************************************************************
//
//! Draws a horizontal sequence of pixels on the screen.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param i32X is the X coordinate of the first pixel.
//! \param i32Y is the Y coordinate of the first pixel.
//! \param i32X0 is sub-pixel offset within the pixel data, which is valid for 1
//! or 4 bit per pixel formats.
//! \param i32Count is the number of pixels to draw.
//! \param i32BPP is the number of bits per pixel; must be 1, 4, or 8 optionally
//! ORed with various flags unused by this driver.
//! \param pui8Data is a pointer to the pixel data.  For 1 and 4 bit per pixel
//! formats, the most significant bit(s) represent the left-most pixel.
//! \param pui8Palette is a pointer to the palette used to draw the pixels.
//!
//! This function draws a horizontal sequence of pixels on the screen, using
//! the supplied palette.  For 1 bit per pixel format, the palette contains
//! pre-translated colors; for 4 and 8 bit per pixel formats, the palette
//! contains 24-bit RGB values that must be translated before being written to
//! the display.
//!
//! \return None.
//
//*****************************************************************************
static void TFT_HW_PixelDrawMultiple(void *pvDisplayData, long i32X,
		long i32Y, long i32X0, long i32Count, long i32BPP,
		const unsigned char *pui8Data, const unsigned char *pui8Palette)
{
	unsigned long ulPixel = 0;
	unsigned long ulColor = 0;

	TFT_HW_SetRect(i32X,320,i32Y,240);
	TFT_HW_Command(ILI9341_CMD_MEMORY_WRITE);

	if (i32BPP&1)
	{
		// 1 bit per pixel in pucData
		// lX0 is the index of the bit processed within a byte
		// pucPalette holds the pre-translated 32bit display color
		while (i32Count)
		{
			ulPixel = *pui8Data++;

			while (i32Count && i32X0 < 8)	// while there are pixels in this byte
			{
				ulColor = ((unsigned long *) pui8Palette)[ulPixel & 1];// retrieve already translated color
				ulColor = DPYCOLORTRANSLATE(ulColor);
				TFT_HW_Write((ulColor>>8 )&0xFF);
				TFT_HW_Write((ulColor>>0 )&0xFF);

				i32Count--;		// processed another pixel
				i32X0++;		// done with this bit
				ulPixel >>= 1;	// prepare next bit
			}

			i32X0 = 0;	// process next byte, reset bit counter
		}
	}
	else if(i32BPP&4)
	{
		// 4 bits per pixel in pucData
		// lX0 holds 0/1 to indicate 4-bit nibble within byte
		// pucPalette holds untranslated 24 bit color
		while (i32Count)
		{
			if (i32X0 == 0)	// read first nibble
			{
				ulPixel = *pui8Data >> 4;
				i32X0 = 1;	// set index to second nibble
			}
			else
			{				// read second nibble
				ulPixel = *pui8Data & 0x0f;
				pui8Data++;// increase byte pointer as we're done reading this byte
				i32X0 = 0;	// set index to first nibble
			}

			ulColor = *(unsigned long *) (pui8Palette + (ulPixel * 3)) & 0x00ffffff;	// retrieve 24 bit color

			ulColor = DPYCOLORTRANSLATE(ulColor);
			TFT_HW_Write((ulColor>>8 )&0xFF);
			TFT_HW_Write((ulColor>>0 )&0xFF);

			i32Count--;	// processed another pixel
		}
	}
	else if(i32BPP&8)
	{
		// 8 bits per pixel in pucData
		// pucPalette holds untranslated 24 bit color
		while (i32Count)
		{
			ulPixel = *pui8Data & 0xFF;
			pui8Data++;	// increase byte pointer as we're done reading this byte
			ulColor = *(unsigned long *) (pui8Palette + (ulPixel *3)) & 0x00ffffff;	// retrieve 24 bit color

			ulColor = DPYCOLORTRANSLATE(ulColor);
			TFT_HW_Write((ulColor>>8 )&0xFF);
			TFT_HW_Write((ulColor>>0 )&0xFF);

			i32Count--;	// processed another pixel
		}
	}
	else if (i32BPP&16)
	{
		// 8 bits per pixel in pucData
			// pucPalette holds untranslated 24 bit color
			while (i32Count)
			{
				ulPixel = *pui8Data & 0xFF;
				pui8Data++;	// increase byte pointer as we're done reading this byte
				ulPixel |= (*pui8Data & 0xFF)<<8;
				pui8Data++;	// increase byte pointer as we're done reading this byte

				ulColor = *(unsigned long *) (pui8Palette + (ulPixel)) & 0x00ffffff;	// retrieve 24 bit color

				ulColor = DPYCOLORTRANSLATE(ulColor);
				TFT_HW_Write((ulColor>>8 )&0xFF);
				TFT_HW_Write((ulColor>>0 )&0xFF);

				i32Count--;	// processed another pixel
			}
	}

	TFT_HW_CS_HI();
}

//*****************************************************************************
//
//! Draws a horizontal line.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param i32X1 is the X coordinate of the start of the line.
//! \param i32X2 is the X coordinate of the end of the line.
//! \param i32Y is the Y coordinate of the line.
//! \param ui32Value is the color of the line.
//!
//! This function draws a horizontal line on the display.  The coordinates of
//! the line are assumed to be within the extents of the display.
//!
//! \return None.
//
//*****************************************************************************
static void TFT_HW_LineDrawH(void *pvDisplayData, long i32X1,
		long i32X2, long i32Y, unsigned long ui32Value)
{
	unsigned int uY;

	ui32Value = DPYCOLORTRANSLATE(ui32Value);

	TFT_HW_SetRect(i32X1,i32X2,i32Y,i32Y);

	TFT_HW_Command(ILI9341_CMD_MEMORY_WRITE);
	for (uY = 0; uY <= i32X2-i32X1; ++uY)
	{
		TFT_HW_Write((ui32Value>>8 )&0xFF);
		TFT_HW_Write((ui32Value>>0 )&0xFF);
	}
	TFT_HW_CS_HI();
}

//*****************************************************************************
//
//! Draws a vertical line.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param i32X is the X coordinate of the line.
//! \param i32Y1 is the Y coordinate of the start of the line.
//! \param i32Y2 is the Y coordinate of the end of the line.
//! \param ui32Value is the color of the line.
//!
//! This function draws a vertical line on the display.  The coordinates of the
//! line are assumed to be within the extents of the display.
//!
//! \return None.
//
//*****************************************************************************
static void TFT_HW_LineDrawV(void *pvDisplayData, long i32X,
		long i32Y1, long i32Y2, unsigned long ui32Value)
{
	unsigned int uY;

	ui32Value = DPYCOLORTRANSLATE(ui32Value);

	TFT_HW_SetRect(i32X,i32X,i32Y1,i32Y2);

	TFT_HW_Command(ILI9341_CMD_MEMORY_WRITE);
	for (uY = 0; uY <= i32Y2-i32Y1; ++uY)
	{
		TFT_HW_Write((ui32Value>>8 )&0xFF);
		TFT_HW_Write((ui32Value>>0 )&0xFF);
	}
	TFT_HW_CS_HI();
}

//*****************************************************************************
//
//! Fills a rectangle.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param pRect is a pointer to the structure describing the rectangle.
//! \param ui32Value is the color of the rectangle.
//!
//! This function fills a rectangle on the display.  The coordinates of the
//! rectangle are assumed to be within the extents of the display, and the
//! rectangle specification is fully inclusive (in other words, both i16XMin and
//! i16XMax are drawn, along with i16YMin and i16YMax).
//!
//! \return None.
//
//*****************************************************************************
static void TFT_HW_RectFill(void *pvDisplayData, const tRectangle *pRect,
		unsigned long ui32Value)
{
	unsigned int uY;

	ui32Value = DPYCOLORTRANSLATE(ui32Value);

	TFT_HW_SetRect(pRect->sXMin,pRect->sXMax,pRect->sYMin,pRect->sYMax);
	TFT_HW_Command(ILI9341_CMD_MEMORY_WRITE);
	for (uY = 0; uY < (((pRect->sXMax-pRect->sXMin)+1)*((pRect->sYMax-pRect->sYMin)+1)) ; ++uY)
	{
		TFT_HW_Write((ui32Value>>8 )&0xFF);
		TFT_HW_Write((ui32Value>>0 )&0xFF);
	}
	TFT_HW_CS_HI();
}

//*****************************************************************************
//
//! Translates a 24-bit RGB color to a display driver-specific color.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param ui32Value is the 24-bit RGB color.  The least-significant byte is the
//! blue channel, the next byte is the green channel, and the third byte is the
//! red channel.
//!
//! This function translates a 24-bit RGB color into a value that can be
//! written into the display's frame buffer in order to reproduce that color,
//! or the closest possible approximation of that color.
//!
//! \return Returns the display-driver specific color.
//
//*****************************************************************************
static unsigned long TFT_HW_ColorTranslate(void *pvDisplayData,
		unsigned long ui32Value)
{
	//
	// Translate from a 24-bit RGB color to a 3-3-2 RGB color.
	//
	return (ui32Value);
}

//*****************************************************************************
//
//! Flushes any cached drawing operations.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//!
//! This functions flushes any cached drawing operations to the display.  This
//! is useful when a local frame buffer is used for drawing operations, and the
//! flush would copy the local frame buffer to the display.  Since no memory
//! based frame buffer is used for this driver, the flush is a no operation.
//!
//! \return None.
//
//*****************************************************************************
static void TFT_HW_Flush(void *pvDisplayData)
{
	//
	// There is nothing to be done.
	//
}

//*****************************************************************************
//
//! The display structure that describes the driver
//
//*****************************************************************************
const tDisplay g_sILI9341_240x320x262K =
{
		sizeof(tDisplay), 0, 320, 240,
		TFT_HW_PixelDraw,
		TFT_HW_PixelDrawMultiple,
		TFT_HW_LineDrawH,
		TFT_HW_LineDrawV,
		TFT_HW_RectFill,
		TFT_HW_ColorTranslate,
		TFT_HW_Flush
};

//*****************************************************************************
//
//! Initializes the display driver.
//!
//! This function initializes the ILI9341 display controller on the panel,
//! preparing it to display data.
//!
//! \return None.
//
//*****************************************************************************
void ILI9341_240x320x262K_Init(void)
{
	//
	// Enable high performance bus
	//
	SysCtlGPIOAHBEnable(SYSCTL_PERIPH_GPIOJ);
	SysCtlGPIOAHBEnable(SYSCTL_PERIPH_GPIOG);
	SysCtlGPIOAHBEnable(SYSCTL_PERIPH_GPIOH);
	SysCtlGPIOAHBEnable(SYSCTL_PERIPH_GPIOC);

	//
	// Enable the peripherals used by this driver
	//
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOH);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);

	//
	// Configure controller control pins as GPIO output
	//
	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTH_AHB_BASE,
			GPIO_PIN_6 | GPIO_PIN_5 | GPIO_PIN_4);

	//
	// Put controller in reset
	//
	TFT_HW_RST_HI();

	//
	// Configure remaining controller control pins as GPIO output
	//
	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTJ_AHB_BASE, 0xFF);
	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTG_AHB_BASE, GPIO_PIN_7);
	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTC_AHB_BASE, GPIO_PIN_5);

	//
	// Set all signals as default (high - non active)
	//
	TFT_HW_RD_HI();
	TFT_HW_WR_HI();
	TFT_HW_RS_HI();
	TFT_HW_CS_HI();

	//
	// Reset chip by reset signal and command
	//
	TFT_HW_RST_LO();
	SysCtlDelay(500 * CPmS);
	TFT_HW_RST_HI();
	SysCtlDelay(200 * CPmS);
	TFT_HW_Command(ILI9341_CMD_SOFTWARE_RESET);
	TFT_HW_CS_HI();
	SysCtlDelay(200 * CPmS);

	//
	// Set up all registers
	//

	// Turn off display
	TFT_HW_Command(ILI9341_CMD_DISPLAY_OFF);
	TFT_HW_CS_HI();

	TFT_HW_Command(ILI9341_CMD_ENTER_SLEEP_MODE);
	TFT_HW_CS_HI();

	//Power control B
	TFT_HW_Command(ILI9341_CMD_POWER_CONTROL_B);
	TFT_HW_Write(0x00);
	TFT_HW_Write(0X83);
	TFT_HW_Write(0X30);
	TFT_HW_CS_HI();

	//Power on sequence control ?
	TFT_HW_Command(0xED); // NOT IN DATASHEET
	TFT_HW_Write(0x64);
	TFT_HW_Write(0x03);
	TFT_HW_Write(0X12);
	TFT_HW_Write(0X81);
	TFT_HW_CS_HI();

	//Driver timing control A
	TFT_HW_Command(ILI9341_CMD_DRIVER_TIMING_CONTROL_A);
	TFT_HW_Write(0x85);
	TFT_HW_Write(0x01);
	TFT_HW_Write(0x79);
	TFT_HW_CS_HI();

	//Power control A
	TFT_HW_Command(ILI9341_CMD_POWER_ON_SEQ_CONTROL);
	TFT_HW_Write(0x39);
	TFT_HW_Write(0x2C);
	TFT_HW_Write(0x00);
	TFT_HW_Write(0x34);
	TFT_HW_Write(0x02);
	TFT_HW_CS_HI();

	TFT_HW_Command(ILI9341_CMD_PUMP_RATIO_CONTROL);
	TFT_HW_Write(0x20);
	TFT_HW_CS_HI();

	//Driver timing control B
	TFT_HW_Command(ILI9341_CMD_DRIVER_TIMING_CONTROL_B);
	TFT_HW_Write(0x00);
	TFT_HW_Write(0x00);
	TFT_HW_CS_HI();

	//Power Control 1
	TFT_HW_Command(ILI9341_CMD_POWER_CONTROL_1);
	TFT_HW_Write(0x26);
	TFT_HW_CS_HI();

	//Power Control 2
	TFT_HW_Command(ILI9341_CMD_POWER_CONTROL_2);
	TFT_HW_Write(0x11);
	TFT_HW_CS_HI();

	//VCOM Control 1
	TFT_HW_Command(ILI9341_CMD_VCOM_CONTROL_1);
	TFT_HW_Write(0x35);
	TFT_HW_Write(0x3E);
	TFT_HW_CS_HI();

	TFT_HW_Command(ILI9341_CMD_VCOM_CONTROL_2);
	TFT_HW_Write(0xbe);
	TFT_HW_CS_HI();


	//Memory Access Control
	// Screen orientation
	TFT_HW_Command(ILI9341_CMD_MEMORY_ACCESS_CONTROL);
	TFT_HW_Write(0x28);
	TFT_HW_CS_HI();

	//COLMOD: Pixel Format Set
	// RGB and MUC interface set 16 bits/pixel
	TFT_HW_Command(ILI9341_CMD_COLMOD_PIXEL_FORMAT_SET);
	TFT_HW_Write(0x55);
	TFT_HW_CS_HI();

	//Frame Rate Control (In Normal Mode/Full Colors)
	TFT_HW_Command(ILI9341_CMD_FRAME_RATE_CONTROL_NORMAL);
	TFT_HW_Write(0x00);
	TFT_HW_Write(0x1B);
	TFT_HW_CS_HI();

	// 3 gamma
	TFT_HW_Command(ILI9341_CMD_ENABLE_3_GAMMA_CONTROL);
	TFT_HW_Write(0x08);
	TFT_HW_CS_HI();

	//Gamma Set
	TFT_HW_Command(ILI9341_CMD_GAMMA_SET);
	TFT_HW_Write(0x01);
	TFT_HW_CS_HI();

	//Positive Gamma Correction
	TFT_HW_Command(ILI9341_CMD_POSITIVE_GAMMA_CORRECTION);
	TFT_HW_Write(0x1F);
	TFT_HW_Write(0x1A);
	TFT_HW_Write(0x18);
	TFT_HW_Write(0x0a);
	TFT_HW_Write(0x0f);
	TFT_HW_Write(0x06);
	TFT_HW_Write(0x45);
	TFT_HW_Write(0x87);
	TFT_HW_Write(0x32);
	TFT_HW_Write(0x0a);
	TFT_HW_Write(0x07);
	TFT_HW_Write(0x02);
	TFT_HW_Write(0x07);
	TFT_HW_Write(0x05);
	TFT_HW_Write(0x00);
	TFT_HW_CS_HI();

	//Negative Gamma Correction
	TFT_HW_Command(ILI9341_CMD_NEGATIVE_GAMMA_CORRECTION);
	TFT_HW_Write(0x00);
	TFT_HW_Write(0x25);
	TFT_HW_Write(0x27);
	TFT_HW_Write(0x05);
	TFT_HW_Write(0x10);
	TFT_HW_Write(0x09);
	TFT_HW_Write(0x3a);
	TFT_HW_Write(0x78);
	TFT_HW_Write(0x4d);
	TFT_HW_Write(0x05);
	TFT_HW_Write(0x18);
	TFT_HW_Write(0x0d);
	TFT_HW_Write(0x38);
	TFT_HW_Write(0x3a);
	TFT_HW_Write(0x1f);
	TFT_HW_CS_HI();

	// ddram
	TFT_HW_Command(ILI9341_CMD_COLUMN_ADDRESS_SET);
	TFT_HW_Write(0x00);
	TFT_HW_Write(0x00);
	TFT_HW_Write(0x00);
	TFT_HW_Write(0xEF);
	TFT_HW_CS_HI();

	TFT_HW_Command(ILI9341_CMD_PAGE_ADDRESS_SET);
	TFT_HW_Write(0x00);
	TFT_HW_Write(0x00);
	TFT_HW_Write(0x01);
	TFT_HW_Write(0x3F);
	TFT_HW_CS_HI();

	TFT_HW_Command(ILI9341_CMD_TEARING_EFFECT_LINE_OFF);
	TFT_HW_CS_HI();

	TFT_HW_Command(ILI9341_CMD_TEARING_EFFECT_LINE_ON);
	TFT_HW_CS_HI();

	TFT_HW_Command(ILI9341_CMD_DISPLAY_INVERSION_CONTROL);
	TFT_HW_Write(0x00);
	TFT_HW_CS_HI();

	TFT_HW_Command(ILI9341_CMD_ENTRY_MODE_SET);
	TFT_HW_Write(0x07);
	TFT_HW_CS_HI();

	//Display Function Control
	TFT_HW_Command(ILI9341_CMD_DISPLAY_FUNCTION_CONTROL);
	TFT_HW_Write(0x0A);
	TFT_HW_Write(0x02);
	TFT_HW_Write(0x27);
	TFT_HW_Write(0x00);
	TFT_HW_CS_HI();

	TFT_HW_Command(ILI9341_CMD_RGB_SIGNAL_CONTROL);
	TFT_HW_Write(0xC0);
	TFT_HW_CS_HI();

	TFT_HW_Command(ILI9341_CMD_FRAME_RATE_CONTROL_NORMAL);
	TFT_HW_Write(0x00);
	TFT_HW_Write(0x1B);
	TFT_HW_CS_HI();

	TFT_HW_Command(ILI9341_CMD_INTERFACE_CONTROL);
	TFT_HW_Write(0x00);
	TFT_HW_Write(0x00);
	TFT_HW_Write(0x00);
	TFT_HW_CS_HI();

	// Sleep Out
	TFT_HW_Command(ILI9341_CMD_SLEEP_OUT);
	TFT_HW_CS_HI();
	SysCtlDelay(120 * CPmS);

	//Display On
	TFT_HW_Command(ILI9341_CMD_DISPLAY_ON);
	TFT_HW_CS_HI();
	SysCtlDelay(1000 * CPmS);

	TFT_HW_Command(ILI9341_CMD_DISP_INVERSION_ON);
	TFT_HW_CS_HI();

	//
	// Fill the entire display with a black rectangle, to clear it.
	//
	tRectangle rect;
	rect.sXMin=0;
	rect.sYMin=0;
	rect.sXMax=320-1;
	rect.sYMax=240-1;
	TFT_HW_RectFill(0, &rect, ClrWhite);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
