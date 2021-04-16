//*****************************************************************************
//
// touch.c - Touch screen driver for the board.
//
// Copyright (c) 2008-2013 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
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
// Maciej Kucia July 2013
//
//*****************************************************************************

//*****************************************************************************
//
//! \addtogroup touch_api
//! @{
//
//*****************************************************************************

#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_adc.h"
#include "inc/hw_gpio.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_timer.h"
#include "inc/hw_types.h"
#include "driverlib/adc.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/rom.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "touch.h"

//*****************************************************************************
//
// The GPIO pins to which the touch screen is connected.
//
//*****************************************************************************
#define TS_DRIVE_PERIPH             SYSCTL_PERIPH_GPIOE
#define TS_DRIVE_BASE               GPIO_PORTE_BASE
#define TS_DRIVEA_PIN               GPIO_PIN_0
#define TS_DRIVEB_PIN               GPIO_PIN_1

#define TS_READ_PERIPH             	SYSCTL_PERIPH_GPIOB
#define TS_READ_BASE               	GPIO_PORTB_BASE
#define TS_X_PIN               		GPIO_PIN_4
#define TS_Y_PIN               		GPIO_PIN_5

//*****************************************************************************
//
// The ADC channels connected to each of the touch screen contacts.
//
//*****************************************************************************
#define ADC_CTL_CH_X ADC_CTL_CH10
#define ADC_CTL_CH_Y ADC_CTL_CH11

//*****************************************************************************
//
// These simple macros make code easier to read
//
//*****************************************************************************
#define X 0
#define Y 1

//*****************************************************************************
//
// The current state of the touch screen driver's state machine.  This is used
// to cycle the touch screen interface through the powering sequence required
// to read the two axes of the surface.
//
//*****************************************************************************
volatile enum
{
    //
    // read-x and read-y unconnected from GND
    //
	TS_STATE_READ_TOUCH,

    //
    // y-grounded
    //
	TS_STATE_READ_X,

    //
    // x-grounded
    //
	TS_STATE_READ_Y

}
g_enuTSState = TS_STATE_READ_TOUCH;

//*****************************************************************************
//
// The most recent raw ADC reading for the X position on the screen.  This
// value is not affected by the selected screen orientation.
//
//*****************************************************************************
volatile int16_t g_i16TouchX;

//*****************************************************************************
//
// The most recent raw ADC reading for the Y position on the screen.  This
// value is not affected by the selected screen orientation.
//
//*****************************************************************************
volatile int16_t g_i16TouchY;

//*****************************************************************************
//
// Converted touch coordinates
//
//*****************************************************************************
volatile int16_t g_i16X=0;
volatile int16_t g_i16Y=0;

//*****************************************************************************
//
// A pointer to the function to receive messages from the touch screen driver
// when events occur on the touch screen (debounced presses, movement while
// pressed, and debounced releases).
//
//*****************************************************************************
static int32_t (*g_pfnTSHandler)(uint32_t, int32_t, int32_t);

//*****************************************************************************
//
// The current state of the touch screen.  When 0, the pen is up.
// When 1, the pen is down.
//
//*****************************************************************************
volatile uint8_t g_u8State = 0;

//*****************************************************************************
//
// Stores amount of ignored readings left
//
//*****************************************************************************
uint32_t g_u32TouchDebouncer;

//*****************************************************************************
//
// Linear scaling matrix. Allow transforming raw reading to touch X,Y location
// Warning! Those variables are floats to show Cortex-M4 FPU capabilities
//          It is a good idea to make drivers as fast as possible avoiding
//          use of float or double.
//
//*****************************************************************************

//[MKra] Inicjalizacja aby uniknac kalibracji
float g_floTouchCalibrationA = -0.6147541;
float g_floTouchCalibrationB = 334.5901;
float g_floTouchCalibrationC = -0.4954955;
float g_floTouchCalibrationD = 261.2162;



//*****************************************************************************
//
// Semaphore used in calibration process
//
//*****************************************************************************
volatile uint8_t g_u8TouchCallbackSemaphore = 0;

//*****************************************************************************
//
//! Debounces presses of the touch screen.
//!
//! This function is called when a new X/Y sample pair has been captured in
//! order to perform debouncing of the touch screen.
//!
//! \return None.
//
//*****************************************************************************
static void TouchScreenDebouncer(bool isTouch)
{
	// See if the touch screen is being touched.
	//
	if (!isTouch)
	{
		//
		// See if the pen was down before
		//
		if (g_u8State != 0)
		{
			//
			// Indicate that the pen is up.
			//
			g_u8State = 0;

			//
			// See if there is a touch screen event handler.
			//
			if (g_pfnTSHandler)
			{
				//
				// Send the pen up message to the touch screen event
				// handler. Use last known X,Y location.
				//
				g_pfnTSHandler(WIDGET_MSG_PTR_UP, g_i16X, g_i16Y);
			}
		}
	}
	else
	{
		//
		// Convert the ADC readings into pixel values on the screen.
		//
		g_i16X = (int16_t) ((g_i16TouchX * g_floTouchCalibrationA) + g_floTouchCalibrationB);
		g_i16Y = (int16_t) ((g_i16TouchY * g_floTouchCalibrationC) + g_floTouchCalibrationD);

		//
		// Was pen UP before?
		//
		if (g_u8State == 0)
		{
			g_u8State = 1;

			if (g_pfnTSHandler)
			{
				//
				// Send the pen down message to the touch screen event
				// handler.
				//
				if (g_u32TouchDebouncer == 0)
				{
					g_pfnTSHandler(WIDGET_MSG_PTR_DOWN, g_i16X, g_i16Y);
					//
					// Ignore some readings after event (debounce)
					//
					g_u32TouchDebouncer += 3 * TOUCH_DEBOUNCE_IGNORE;
				}
			}
		}
		//
		// Pen was down before
		//
		else
		{
			//
			// See if there is a touch screen event handler.
			//
			if (g_pfnTSHandler)
			{
				//
				// Send the pen move message to the touch screen event
				// handler.
				//
				g_pfnTSHandler(WIDGET_MSG_PTR_MOVE, g_i16X, g_i16Y);
			}

		}
	}
}

//*****************************************************************************
//
//! Handles the ADC interrupt for the touch screen.
//!
//! This function is called when the ADC sequence that samples the touch screen
//! has completed its acquisition.  The touch screen state machine is advanced
//! and the acquired ADC sample is processed appropriately.
//!
//! It is the responsibility of the application using the touch screen driver
//! to ensure that this function is installed in the interrupt vector table for
//! the ADC3 interrupt.
//!
//! \return None.
//
//*****************************************************************************
void TouchScreenIntHandler(void)
{
	unsigned long temp;

	//
	// Clear the ADC sample sequence interrupt.
	// Read data
	//
	ROM_ADCIntClear (ADC0_BASE, 3);
	ROM_ADCSequenceDataGet (ADC0_BASE, 3, &temp);

	//
	// If touch event happened, ignore some readings
	//
	if (g_u32TouchDebouncer > 0)
	{
		--g_u32TouchDebouncer;
		return;
	}

	//
	// Determine what to do based on the current state of the state machine.
	//
	switch (g_enuTSState)
	{
		//
		// The state machine is in its initial state
		//
		case TS_STATE_READ_TOUCH:
		{
			//
			// Is screen being touched?
			//
			if (temp > TOUCH_MIN)
			{
				//
				// The new sample is an X axis sample that should be processed.
				//
				g_enuTSState = TS_STATE_READ_X;
				ROM_GPIOPinWrite (TS_DRIVE_BASE, TS_DRIVEA_PIN | TS_DRIVEB_PIN, TS_DRIVEB_PIN);
				ROM_ADCSequenceStepConfigure (ADC0_BASE, 3, 0, ADC_CTL_CH_X | ADC_CTL_END | ADC_CTL_IE);
			}
			else
			{
				//
				// Nope, keep checking for touch
				//
				g_enuTSState = TS_STATE_READ_TOUCH;
				ROM_GPIOPinWrite (TS_DRIVE_BASE, TS_DRIVEA_PIN | TS_DRIVEB_PIN, 0);
				ROM_ADCSequenceStepConfigure (ADC0_BASE, 3, 0, ADC_CTL_CH_X | ADC_CTL_END | ADC_CTL_IE);
				TouchScreenDebouncer(false);
			}

			g_u32TouchDebouncer=1;
			break;
		}

		case TS_STATE_READ_X:
		{
			//
			// Read the raw ADC sample.
			//
			g_i16TouchX = (int16_t) temp;

			//
			// The next sample will be an Y axis sample.
			//
			g_enuTSState = TS_STATE_READ_Y;
			ROM_GPIOPinWrite (TS_DRIVE_BASE, TS_DRIVEA_PIN | TS_DRIVEB_PIN, TS_DRIVEA_PIN);
			ROM_ADCSequenceStepConfigure (ADC0_BASE, 3, 0, ADC_CTL_CH_Y | ADC_CTL_END | ADC_CTL_IE);
			g_u32TouchDebouncer=1;
			break;
		}

			//
			// The new sample is a Y axis sample that should be processed.
			//
		case TS_STATE_READ_Y:
		{
			g_i16TouchY = (int16_t) temp;
			//
			// The next sample will determine if screen is being touched.
			//
			g_enuTSState = TS_STATE_READ_TOUCH;
			ROM_GPIOPinWrite (TS_DRIVE_BASE, TS_DRIVEA_PIN | TS_DRIVEB_PIN, 0);
			ROM_ADCSequenceStepConfigure (ADC0_BASE, 3, 0, ADC_CTL_CH_X | ADC_CTL_END | ADC_CTL_IE);

			TouchScreenDebouncer(true);

			g_u32TouchDebouncer=1;
			break;
		}
	}
}

//*****************************************************************************
//
//! Initializes the touch screen driver.
//!
//! This function initializes the touch screen driver, beginning the process of
//! reading from the touch screen.  This driver uses the following hardware
//! resources:
//!
//! - ADC sample sequence 3
//! - Timer 1 subtimer A
//!
//! \return None.
//
//*****************************************************************************
void
TouchScreenInit(void)
{
	 g_u32TouchDebouncer = 0;

    //
    // Enable the peripherals used by the touch screen interface.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    ROM_SysCtlPeripheralEnable(TS_READ_PERIPH);
    ROM_SysCtlPeripheralEnable(TS_DRIVE_PERIPH);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);

	//
	// Configure ADC for Read X (PB4,AIN10) and Read Y (PB5,AIN11)
	//
    GPIOPinTypeADC(TS_READ_BASE, TS_X_PIN | TS_Y_PIN);

    //
    // There is no touch screen handler initially.
    //
    g_pfnTSHandler = 0;

    //
    // Configure the ADC sample sequence used to read the touch screen reading.
    //
    ROM_ADCHardwareOversampleConfigure(ADC0_BASE, 4);
    ROM_ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_TIMER, 0);
    ROM_ADCSequenceStepConfigure(ADC0_BASE, 3, 0, ADC_CTL_CH_Y | ADC_CTL_END | ADC_CTL_IE);
    ROM_ADCSequenceEnable(ADC0_BASE, 3);

    //
    // Configure the GPIOs used to drive the touch screen layers.
    //
    ROM_GPIOPinTypeGPIOOutput(TS_DRIVE_BASE, TS_DRIVEA_PIN | TS_DRIVEB_PIN);
    ROM_GPIOPinWrite(TS_DRIVE_BASE, TS_DRIVEA_PIN | TS_DRIVEB_PIN, 0x00);

    //
    // See if the ADC trigger timer has been configured, and configure it only
    // if it has not been configured yet.
    //
    if((HWREG(TIMER1_BASE + TIMER_O_CTL) & TIMER_CTL_TAEN) == 0)
    {
        //
        // Configure the timer to trigger the sampling of the touch screen
        // every 10 milliseconds.
        //
    	ROM_TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);
    	//ROM_TimerConfigure(TIMER1_BASE, (TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PERIODIC | TIMER_CFG_B_PERIODIC));
        ROM_TimerLoadSet(TIMER1_BASE, TIMER_A, (ROM_SysCtlClockGet() / TOUCH_POOL_TIME) - 1);
        ROM_TimerControlTrigger(TIMER1_BASE, TIMER_A, true);

        //
        // Enable the timer.  At this point, the touch screen state machine
        // will sample and run once per millisecond.
        //
        ROM_TimerEnable(TIMER1_BASE, TIMER_A);
    }

    //
    // Dispose some first samples
    //
    g_u32TouchDebouncer=10;

    //
    // Enable the ADC sample sequence interrupt.
    //
    ROM_ADCIntEnable(ADC0_BASE, 3);
    ROM_IntEnable(INT_ADC0SS3);
}

//*****************************************************************************
//
//! Sets the callback function for touch screen events.
//!
//! \param pfnCallback is a pointer to the function to be called when touch
//! screen events occur.
//!
//! This function sets the address of the function to be called when touch
//! screen events occur.  The events that are recognized are the screen being
//! touched (``pen down''), the touch position moving while the screen is
//! touched (``pen move''), and the screen no longer being touched (``pen
//! up'').
//!
//! \return None.
//
//*****************************************************************************
void TouchScreenCallbackSet(int32_t (*pfnCallback)(uint32_t ulMessage, int32_t g_i16X, int32_t g_i16Y))
{
    //
    // Save the pointer to the callback function.
    //
    g_pfnTSHandler = pfnCallback;
}

//*****************************************************************************
//
//! Callback for calibration process
//!
//! \param ulMessage is type of event
//! \param lX is a x location of touch
//! \param lY is a y location of cross center
//!
//! \return None.
//
//*****************************************************************************
int32_t TouchCalibrationCallback(uint32_t ulMessage, int32_t lX, int32_t lY)
{
	//
	// Open semaphore on touch event
	//
	if (ulMessage == WIDGET_MSG_PTR_DOWN)
		g_u8TouchCallbackSemaphore = 1;

	return 0;
}

//*****************************************************************************
//
//! Draws small cross on screen
//!
//! \param psContext is a pointer to the graphics context
//!
//! \param x is a x location of cross center
//!
//! \param y is a y location of cross center
//!
//! \param colour defines color of cross lines
//!
//! \return None.
//
//*****************************************************************************
inline void DrawCross(tContext *psContext, uint16_t x, uint16_t y, uint32_t colour)
{
	GrContextForegroundSet(psContext, colour);
	GrLineDraw(psContext,-10+x,0+y,10+x,0+y);
	GrLineDraw(psContext,0+x,-10+y,0+x,10+y);
}

//*****************************************************************************
//
//! Performs screen calibration
//!
//! \param psContext is a pointer to the graphics context
//!
//! \return None.
//
//*****************************************************************************
void TouchScreenCalibrate(tContext *psContext)
{


//    g_floTouchCalibrationA = 1;//((E[1][X]-E[0][X])) / (M[1][X]-M[0][X]);
//    g_floTouchCalibrationB = 0; //E[0][X] - (M[0][X] * g_floTouchCalibrationA);
//
//    g_floTouchCalibrationC = 1;// ((E[1][Y]-E[0][Y])) / (M[1][Y]-M[0][Y]);
//    g_floTouchCalibrationD = 0;//E[0][Y] - (M[0][Y] * g_floTouchCalibrationC);


	uint8_t i;

	//
	// M stores measured values
	//
	float M[2][2] = { {0,0}, {0,0} };

	//
	// E stores expected values
	//
	float E[2][2] = { {10,10}, {310,230} };

	//
	// Set temporary callback
	//
	TouchScreenCallbackSet(TouchCalibrationCallback);

	//
	// Draw message
	//
	GrContextFontSet(psContext, g_pFontCm14b);
    GrStringDrawCentered(psContext, "Touchscreen calibration: Press on marks", -1, GrContextDpyWidthGet(psContext)/2, 30, 0);

    g_u8TouchCallbackSemaphore = 0;
   	while( !g_u8TouchCallbackSemaphore );

    //
    // Do measurements
    //
    for (i=0;i<2;++i)
    {
		//
		// Show cross
		//
		DrawCross(psContext, E[i][X], E[i][Y], ClrBlack);

		//
		// Wait for touch and store measurements
		//
		g_u8TouchCallbackSemaphore = 0;
		while( !g_u8TouchCallbackSemaphore );
		M[i][X] = g_i16TouchX;
		M[i][Y] = g_i16TouchY;

		//
		// Remove cross
		//
		DrawCross(psContext, E[i][X], E[i][Y], ClrWhite);
    }

	//
	// Now calculate the coefficients
	// Fixed point arithmetic would be a good idea here.
    // We have FPU so float will give us acceptable performance.
    //
    g_floTouchCalibrationA = ((E[1][X]-E[0][X])) / (M[1][X]-M[0][X]);
    g_floTouchCalibrationB = E[0][X] - (M[0][X] * g_floTouchCalibrationA);

    g_floTouchCalibrationC = ((E[1][Y]-E[0][Y])) / (M[1][Y]-M[0][Y]);
    g_floTouchCalibrationD = E[0][Y] - (M[0][Y] * g_floTouchCalibrationC);

	//
	// One can use the following code to check if calibration was successful
	//
//	for(;;)
//	{
//		TouchCallbackSemaphore=0;
//		while(!TouchCallbackSemaphore);
//		DrawCross(psContext,lX,lY,ClrRed);
//	}

    GrContextFontSet(psContext, g_pFontCm14b);
    GrContextForegroundSet(psContext, ClrWhite);
    GrStringDrawCentered(psContext, "Touchscreen calibration: Press on marks", -1, GrContextDpyWidthGet(psContext)/2, 30, 0);

	//
	// Release callback
	//
	TouchScreenCallbackSet(0);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
