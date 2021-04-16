/*
 * weather_station_display_defs.h
 *
 *  Created on: 6 sty 2021
 *      Author: Eryk
 */

#ifndef WEATHER_STATION_DISPLAY_DEFS_H_
#define WEATHER_STATION_DISPLAY_DEFS_H_


#include <stdint.h>
#include <stddef.h>
#include "stdbool.h"

struct displayed_data
{
	//if weather data isn't displayed, this flag should be true
	tBoolean bme280_empty;

	//if time isn't displayed, this flag should be true
	tBoolean time_empty;

	//current temperature value on display
	float temperature;

	//current pressure value on display
	uint32_t pressure;

	//current humidity value on display
	uint32_t humidity;


	uint8_t minutes;

	uint8_t hours;
	//currently displayed day of the week
	uint8_t dow;

	uint8_t day;

	uint8_t month;

	uint8_t year;

};



#endif /* WEATHER_STATION_DISPLAY_DEFS_H_ */
