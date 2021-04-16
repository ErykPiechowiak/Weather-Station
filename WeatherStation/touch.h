//*****************************************************************************
//
// touch.h - Prototypes for the touch screen driver.
//
// Copyright (c) 2008-2012 Texas Instruments Incorporated.  All rights reserved.
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

#ifndef __TOUCH_H__
#define __TOUCH_H__



//*****************************************************************************
//
// The lowest ADC reading assumed to represent a press on the screen.  Readings
// below this indicate no press is taking place.
//
//*****************************************************************************
//#define TOUCH_MIN 				1700
#define TOUCH_MIN 				200

//*****************************************************************************
//
// How many readings ignore before next touch event can happen.
//
//*****************************************************************************
#define TOUCH_DEBOUNCE_IGNORE 	500

//*****************************************************************************
//
// How often collect samples ( in [Hz] )
//
//*****************************************************************************
#define TOUCH_POOL_TIME 		30

//*****************************************************************************
//
// Prototypes for the functions exported by the touch screen driver.
//
//*****************************************************************************
extern void TouchScreenIntHandler(void);
extern void TouchScreenInit(void);
extern void TouchScreenCalibrate(tContext *);
extern void TouchScreenCallbackSet(int32_t (*pfnCallback)(uint32_t, int32_t, int32_t));

#endif // __TOUCH_H__
