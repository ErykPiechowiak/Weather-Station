#include "stdint.h"
#include "stdio.h"
#include "stdbool.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/fpu.h"
#include "inc/hw_i2c.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/pin_map.h"
#include "driverlib/i2c.h"
#include "utils/uartstdio.h"
#include "driverlib/uart.h"
#include "grlib/grlib.h"
#include "ili9341_240x320x262K.h"
#include "bme280.h"
#include "DS3231.h"
#include "weather_station_display_defs.h"
#include "driverlib/systick.h"
#include "touch.h"
#include "mmc.h"
#include "time.h"
#include "grlib/widget.h"
#include "grlib/pushbutton.h"

/*
 * MICROSD DATA INFO
 *
 * BLOCK 1: pressure history data
 * BLOCK 2: pressure history data time
 * BLOCK 3: daily temp data
 * BLOCK 4: daily temp data time
 * BLOCK 5: weekly max temp data
 * BLOCK 6: weekly max temp data time
 * BLOCK 7: daily humidity data
 * BLOCK 8: daily humidity data time
 * BLOCK 9: weekly humidity data
 * BLOCK 10: weekly humidity data time
 * BLOCK 11: weekly min temp data
 * BLOCK 12: weekly min temp data time
 *
 */

extern const uint8_t g_pucBackground[]; //Home screen background

#define GPIO_PINS_ALL GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7

volatile uint32_t g_ui32SysTickCount;
tContext sContext;
/*
 * SCREENS
 * 0: Home Screen
 * 0b: Pressure info
 * 1: Daily Temp Screen
 * 1:b 12-23
 * 2: Daily Humidity Screen
 */
int screen = 0;
tBoolean screen_0b = false;
tBoolean screen_1b = false;
tBoolean screen_changed = false;
tBoolean dialog_answer = false;
tBoolean daily_flag = true; //if false weekly_temp is displayed
tBoolean power_off = false; //is set to true if power button was pressed


//BME280
tBoolean first_measurement = true; //first bme280 measurement is invalid
tBoolean config = false; //by default config is set to false which means that bme280 wasn't earlier configured

/*
 * Pressure history graph
 */
tBoolean pressure_graph_ready = false;
tBoolean phd_save_locked = true; //when it's true, saving a pressure history data on the microSD card isn't possible
uint32_t pressure_history_data[13] = {0};
RTC_Time pressure_history_data_time[13];
uint32_t pressure_history_loaded_data[13];
uint32_t pressure_history_loaded_data_time[13];//describing when pressure_history_loaded_data was measured

/*
 * daily temp
 */
float last_hour_temp = 0; //Sum of the last hour temperature measurements
int temp_measurements_counter = 1; //Counts how many temperature measurements were performed last hour
float daily_temp[24];
uint32_t daily_temp_to_save[24];
RTC_Time daily_temp_time[24];
uint32_t loaded_daily_temp[24];
float f_loaded_daily_temp[24];
RTC_Time loaded_daily_temp_time[24];

/*
 * weekly temp
 */
float weekly_max_temp[7];
float weekly_min_temp[7];
uint32_t weekly_temp_to_save[7];
RTC_Time weekly_max_temp_time[7];
RTC_Time weekly_min_temp_time[7];
uint32_t loaded_weekly_max_temp[7];
uint32_t loaded_weekly_min_temp[7];

RTC_Time loaded_weekly_max_temp_time[7];
RTC_Time loaded_weekly_min_temp_time[7];


//daily humidity
 
uint32_t last_hour_humidity = 0;
int humidity_measurements_counter = 1;
uint32_t daily_humidity[24];
RTC_Time daily_humidity_time[24];
uint32_t loaded_daily_humidity[24];
RTC_Time loaded_daily_humidity_time[24];


 //weekly humidity
 
uint32_t weekly_humidity[7];
RTC_Time weekly_humidity_time[7];
uint32_t loaded_weekly_humidity[7];
RTC_Time loaded_weekly_humidity_time[7];
tBoolean clear_weekly_humidity_data_flag = false;

char day_of_the_week[7][10] = {
		"niedz.",
		"pon.",
		"wt.",
		"sr.",
		"czw.",
		"pia.",
		"sob."

};

struct displayed_data displayed_data;


 //TOUCHSCREEN DATA
 
int32_t firstX; // finger down x position
int32_t firstY; // finger down y position
tBoolean finger_down = false;


//TOUCHSCREEN//
int32_t TouchCallback(uint32_t ulMessage, int32_t lX, int32_t lY)
{
	if (ulMessage == WIDGET_MSG_PTR_DOWN && (screen == 1 || screen == 2) && lY > 60 && lY < 200)
	{
		firstX = lX;
		firstY = lY;
		finger_down = true;
	}
	//gesture to the left
	else if (ulMessage== WIDGET_MSG_PTR_MOVE && lX < firstX - 150 && finger_down == true && (screen == 1 || screen == 2) && !screen_1b) {

		firstX = lX;
		firstY = lY;
		finger_down = false;
		screen_1b = true;
		screen_changed = true;
	}
	//gesture to the right
	else if (ulMessage == WIDGET_MSG_PTR_MOVE && lX > firstX + 150 && finger_down == true && (screen == 1 || screen == 2) && screen_1b) {

		firstX = lX;
		firstY = lY;
		finger_down = false;
		screen_1b = false;
		screen_changed = true;
	}
	//pressure info
	if(ulMessage == WIDGET_MSG_PTR_DOWN && lX > 0 && lX < 320 && lY > 100 && lY < 200 && screen == 0 && screen_changed == false && !power_off)
		{
			screen_0b = !screen_0b;
			screen_changed = true;
		}

	//temp graph
	if(ulMessage == WIDGET_MSG_PTR_DOWN && lX > 0 && lX < 100 && lY > 0 && lY < 100 && screen == 0 && screen_changed == false && !power_off)
	{
		screen = 1;
		screen_changed = true;
	}
	//humidity graph
	if(ulMessage == WIDGET_MSG_PTR_DOWN && lX > 220 && lX < 320 && lY > 0 && lY < 100 && screen == 0 && screen_changed == false && !power_off)
	{
		screen = 2;
		screen_changed = true;
	}
	//back button
	else if(ulMessage == WIDGET_MSG_PTR_DOWN && lX > 0 && lX < 100 && lY > 0 && lY < 40 && (screen == 1 || screen == 2) && screen_changed == false)
	{
		screen = 0;
		screen_changed = true;
	}
	//weekly/daily temp button
	else if(ulMessage == WIDGET_MSG_PTR_DOWN && lX > 220 && lX < 340 && lY > 0 && lY < 40 && (screen == 1 || screen == 2) && screen_changed == false)
	{
		daily_flag = !daily_flag;
		screen_changed = true;
	}
	//power off button
	else if(ulMessage == WIDGET_MSG_PTR_DOWN && lX > 110 && lX < 210 && lY > 0 && lY < 100 && screen == 0 && !power_off)
	{
		power_off = true;
	}
	//dialog "yes"
	else if(ulMessage == WIDGET_MSG_PTR_DOWN && lX > 50 && lX < 120 && lY > 120 && lY < 170 && screen == 0 && power_off)
	{
		dialog_answer = true;
	}
	//dialog "no"
	else if(ulMessage == WIDGET_MSG_PTR_DOWN && lX > 200 && lX < 270 && lY > 120 && lY < 170 && screen == 0 && power_off)
	{
		dialog_answer = true;
		power_off = false;
	}


    return 0;
}

void prepare_temp_to_save(float data[], uint32_t data_to_save[], int n ){

	int i;
	float temp;
	for(i=0;i<n;i++)
	{
		temp = data[i]*10;
		data_to_save[i]=temp;
	}
}


/*
 * block_addr - Address of the block where the data_array will be saved
 * n - Number of data to save
 * data_array - data to be saved
 * time_array - data to be saved
 * daily - true if the data to save contains daily measurements
 */
void save_temp_or_humidity(unsigned int block_nr, int n, uint32_t data_array[], RTC_Time time_array[], tBoolean daily)
{
	//saving data_array
	unsigned char buffer[511] = {0};
	unsigned char data_byte = 0;
	int i;
	int data_counter = 0;
	int shift = 24;
	for (i = 0; i < n*4; i++)
	{
		data_byte = data_array[data_counter] >> shift;
		shift -= 8;
		buffer[i] = data_byte;
		if (shift < 0)
		{
			shift = 24;
			data_counter++;
		}
	}
	writeSingleBlock(buffer, block_nr*512);
	block_nr++;

	//saving time_array
	data_counter = 0;
	shift = 24;
	for (i = 0; i < n; i++)
	{

		if (daily)
		{
			buffer[data_counter] = time_array[i].hours;
			data_counter++;
			buffer[data_counter] = time_array[i].day;
			data_counter++;
			buffer[data_counter] = time_array[i].month;
			data_counter++;
			buffer[data_counter] = time_array[i].year;
			data_counter++;
		}
		else
		{
			buffer[data_counter] = time_array[i].day;
			data_counter++;
			buffer[data_counter] = time_array[i].dow;
			data_counter++;
			buffer[data_counter] = time_array[i].month;
			data_counter++;
			buffer[data_counter] = time_array[i].year;
			data_counter++;
		}
	}


	writeSingleBlock(buffer, block_nr*512);
}


/*
 * block_addr - Address of the block where the data was saved
 * n - Number of bytes to load
 * data_array - pointer to the array where the loaded data will be stored
 * time_array - pointer to the array where the loaded data will be stored
 * daily - true if the data to load contains daily measurements
 */
void load_temp_or_humidity(unsigned int block_nr, int n, uint32_t *data_array, RTC_Time *time_array, tBoolean daily)
{
	//read data
	int rslt;
	unsigned char buffer[511];
	rslt = readSingleBlock(buffer, block_nr*512);
	if (rslt == 0)
	{
		int i;
		int data_counter = 0;
		int shift = 24;
		for (i = 0; i < n*4; i++)
		{
			data_array[data_counter] |= buffer[i] << shift;
			shift -= 8;
			if (shift < 0)
			{
				data_counter++;
				shift = 24;
			}
		}
		//read time data
		block_nr++;
		readSingleBlock(buffer, block_nr * 512);
		data_counter = 0;
		for (i = 0; i < n; i++)
		{
			if(daily)
			{
				time_array[i].hours = buffer[data_counter];
				data_counter++;
				time_array[i].day = buffer[data_counter];
				data_counter++;
				time_array[i].month = buffer[data_counter];
				data_counter++;
				time_array[i].year = buffer[data_counter];
				data_counter++;
			}
			else
			{
				time_array[i].day = buffer[data_counter];
				data_counter++;
				time_array[i].dow = buffer[data_counter];
				data_counter++;
				time_array[i].month = buffer[data_counter];
				data_counter++;
				time_array[i].year = buffer[data_counter];
				data_counter++;
			}
		}
	}
}



//Checks if the loaded temperature data is from the current day

void check_loaded_daily_temp()
{
	int i;
	float temp;
	for(i=0;i<24;i++)
	{
		temp = (float)loaded_daily_temp[i]/10;
		f_loaded_daily_temp[i] = temp;
		if(loaded_daily_temp_time[i].day == displayed_data.day && loaded_daily_temp_time[i].year == displayed_data.year)
		{
			daily_temp[loaded_daily_temp_time[i].hours] = f_loaded_daily_temp[i];
			daily_temp_time[loaded_daily_temp_time[i].hours].hours = loaded_daily_temp_time[i].hours;
			daily_temp_time[loaded_daily_temp_time[i].hours].day = loaded_daily_temp_time[i].day;
			daily_temp_time[loaded_daily_temp_time[i].hours].year = loaded_daily_temp_time[i].year;
		}
	}
}



 //Checks if the loaded humidity data is from the current day

void check_loaded_daily_humidity()
{
	int i;
	for(i=0;i<24;i++)
	{
		if(loaded_daily_humidity_time[i].day == displayed_data.day && loaded_daily_humidity_time[i].year == displayed_data.year)
		{
			daily_humidity[loaded_daily_humidity_time[i].hours] = loaded_daily_humidity[i];
			daily_humidity_time[loaded_daily_humidity_time[i].hours].hours = loaded_daily_humidity_time[i].hours;
			daily_humidity_time[loaded_daily_humidity_time[i].hours].day = loaded_daily_humidity_time[i].day;
			daily_humidity_time[loaded_daily_humidity_time[i].hours].year = loaded_daily_humidity_time[i].year;
		}
	}
}


void draw_or_erase_right_arrow(tBoolean erase)
{
	if(erase)
		GrContextForegroundSet(&sContext, ClrBlack);
	else
		GrContextForegroundSet(&sContext, ClrWhite);
	GrLineDrawH(&sContext, 280, 300, 220);
	int i;
	short x_position = 299;
	short y_position = 219;
	for(i=0;i<3;i++)
	{
		GrPixelDraw(&sContext, x_position, y_position);
		x_position--;
		y_position--;
	}
	x_position = 299;
	y_position = 221;
	for(i=0;i<3;i++)
	{
		GrPixelDraw(&sContext, x_position, y_position);
		x_position--;
		y_position++;
	}


}


void draw_or_erase_left_arrow(tBoolean erase)
{
	if(erase)
		GrContextForegroundSet(&sContext, ClrBlack);
	else
		GrContextForegroundSet(&sContext, ClrWhite);
	GrLineDrawH(&sContext, 30, 50, 220);
	int i;
	short x_position = 31;
	short y_position = 219;
	for(i=0;i<3;i++)
	{
		GrPixelDraw(&sContext, x_position, y_position);
		x_position++;
		y_position--;
	}
	x_position = 31;
	y_position = 221;
	for(i=0;i<3;i++)
	{
		GrPixelDraw(&sContext, x_position, y_position);
		x_position++;
		y_position++;
	}
}

/* n - number of data
 * temp_data - data to display
 * daily - if true, daily graph is drawn
 */
void draw_temperature_graph(int n, float temp_data[], tBoolean daily)
{

	float min;
	float max;
	float mean=0;
	int i;
	for(i=0;i<n;i++)
	{
		if(temp_data[i]!=100)
		{
		 min = temp_data[i];
		 max = temp_data[i];
		 break;
		}
		if(i==6 && !daily)//drawing weekly temp data
		{
			min=20;
			max=30;
		}
		if(i==23)
		{
			min=20;
			max=30;
		}
	}

	//find min and max
	for(i=0;i<n;i++)
	{
		if (temp_data[i] != 100)
		{
			if (temp_data[i] < min)
				min = temp_data[i];
			else if (temp_data[i] > max)
				max = temp_data[i];
		}
	}

	if(min == max)
	{
		min=min-4;
		max=max+4;
	}

	//add margins
	min-=1;
	max+=1;
	int no_data = 0;
	//calculate mean temperature
	for(i=0;i<n;i++)
	{
		if(temp_data[i]!=100)
			mean+=temp_data[i];
		else
			no_data++;
	}
	mean/=n-no_data;

	//calculate step
	float diff = max-min;
	float step = diff/10;

	//draw y axis
	char fullText[100];
	short x_position = 12;//10
	short y_position = 66;
	float step_value = max;

	GrContextFontSet(&sContext, g_pFontCmss12);
	GrContextForegroundSet(&sContext, ClrWhite);
	GrStringDrawCentered(&sContext, "[ C]", -1, 10, 53, 0);
	GrStringDrawCentered(&sContext, "o", -1, 6, 50, 0);

	for(i=0;i<10;i++)
	{
		sprintf(fullText, "%0.2f", step_value);
		GrStringDrawCentered(&sContext, fullText, -1, x_position, y_position, 0);
		step_value-=step;
		y_position+=13;
	}

	//draw x axis
	if(daily)
	{
		x_position = 34;//30
		y_position = 200;
		int hour;
		if (!screen_1b)
			hour = 0;
		else
			hour = 12;
		for (i = 0; i < 12; i++)
		{
			sprintf(fullText, "%d", hour);
			GrStringDrawCentered(&sContext, fullText, -1, x_position,
					y_position, 0);
			x_position += 25;
			hour++;
		}
		//draw x label
		if(!screen_1b)
		{
			draw_or_erase_left_arrow(true);
			draw_or_erase_right_arrow(false);
		}
		else
		{
			draw_or_erase_left_arrow(false);
			draw_or_erase_right_arrow(true);
		}
		GrContextForegroundSet(&sContext, ClrWhite);
		GrContextFontSet(&sContext, g_pFontCmss12b);
		GrStringDrawCentered(&sContext, "Godzina", -1, 160, 210, 0);
		//draw title
		GrStringDrawCentered(&sContext, "Zmiany wartosci sredniej temperatury", -1, 160, 50, 0);

		//draw rectangles
		tRectangle rect;
		rect.sYMax = 194;
		rect.sXMin = 24;//20
		rect.sXMax = 44;//40
		int rect_height;
		float compare_value;
		float temp;
		float i_min = step_value+step;
		int range;
		if (!screen_1b)
		{
			i = 0;
			range = 12;
		}
		else
		{
			i = 12;
			range = 24;

		}
		while (i < range)
		{
			rect_height = -2;	//12 -2: offset
			compare_value = 0;
			if (temp_data[i] != 100)
			{
				temp = temp_data[i];
				while (temp > i_min + compare_value)
				{
					compare_value += step;
					rect_height += 13;
				}
				rect.sYMin = rect.sYMax - rect_height;
				GrRectFill(&sContext, &rect);
			}
			rect.sXMin += 25;
			rect.sXMax += 25;
			i++;
		}
	}
	else
	{
		x_position = 45;//41
		y_position = 200;
		for (i = 0; i < 7; i++)
		{
			GrStringDrawCentered(&sContext, day_of_the_week[i], -1, x_position, y_position, 0);
			x_position += 42;
		}

		if(!screen_1b)
		{
			//draw arrows
			draw_or_erase_left_arrow(true);
			draw_or_erase_right_arrow(false);

			//draw x label
			GrContextFontSet(&sContext, g_pFontCmss12b);
			GrContextForegroundSet(&sContext, ClrWhite);
			GrStringDrawCentered(&sContext, "Dzien tygodnia", -1, 160, 210, 0);
			//draw title
			GrStringDrawCentered(&sContext, "Najwyzsza temperatura danego dnia", -1, 160,50, 0);
		}

		else
		{
			//draw arrows
			draw_or_erase_left_arrow(false);
			draw_or_erase_right_arrow(true);

			//draw x label
			GrContextFontSet(&sContext, g_pFontCmss12b);
			GrContextForegroundSet(&sContext, ClrWhite);
			GrStringDrawCentered(&sContext, "Dzien tygodnia", -1, 160, 210, 0);
			//draw title
			GrStringDrawCentered(&sContext, "Najnizsza temperatura danego dnia", -1, 160,50, 0);
		}

		//draw rectangles
		tRectangle rect;
		rect.sYMax = 194;
		rect.sXMin = 25;//21
		rect.sXMax = 65;//61
		int rect_height;
		int compare_value;
		int temp;
		int i_min = (step_value+step) * 10;
		int range;
		range = 7;
		i=0;
		while (i < range) {
			rect_height = -2;	//12 -2: offset
			compare_value = 0;
			if (temp_data[i] != 100) {
				temp = temp_data[i] * 10;
				while (temp > i_min + compare_value) {
					compare_value += step * 10;
					rect_height += 13;
				}
				if(rect_height == -2)
					rect_height+=13;
				rect.sYMin = rect.sYMax - rect_height;
				GrRectFill(&sContext, &rect);
			}
			rect.sXMin += 42;
			rect.sXMax += 42;
			i++;
		}
	}


}


/* n - number of data
 * temp_data - data to display
 * daily - if true, daily graph is drawn
 */
void draw_humidity_graph(int n, uint32_t humidity_data[], tBoolean daily)
{

	float min;
	float max;
	float mean=0;
	int i;
	for(i=0;i<n;i++)
	{
		if(humidity_data[i]!=100)
		{
		 min = (float)humidity_data[i];
		 max = (float)humidity_data[i];
		 break;
		}
		if(i==6 && !daily)//drawing weekly temp data
		{
			min=20;
			max=30;
		}
		if(i==23)
		{
			min=20;
			max=30;
		}
	}

	//find min and max
	for(i=0;i<n;i++)
	{
		if (humidity_data[i] != 100) {
			if ((float)humidity_data[i] < min)
				min = (float)humidity_data[i];
			else if ((float)humidity_data[i] > max)
				max = (float)humidity_data[i];
		}
	}

	if(min == max)
	{
		min=min-4;
		max=max+4;
	}

	//add margins
	min-=1;
	max+=1;
	int no_data = 0;
	//calculate mean humidity
	for(i=0;i<n;i++)
	{
		if(humidity_data[i]!=100)
			mean+=(float)humidity_data[i];
		else
			no_data++;
	}
	if(n == no_data)
		mean = 0;
	else
		mean/=n-no_data;

	//calculate step
	float diff = max-min;
	float step = diff/10;

	//draw y axis
	char fullText[100];
	short x_position = 10;
	short y_position = 66;
	float step_value = max;

	GrContextFontSet(&sContext, g_pFontCmss12);
	GrContextForegroundSet(&sContext, ClrWhite);
	GrStringDrawCentered(&sContext, "[%]", -1, 10, 53, 0);
	for(i=0;i<10;i++)
	{
		sprintf(fullText, "%0.1f", step_value);
		GrStringDrawCentered(&sContext, fullText, -1, x_position, y_position, 0);
		step_value-=step;
		y_position+=13;
	}

	//draw x axis
	if(daily){
		x_position = 30;
		y_position = 200;
		int hour;
		if (!screen_1b)
			hour = 0;
		else
			hour = 12;
		for (i = 0; i < 12; i++) {
			sprintf(fullText, "%d", hour);
			GrStringDrawCentered(&sContext, fullText, -1, x_position,
					y_position, 0);
			x_position += 25;
			hour++;
		}
		//draw arrows
		if(!screen_1b){
			draw_or_erase_left_arrow(true);
			draw_or_erase_right_arrow(false);
		}
		else
		{
			draw_or_erase_left_arrow(false);
			draw_or_erase_right_arrow(true);
		}
		//draw x label
		GrContextForegroundSet(&sContext, ClrWhite);
		GrContextFontSet(&sContext, g_pFontCmss12b);
		GrStringDrawCentered(&sContext, "Godzina", -1, 160, 210, 0);
		//draw title
		GrStringDrawCentered(&sContext, "Zmiany wartosci sredniej wilgotnosci", -1, 160, 50, 0);

		//draw rectangles
		tRectangle rect;
		rect.sYMax = 194;
		rect.sXMin = 20;
		rect.sXMax = 40;
		int rect_height;
		int compare_value;
		int temp;
		int i_min = (step_value+step) * 10;
		int range;
		if (!screen_1b)
		{
			i = 0;
			range = 12;
		}
		else
		{
			i = 12;
			range = 24;

		}
		while (i < range)
		{
			rect_height = -2;	//12 -2: offset
			compare_value = 0;
			if (humidity_data[i] != 100)
			{
				temp = humidity_data[i] * 10;
				while (temp > i_min + compare_value)
				{
					compare_value += step * 10;
					rect_height += 13;
				}
				if(rect_height == -2)
					rect_height+=13;
				rect.sYMin = rect.sYMax - rect_height;
				GrRectFill(&sContext, &rect);
			}
			rect.sXMin += 25;
			rect.sXMax += 25;
			i++;
		}
	}
	else
	{
		x_position = 41;
		y_position = 200;

		for (i = 0; i < 7; i++) {
			GrStringDrawCentered(&sContext, day_of_the_week[i], -1, x_position, y_position, 0);
			x_position += 42;

		}
		//draw x label
		GrContextForegroundSet(&sContext, ClrWhite);
		GrContextFontSet(&sContext, g_pFontCmss12b);
		GrStringDrawCentered(&sContext, "Dzien tygodnia", -1, 160, 210, 0);
		//draw title
		GrStringDrawCentered(&sContext, "Srednia wilgotnosc danego dnia", -1, 160,
				50, 0);

		//draw rectangles
		tRectangle rect;
		rect.sYMax = 194;
		rect.sXMin = 21;
		rect.sXMax = 61;
		int rect_height;
		int compare_value;
		int temp;
		int i_min = (step_value+step) * 10;
		int range;
		range = 7;
		i=0;
		while (i < range) {
			rect_height = -2;	//12 -2: offset
			compare_value = 0;
			if (humidity_data[i] != 100) {
				temp = humidity_data[i] * 10;
				while (temp > i_min + compare_value) {
					compare_value += step * 10;
					rect_height += 13;
				}
				if(rect_height == -2)
					rect_height+=13;
				rect.sYMin = rect.sYMax - rect_height;
				GrRectFill(&sContext, &rect);
			}
			rect.sXMin += 42;
			rect.sXMax += 42;
			i++;
		}
	}
}


//This function is used to clear last day temperature data, when the new day starts

void clear_daily_temp_data()
{
	int i;
	//leave 0:00 temp
	for (i = 1; i < 24; i++) {
		daily_temp[i] = 100;
		daily_temp_time[i].hours = 0;
		daily_temp_time[i].day = 0;
		daily_temp_time[i].year = 0;
	}
}



//This function is used to clear last day humidity data, when the new day starts

void clear_daily_humidity_data()
{
	int i;
	//leave 0:00 temp
	for (i = 1; i < 24; i++) {
		daily_humidity[i] = 100;
		daily_humidity_time[i].hours = 0;
		daily_humidity_time[i].day = 0;
		daily_humidity_time[i].year = 0;
	}
}


//This function is used every hour to update daily humidity data

void update_daily_humidity_data(uint8_t day,uint8_t hour,uint8_t month, uint8_t year)
{
	last_hour_humidity = last_hour_humidity / humidity_measurements_counter;
	daily_humidity[displayed_data.hours] = last_hour_humidity;
	daily_humidity_time[displayed_data.hours].hours = hour;
	daily_humidity_time[displayed_data.hours].day = day;
	daily_humidity_time[displayed_data.hours].month = month;
	daily_humidity_time[displayed_data.hours].year = year;
	last_hour_humidity = displayed_data.humidity;
	humidity_measurements_counter = 1;

	if (displayed_data.hours == 0)
	{
		clear_daily_humidity_data();
		save_temp_or_humidity(7, 24, daily_humidity, daily_humidity_time, true);
	}
	else
	{
		save_temp_or_humidity(7, 24, daily_humidity, daily_humidity_time, true);
	}
}



//This function is used every hour to update daily temperature data

void update_daily_temp_data(uint8_t day,uint8_t hour,uint8_t month, uint8_t year)
{
	last_hour_temp = last_hour_temp / temp_measurements_counter;
	daily_temp[displayed_data.hours] = last_hour_temp;
	daily_temp_time[displayed_data.hours].hours = hour;
	daily_temp_time[displayed_data.hours].day = day;
	daily_temp_time[displayed_data.hours].month = month;
	daily_temp_time[displayed_data.hours].year = year;
	last_hour_temp = 0;
	temp_measurements_counter = 0;

	if(displayed_data.hours == 0)
	{
		//clear daily_temp_data
		clear_daily_temp_data();
		prepare_temp_to_save(daily_temp,daily_temp_to_save, 24);
		save_temp_or_humidity(3, 24, daily_temp_to_save, daily_temp_time,true);
	}
	else
	{
		prepare_temp_to_save(daily_temp,daily_temp_to_save, 24);
		save_temp_or_humidity(3, 24, daily_temp_to_save, daily_temp_time, true);
	}

}



// This function is used at the begining of the new day to update max temperature data

void update_weekly_max_temp_data()
{
	float temp = 100;
	int i;
	//find first measurement of the day
	for(i=0;i<24;i++)
	{
		if(daily_temp[i]!=100)
		{
			temp = daily_temp[i];
			break;
		}
	}
	//find max or min temp
	for(i=0;i<24;i++)
	{
		if(daily_temp[i]!=100 && daily_temp[i]>temp)
		{
			temp = daily_temp[i];
		}
	}

	//dow-1 because in weekly_temp array saturday is saved under index 0 not 1
	weekly_max_temp[displayed_data.dow-1] = temp;
	weekly_max_temp_time[displayed_data.dow-1].day = displayed_data.day;
	weekly_max_temp_time[displayed_data.dow-1].dow = displayed_data.dow-1;//
	weekly_max_temp_time[displayed_data.dow-1].month = displayed_data.month;
	weekly_max_temp_time[displayed_data.dow-1].year = displayed_data.year;
	prepare_temp_to_save(weekly_max_temp,weekly_temp_to_save,7);
	save_temp_or_humidity(5,7,weekly_temp_to_save,weekly_max_temp_time,false);
}



//This function is used at the begining of the new day to update min temperature data

void update_weekly_min_temp_data()
{
	float temp = 100;
	int i;
	//find first measurement of the day
	for(i=0;i<24;i++)
	{
		if(daily_temp[i]!=100)
		{
			temp = daily_temp[i];
			break;
		}
	}
	//find max or min temp
	for(i=0;i<24;i++)
	{
		if(daily_temp[i]!=100 && daily_temp[i]<temp)
		{
			temp = daily_temp[i];
		}
	}
	//dow-1 because in weekly_temp array saturday is saved under index 0 not 1
	weekly_min_temp[displayed_data.dow-1] = temp;
	weekly_min_temp_time[displayed_data.dow-1].day = displayed_data.day;
	weekly_min_temp_time[displayed_data.dow-1].dow = displayed_data.dow-1;//
	weekly_min_temp_time[displayed_data.dow-1].month = displayed_data.month;
	weekly_min_temp_time[displayed_data.dow-1].year = displayed_data.year;
	prepare_temp_to_save(weekly_min_temp,weekly_temp_to_save,7);
	save_temp_or_humidity(11,7,weekly_temp_to_save,weekly_min_temp_time,false);

}



//This function is used at the begining of the new day to update mean humidity data

void update_weekly_humidity_data()
{
	uint32_t mean_humidity = 0;
	int i;
	int counter = 0;
	for(i=0;i<24;i++)
	{
		if(daily_humidity[i]!=100)
		{
			mean_humidity+=daily_humidity[i];
			counter++;
		}
	}
	if(counter!=0)
		mean_humidity/=counter;
	else
		mean_humidity = 100;
	//dow-1 because in weekly_temp array saturday is saved under index 0 not 1
	weekly_humidity[displayed_data.dow-1] = mean_humidity;
	weekly_humidity_time[displayed_data.dow-1].day = displayed_data.day;
	weekly_humidity_time[displayed_data.dow-1].dow = displayed_data.dow-1;
	weekly_humidity_time[displayed_data.dow-1].month = displayed_data.month;
	weekly_humidity_time[displayed_data.dow-1].year = displayed_data.year;
	save_temp_or_humidity(9,7,weekly_humidity,weekly_humidity_time,false);

}


void clear_daily_graph()
{
	tRectangle rect;
	rect.sYMin = 61;
	rect.sYMax = 194;
	rect.sXMin = 20;
	rect.sXMax = 320;
	GrContextForegroundSet(&sContext, ClrBlack);
	GrRectFill(&sContext, &rect);
}


void clear_weekly_humidity_data()
{
	int i;
	for(i=0;i<7;i++)
	{
		weekly_humidity[i] = 100;
		weekly_humidity_time[i].day = 0;
		weekly_humidity_time[i].dow = 0;
		weekly_humidity_time[i].month = 0;
		weekly_humidity_time[i].year = 0;
	}
}


void clear_weekly_temp_data()
{
	int i;
	for(i=0;i<7;i++)
	{
		weekly_max_temp[i] = 100;
		weekly_max_temp_time[i].day = 0;
		weekly_max_temp_time[i].dow = 0;
		weekly_max_temp_time[i].month = 0;
		weekly_max_temp_time[i].year = 0;

		weekly_min_temp[i] = 100;
		weekly_min_temp_time[i].day = 0;
		weekly_min_temp_time[i].dow = 0;
		weekly_min_temp_time[i].month = 0;
		weekly_min_temp_time[i].year = 0;

	}
}


tBoolean check_day(uint8_t current_time, uint8_t past_time)
{
	if(current_time-past_time <= 7)
		return true;
	else
		return false;
}


tBoolean check_dow(uint8_t current_dow, uint8_t past_dow)
{
	if(current_dow == 1)
		current_dow = 7;
	if(past_dow<current_dow)
		return true;
	else
		return false;
}



//This function is used to check if the loaded weekly temperature data is from the current week

void check_loaded_weekly_max_temp(float *weekly_temp, RTC_Time *weekly_temp_time)
{
	int i;
	float temp;
	float f_loaded_weekly_temp[7];
	for(i=0;i<7;i++)
	{
		temp = (float)loaded_weekly_max_temp[i]/10;
		f_loaded_weekly_temp[i] = temp;
		if(loaded_weekly_max_temp_time[i].year == displayed_data.year && loaded_weekly_max_temp_time[i].month == displayed_data.month)
		{
			if(check_day(displayed_data.day, loaded_weekly_max_temp_time[i].day) &&
					check_dow(displayed_data.dow, loaded_weekly_max_temp_time[i].dow))
			{
				weekly_temp[loaded_weekly_max_temp_time[i].dow] =
						f_loaded_weekly_temp[i];
				weekly_temp_time[loaded_weekly_max_temp_time[i].dow].day =
						loaded_weekly_max_temp_time[i].day;
				weekly_temp_time[loaded_weekly_max_temp_time[i].dow].dow =
						loaded_weekly_max_temp_time[i].dow;
				weekly_temp_time[loaded_weekly_max_temp_time[i].dow].month =
						loaded_weekly_max_temp_time[i].month;
				weekly_temp_time[loaded_weekly_max_temp_time[i].dow].year =
						loaded_weekly_max_temp_time[i].year;
			}
		}
	}
}



//This function is used to check if the loaded weekly min temperature data is from the current week

void check_loaded_weekly_min_temp(float *weekly_temp, RTC_Time *weekly_temp_time)
{
	int i;
	float temp;
	float f_loaded_weekly_temp[7];
	for(i=0;i<7;i++)
	{
		temp = (float)loaded_weekly_min_temp[i]/10;
		f_loaded_weekly_temp[i] = temp;
		if(loaded_weekly_min_temp_time[i].year == displayed_data.year && loaded_weekly_min_temp_time[i].month == displayed_data.month)
		{
			if(check_day(displayed_data.day, loaded_weekly_min_temp_time[i].day) &&
					check_dow(displayed_data.dow, loaded_weekly_min_temp_time[i].dow))
			{
				weekly_temp[loaded_weekly_min_temp_time[i].dow] =
						f_loaded_weekly_temp[i];
				weekly_temp_time[loaded_weekly_min_temp_time[i].dow].day =
						loaded_weekly_min_temp_time[i].day;
				weekly_temp_time[loaded_weekly_min_temp_time[i].dow].dow =
						loaded_weekly_min_temp_time[i].dow;
				weekly_temp_time[loaded_weekly_min_temp_time[i].dow].month =
						loaded_weekly_min_temp_time[i].month;
				weekly_temp_time[loaded_weekly_min_temp_time[i].dow].year =
						loaded_weekly_min_temp_time[i].year;
			}
		}
	}
}


// This function is used to check if the loaded weekly humidity data is from the current week

void check_loaded_weekly_humidity()
{
	int i;
	for(i=0;i<7;i++)
	{
		if(loaded_weekly_humidity_time[i].year == displayed_data.year && loaded_weekly_humidity_time[i].month == displayed_data.month)
		{
			if(check_day(displayed_data.day, loaded_weekly_humidity_time[i].day)&&
					check_dow(displayed_data.dow, loaded_weekly_humidity_time[i].dow))
			{
				weekly_humidity[loaded_weekly_humidity_time[i].dow] =
						loaded_weekly_humidity[i];
				weekly_humidity_time[loaded_weekly_humidity_time[i].dow].day =
						loaded_weekly_humidity_time[i].day;
				weekly_humidity_time[loaded_weekly_humidity_time[i].dow].dow =
						loaded_weekly_humidity_time[i].dow;
				weekly_humidity_time[loaded_weekly_humidity_time[i].dow].month =
						loaded_weekly_humidity_time[i].month;
				weekly_humidity_time[loaded_weekly_humidity_time[i].dow].year =
						loaded_weekly_humidity_time[i].year;
			}
		}
	}
}


void init_data()
{
	displayed_data.bme280_empty = true;
	displayed_data.time_empty = true;

	//init pressure_history_data_time
	int i;
	for (i = 0; i < 13; i++)
	{
		pressure_history_data_time[i].hours = 0;
		pressure_history_data_time[i].day = 0;
		pressure_history_data_time[i].month = 0;

	}
	//clear_daily_humidity_data
	for (i = 0; i < 24; i++)
	{
		daily_temp[i] = 100;
		daily_temp_time[i].hours = 0;
		daily_temp_time[i].day = 0;
		daily_temp_time[i].year = 0;
	}

	//clear_daily_humidity_data
	for (i = 0; i < 24; i++)
	{
		daily_humidity[i] = 100;
		daily_humidity_time[i].hours = 0;
		daily_humidity_time[i].day = 0;
		daily_humidity_time[i].year = 0;
	}

	clear_weekly_temp_data();
	clear_weekly_humidity_data();
}


void clear_loaded_data()
{
	int i;
	for(i=0;i<13;i++)
	{
		pressure_history_loaded_data[i]=0;
		pressure_history_loaded_data_time[i]=0;
	}
}


/*
 * block_addr - Address of the block where the data_array will be saved
 * n - Number of bytes to save
 * data_array - data to be saved
 * time_array - data to be saved
 */
void save_pressure_history_data(unsigned int block_addr, int n, uint32_t data_array[], RTC_Time time_array[])
{
	//unsigned int block_addr = 1;
	unsigned char buffer[511] = {0};

	//saving pressure values
	unsigned char data_byte = 0;
	int i;
	int data_counter = 0;
	int shift = 24;
	for (i = 0; i < n; i++)
	{
		data_byte = data_array[data_counter] >> shift;
		shift -= 8;
		buffer[i] = data_byte;
		if (shift < 0)
		{
			shift = 24;
			data_counter++;
		}
	}
	writeSingleBlock(buffer, block_addr*512);
	block_addr++;

	 //saving time values
	time_t t_of_day[13];
	for(i=0;i<13;i++)
	{
		struct tm time;
		time.tm_year = (time_array[i].year+2000)-1900;
		time.tm_mon = time_array[i].month-1;
		time.tm_mday = time_array[i].day;
		time.tm_hour = time_array[i].hours;
		time.tm_min = 0;
		time.tm_sec = 0;
		t_of_day[i] = mktime(&time);
	}
	unsigned char time_byte = 0;
	data_counter = 0;
	shift = 24;
	for (i = 0; i < n; i++)
	{
			time_byte = t_of_day[data_counter] >> shift;
			shift -= 8;
			buffer[i] = time_byte;
			if (shift < 0)
			{
				shift = 24;
				data_counter++;
			}
		}
	writeSingleBlock(buffer, block_addr*512);

}


/*
 * block_addr - Address of the block where the data was saved
 * n - Number of bytes to load
 * data_array - pointer to the array where the loaded data will be stored
 * time_array - pointer to the array where the loaded data will be stored
 */
void load_pressure_history_data(unsigned int block_addr, int n, uint32_t *data_array, uint32_t *time_array)
{
	int rslt;
	unsigned char buffer[511];
	rslt = readSingleBlock(buffer, block_addr * 512);
	if (rslt == 0) {

		//read pressure values
		int i;
		int data_counter = 0;
		int shift = 24;
		for (i = 0; i < n; i++) {
			data_array[data_counter] |= buffer[i] << shift;
			shift -= 8;
			if (shift < 0) {
				data_counter++;
				shift = 24;
			}
		}
		//read time
		block_addr++;
		readSingleBlock(buffer, block_addr * 512);
		data_counter = 0;
		shift = 24;
		for (i = 0; i < n; i++)
		{
			time_array[data_counter] |= buffer[i] << shift;
			shift -= 8;
			if (shift < 0)
			{
				data_counter++;
				shift = 24;
			}
		}
	}
}



//returns time difference in seconds

int compare_time(uint32_t current_time , uint32_t past_time)
{
	int diff = current_time - past_time;
	return diff;
}


//This function checks if the loaded data contains pressure values from the last 12 hours

void check_loaded_pressure_history_data()
{
	struct tm time;
	time.tm_year = (displayed_data.year + 2000) - 1900;
	time.tm_mon = displayed_data.month - 1;
	time.tm_mday = displayed_data.day;
	time.tm_hour = displayed_data.hours;
	time.tm_min = 0;
	time.tm_sec = 0;
	time.tm_isdst - 1;
	time_t current_time = mktime(&time);
	int i;
	int diff;
	for(i=0;i<13;i++)
	{
		diff = compare_time(current_time,pressure_history_loaded_data_time[i]);
		if(diff/3600 >= 0 && diff/3600 <= 12)
		{
			pressure_history_data[diff/3600] = pressure_history_loaded_data[i];
			struct tm  ts;
			ts = *localtime(&pressure_history_loaded_data_time[i]);
			pressure_history_data_time[diff/3600].hours = ts.tm_hour;
			pressure_history_data_time[diff/3600].day = ts.tm_mday;
			pressure_history_data_time[diff/3600].month = ts.tm_mon+1;
			pressure_history_data_time[diff/3600].year = ts.tm_year-100;
			pressure_history_data_time[diff/3600].minutes = 0;
			pressure_history_data_time[diff/3600].seconds = 0;
		}
	}
}


//This function is used every hour to update pressure_history_data array

void update_pressure_history_data()
{
	int i;
	for(i=12;i>0;i--)
	{
		pressure_history_data[i] = pressure_history_data[i-1];
		pressure_history_data_time[i] = pressure_history_data_time[i-1];
	}
	pressure_history_data[0] = displayed_data.pressure;
	pressure_history_data_time[0].hours = displayed_data.hours;
	pressure_history_data_time[0].day = displayed_data.day;
	pressure_history_data_time[0].month = displayed_data.month;
	pressure_history_data_time[0].year = displayed_data.year;
	save_pressure_history_data(1,52,pressure_history_data,pressure_history_data_time);
}


void clear_pressure_history_graph()
{
	tRectangle eraser;
	eraser.sXMin = 0;
	eraser.sXMax = 320;
	eraser.sYMin = 105;
	eraser.sYMax = 220;
	GrContextForegroundSet(&sContext, ClrBlack);
	GrRectFill(&sContext, &eraser);
}

void draw_pressure_history_graph()
{
	clear_pressure_history_graph();
	uint32_t min;
	uint32_t max;
	float mean=0;
	int i;
	for(i=1;i<12;i++)
	{
		if(pressure_history_data[i]!=0)
		{
		 min = pressure_history_data[i];
		 max = pressure_history_data[i];
		 break;
		}
		if(i==11)
		{
			min=1000;
			max=1007;
		}
	}
	//find min and max
	for(i=1;i<13;i++)
	{
		if (pressure_history_data[i] != 0) {
			if (pressure_history_data[i] < min)
				min = pressure_history_data[i];
			else if (pressure_history_data[i] > max)
				max = pressure_history_data[i];
		}
	}

	if(min == max)
	{
		min=min-3;
		max=max+3;
	}

	int no_data = 0;
	//calculate mean pressure
	for(i=1;i<13;i++)
	{
		if(pressure_history_data[i]!=0)
			mean+=(float)pressure_history_data[i];
		else
			no_data++;
	}
	mean/=12-no_data;

	//calculate step
	uint32_t diff = max-min;
	float step = diff/7;
	if(step<1)
		step = 1;
	else if(step > 1 && step<2)
		step = 2;
	else if(step >2 && step <3)
		step = 3;
	else
		step =4;

	//draw title
	GrContextFontSet(&sContext, g_pFontCmss12b);
	GrContextForegroundSet(&sContext, ClrWhite);
	GrStringDrawCentered(&sContext, "Historia cisnienia", -1, 160, 95, 0);
	GrContextFontSet(&sContext, g_pFontCmss12);
	//draw y axis
	char fullText[100];
	short x_position = 13;
	short y_position = 112;
	uint32_t step_value = max;

	GrStringDrawCentered(&sContext, "[hPa]", -1, x_position, 98, 0);
	for(i=0;i<7;i++)
	{
		sprintf(fullText, "%d", step_value);
		GrStringDrawCentered(&sContext, fullText, -1, x_position, y_position, 0);
		step_value-=step;
		y_position+=14;
	}

	//draw x axis
	x_position = 42;
	y_position = 210;
	int hour = -12;

	for (i = 0; i < 12; i++)
	{
		sprintf(fullText, "%dh", hour);
		GrStringDrawCentered(&sContext, fullText, -1, x_position, y_position,0);
		x_position += 24;
		hour++;
	}

	tRectangle rect;
	rect.sYMax = 204;
	rect.sXMin = 31;
	rect.sXMax = 53;
	int rect_height;
	int compare_value;
	i = 12;
	while (i > 0)
	{
		compare_value = 0;
		rect_height = 10;
		if (pressure_history_data[i] != 0)
		{

			while (pressure_history_data[i] > (step_value+step) + compare_value)
			{
				compare_value += step;
				rect_height += 14;
			}
			rect.sYMin = rect.sYMax - rect_height;
			GrRectFill(&sContext, &rect);
		}
		rect.sXMin += 24;
		rect.sXMax += 24;
		i--;
	}


	pressure_graph_ready = true;
}



//This function erases outdated sensor data from the display

void erase_outdated_sensor_data()
{
	char fullText[100];
	//temperature
	sprintf(fullText, "%0.1fC", displayed_data.temperature);
	GrContextFontSet(&sContext, g_pFontCmss28b);
	GrContextForegroundSet(&sContext, ClrBlack);
	GrStringDrawCentered(&sContext, fullText, -1, 43, 45, 0);
	GrContextFontSet(&sContext, g_pFontCmss12);
	GrStringDrawCentered(&sContext, "o", -1, 58, 35, 0);

	//humidity
	sprintf(fullText, "%d%", displayed_data.humidity);
	GrContextFontSet(&sContext, g_pFontCmss28b);
	GrContextForegroundSet(&sContext, ClrBlack);
	GrStringDrawCentered(&sContext, fullText, -1, 265, 45, 0);
	GrStringDrawCentered(&sContext, "%", -1, 295 ,45, 0);

	//pressure
	sprintf(fullText, "%dhPa", displayed_data.pressure);
	GrContextFontSet(&sContext, g_pFontCmss18b);
	GrContextForegroundSet(&sContext, ClrBlack);
	GrStringDrawCentered(&sContext, fullText, -1, 160, 227, 0);

}



// This function erases outdated time from the display

void erase_outdated_time()
{
	char fullText[100];

	sprintf(fullText, "%02d:%02d", displayed_data.hours, displayed_data.minutes);
	GrContextFontSet(&sContext, g_pFontCmss26b);
	GrContextForegroundSet(&sContext, ClrBlack);
	GrStringDrawCentered(&sContext, fullText, -1, 160,25, 0);//30
}



//This function erases outdated date from the display

void erase_outdated_date()
{
	char fullText[100];

	sprintf(fullText, "%02d.%02d.%d", displayed_data.day, displayed_data.month,
			displayed_data.year + 2000);
	GrContextFontSet(&sContext, g_pFontCmss16b);
	GrContextForegroundSet(&sContext, ClrBlack);
	GrStringDrawCentered(&sContext, fullText, -1, 160, 50, 0);//65

	GrStringDrawCentered(&sContext, day_of_the_week[displayed_data.dow-1], -1,
			160, 70, 0);//85

}



//This function draws current temperature, humidity and pressure values on the display

void draw_sensor_data(float temp, uint32_t pressure, uint32_t humidity)
{
	char fullText[100];

    if(!displayed_data.bme280_empty && screen == 0)
    	erase_outdated_sensor_data();

	//temperature
	if (screen == 0) {
		sprintf(fullText, "%0.1fC", temp);
		GrContextFontSet(&sContext, g_pFontCmss28b);
		GrContextForegroundSet(&sContext, ClrGold);
		GrStringDrawCentered(&sContext, fullText, -1, 43, 45, 0);
		GrContextFontSet(&sContext, g_pFontCmss12);
		GrStringDrawCentered(&sContext, "o", -1, 60, 35, 0);

		//humidity
		sprintf(fullText, "%d", humidity);
		GrContextFontSet(&sContext, g_pFontCmss28b);
		GrContextForegroundSet(&sContext, ClrBlue);
		GrStringDrawCentered(&sContext, fullText, -1, 265, 45, 0);
		GrStringDrawCentered(&sContext, "%", -1, 295, 45, 0);

		//pressure
		sprintf(fullText, "%dhPa", pressure);
		GrContextFontSet(&sContext, g_pFontCmss18b);
		GrContextForegroundSet(&sContext, ClrWhite);
		GrStringDrawCentered(&sContext, fullText, -1, 160, 227, 0);
	}

	displayed_data.temperature = temp;
	if(displayed_data.pressure != pressure || displayed_data.bme280_empty)
	{
		displayed_data.pressure = pressure;
		//change current pressure value
		pressure_history_data[0] = pressure;
		//save when it was measured
		pressure_history_data_time[0].hours = displayed_data.hours;
		pressure_history_data_time[0].day = displayed_data.day;
		pressure_history_data_time[0].month = displayed_data.month;
		pressure_history_data_time[0].year = displayed_data.year;

		if(phd_save_locked == false)
		{
			//save new data on microSD card
			save_pressure_history_data(1,52,pressure_history_data,pressure_history_data_time);
		}
	}
	displayed_data.humidity = humidity;

	if(displayed_data.bme280_empty)
		displayed_data.bme280_empty = false;



}



//This function displays current time and checks if an hour has passed

void check_time(RTC_Time *time)
{
	if ((time->minutes != displayed_data.minutes || screen_changed) && screen == 0) {
		if (!displayed_data.time_empty)
			erase_outdated_time();

		char fullText[100];
		//check which screen is active
		if (screen == 0) {
			sprintf(fullText, "%02d:%02d", time->hours, time->minutes);
			GrContextFontSet(&sContext, g_pFontCmss26b);
			GrContextForegroundSet(&sContext, ClrWhite);
			GrStringDrawCentered(&sContext, fullText, -1, 160, 25, 0);
			sprintf(fullText, "%02d.%02d.%d", time->day, time->month,
					time->year + 2000);
			GrContextFontSet(&sContext, g_pFontCmss16b);
			GrStringDrawCentered(&sContext, fullText, -1, 160, 50, 0);

			GrStringDrawCentered(&sContext, day_of_the_week[time->dow - 1], -1,
					160, 70, 0);
			if (time->day != displayed_data.day ) {
				if (!displayed_data.time_empty)
					erase_outdated_date();
				sprintf(fullText, "%02d.%02d.%d", time->day, time->month,
						time->year + 2000);
				GrContextFontSet(&sContext, g_pFontCmss16b);
				GrContextForegroundSet(&sContext, ClrWhite);
				GrStringDrawCentered(&sContext, fullText, -1, 160, 50, 0);

				GrStringDrawCentered(&sContext, day_of_the_week[time->dow - 1],
						-1, 160, 70, 0);
			}
		}

		if (displayed_data.day != time->day && !displayed_data.time_empty) {

			if(time->dow == 2)
			{
				clear_weekly_temp_data();
				clear_weekly_humidity_data();

			}
			update_weekly_max_temp_data();
			update_weekly_min_temp_data();
			update_weekly_humidity_data();

			displayed_data.day = time->day;
			displayed_data.dow = time->dow;


		}

		if (displayed_data.hours != time->hours && !displayed_data.time_empty)
		{

			displayed_data.hours = time->hours;
			displayed_data.minutes = time->minutes;
			displayed_data.month = time->month;
			displayed_data.year = time->year;

			update_daily_temp_data(time->day,time->hours,time->month,time->year);
			update_daily_humidity_data(time->day,time->hours,time->month,time->year);
			update_pressure_history_data();
			if(screen == 0)
			{
				draw_pressure_history_graph();
			}
			else if(screen == 1 && daily_flag)
			{
				clear_daily_graph();
				draw_temperature_graph(24,daily_temp,true);
			}
			else if(screen == 2 && daily_flag)
			{
				clear_daily_graph();
				draw_humidity_graph(24,daily_humidity,true);
			}

		}
		else
		{
			displayed_data.hours = time->hours;
			displayed_data.minutes = time->minutes;
			displayed_data.day = time->day;
			displayed_data.month = time->month;
			displayed_data.year = time->year;
			displayed_data.dow = time->dow;
		}

		if (displayed_data.time_empty)
			displayed_data.time_empty = false;
	}
}


void print_sensor_data(struct bme280_data *comp_data)
{

	float temp = (float) comp_data->temperature / 100;
	uint32_t pressure = (comp_data->pressure / 10000) + 8;//adding 8 to calibrate the sensor
	uint32_t humidity = comp_data->humidity / 1024;
	if (temp != displayed_data.temperature
			|| pressure != displayed_data.pressure
			|| humidity != displayed_data.humidity
			|| displayed_data.bme280_empty
			|| screen_changed)
	{
		draw_sensor_data(temp, pressure, humidity);
	}

}



// This function gets current temperature, pressure and humidity data from the BME280

int8_t stream_sensor_data_forced_mode(struct bme280_dev *dev)
{
	int8_t rslt;
	uint8_t settings_sel;
	uint32_t req_delay;
	struct bme280_data comp_data;
	//configure sensor if it wasn't configured before
	if (!config)
	{

		/* Recommended mode of operation: Indoor navigation */
		dev->settings.osr_h = BME280_OVERSAMPLING_2X;
		dev->settings.osr_p = BME280_OVERSAMPLING_16X;
		dev->settings.osr_t = BME280_OVERSAMPLING_2X;
		dev->settings.filter = BME280_FILTER_COEFF_16;

		settings_sel = BME280_OSR_PRESS_SEL | BME280_OSR_TEMP_SEL
				| BME280_OSR_HUM_SEL | BME280_FILTER_SEL;

		rslt = bme280_set_sensor_settings(settings_sel, dev);

		/*Calculate the minimum delay required between consecutive measurement based upon the sensor enabled
		 *  and the oversampling configuration. */
		req_delay = bme280_cal_meas_delay(&dev->settings);

		config = true;
	}

	rslt = bme280_set_sensor_mode(BME280_FORCED_MODE, dev);
	/* Wait for the measurement to complete and print data @25Hz */
	dev->delay_us(req_delay, dev->intf_ptr);
	rslt = bme280_get_sensor_data(BME280_ALL, &comp_data, dev);
	if (!first_measurement)
		print_sensor_data(&comp_data);
	else
		first_measurement = false;

	return rslt;
}




// delay used in i2c communication

void delay_ms(uint32_t period, void *intf_ptr)
{
	if(period>=1000)
	{
		unsigned long ulCount = (period/1000)*6667;
		SysCtlDelay(ulCount);
	}

	else SysCtlDelay(6667);//1ms delay was 13334
}



//Function used to write data to the BME280 registers

int8_t i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    int8_t rslt = 0; /* Return 0 for Success, non-zero for failure */


    while(I2CMasterBusy(I2C1_MASTER_BASE)){}
    I2CMasterSlaveAddrSet(I2C1_MASTER_BASE, BME280_I2C_ADDR_PRIM, false);
    //Sending reg_addr
    I2CMasterDataPut(I2C1_MASTER_BASE, reg_addr);
    I2CMasterControl(I2C1_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_START);
    while(I2CMasterBusy(I2C1_MASTER_BASE)) {} //wait till transmission is done
    //check if transmission was successful
    if(I2CMasterErr(I2C1_MASTER_BASE) != I2C_MASTER_ERR_NONE)
      {
        return 1;
      }

    unsigned long i;
    for(i=0;i<len;i++)
    {
    	if(i==len-1)
    	{
    		I2CMasterDataPut(I2C1_MASTER_BASE, reg_data[i]);
    		I2CMasterControl(I2C1_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);
    		while(I2CMasterBusy(I2C1_MASTER_BASE)) {}
    		 if(I2CMasterErr(I2C1_MASTER_BASE) != I2C_MASTER_ERR_NONE)
    		 {
    		   	return 1;
    		 }
    	}
    	else
    	{
    		I2CMasterDataPut(I2C1_MASTER_BASE, reg_data[i]);
    		I2CMasterControl(I2C1_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_CONT);
    		while(I2CMasterBusy(I2C1_MASTER_BASE)) {}
    		 if(I2CMasterErr(I2C1_MASTER_BASE) != I2C_MASTER_ERR_NONE)
    		 {
    		  	return 1;
    		 }
    	}
    }

    return rslt;
}



//Function used to read data from the BME280 registers

int8_t i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
    {

        uint8_t rslt = 0; /* Return 0 for Success, non-zero for failure */
        unsigned long i;

        I2CMasterSlaveAddrSet(I2C1_MASTER_BASE, BME280_I2C_ADDR_PRIM, false); //write mode
        I2CMasterDataPut(I2C1_MASTER_BASE, reg_addr);
        I2CMasterControl(I2C1_MASTER_BASE, I2C_MASTER_CMD_SINGLE_SEND);
        while(I2CMasterBusy(I2C1_MASTER_BASE)){}//wait till transmission is done
        //check if transmission was successful
        if(I2CMasterErr(I2C1_MASTER_BASE) != I2C_MASTER_ERR_NONE)
            {
        		return 1;//transmission failed
            }

        I2CMasterSlaveAddrSet(I2C1_MASTER_BASE, BME280_I2C_ADDR_PRIM, true);//read mode
        //Start reading
        for(i=0;i<len;i++)
        {
        	//If there is only one byte to read
        	if(len==1)
        	{
        		I2CMasterControl(I2C1_MASTER_BASE, I2C_MASTER_CMD_SINGLE_RECEIVE);
        		while(I2CMasterBusy(I2C1_MASTER_BASE)){}
        		if(I2CMasterErr(I2C1_MASTER_BASE) != I2C_MASTER_ERR_NONE)
        		   {
        		     return 1;
        		   }
        		reg_data[i] = I2CMasterDataGet(I2C1_MASTER_BASE);
        		break;
        	}
        	//If there is more than one byte to read
        	if(i==0)
        	{
        		I2CMasterControl(I2C1_MASTER_BASE, I2C_MASTER_CMD_BURST_RECEIVE_START);
        		while(I2CMasterBusy(I2C1_MASTER_BASE)){}
        		if(I2CMasterErr(I2C1_MASTER_BASE) != I2C_MASTER_ERR_NONE)
        		  {
        			return 1;
        		  }
        		  reg_data[i] = I2CMasterDataGet(I2C1_MASTER_BASE);
        	}
        	//Stop reading
        	else if(i==len-1)
        	{
        		I2CMasterControl(I2C1_MASTER_BASE, I2C_MASTER_CMD_BURST_RECEIVE_FINISH);
        		while(I2CMasterBusy(I2C1_MASTER_BASE)){}
        		if(I2CMasterErr(I2C1_MASTER_BASE) != I2C_MASTER_ERR_NONE)
        		  {
        			return 1;
        		  }
        		  reg_data[i] = I2CMasterDataGet(I2C1_MASTER_BASE);
        	}
        	//Continue reading
        	else
        	{
        		I2CMasterControl(I2C1_MASTER_BASE, I2C_MASTER_CMD_BURST_RECEIVE_CONT);
        		while(I2CMasterBusy(I2C1_MASTER_BASE)){}
        		if(I2CMasterErr(I2C1_MASTER_BASE) != I2C_MASTER_ERR_NONE)
        		  {
        			return 1;
        		  }
        		  reg_data[i] = I2CMasterDataGet(I2C1_MASTER_BASE);
        	}
        }
        return rslt;
    }



void draw_pressure_info()
{
	clear_pressure_history_graph();

	GrContextForegroundSet(&sContext, ClrWhite);
	GrContextFontSet(&sContext, g_pFontCmss16b);
	uint32_t pressure_min_index = 100;
	uint32_t pressure_max_index = 100;

	int i;
	for(i=1;i<13;i++)
	{
		if(pressure_history_data[i]!=0)
		{
			pressure_min_index = i;
			pressure_max_index = i;
			break;
		}

	}

	//find min and max

	for(i=1;i<13;i++)
	{
		if(pressure_history_data[i]<pressure_history_data[pressure_min_index] && pressure_history_data[i]!=0)
			pressure_min_index = i;
		if(pressure_history_data[i]>pressure_history_data[pressure_max_index] && pressure_history_data[i]!=0)
			pressure_max_index = i;
	}

	if(pressure_min_index !=100)
	{

		char fullText[100];
		//min
		sprintf(fullText, "%d", pressure_history_data[pressure_min_index]);
		GrStringDrawCentered(&sContext, "Najnizsze odnotowane cisnienie: ", -1, 160, 120, 0);
		GrStringDrawCentered(&sContext, fullText, -1, 70, 140, 0);
		GrStringDrawCentered(&sContext, "[hPa] ", -1, 110, 140, 0);
		GrStringDrawCentered(&sContext, "o godzinie: ", -1, 170, 140, 0);
		sprintf(fullText, "%02d:00", pressure_history_data_time[pressure_min_index].hours);
		GrStringDrawCentered(&sContext, fullText, -1, 230, 140, 0);

		//max
		sprintf(fullText, "%d", pressure_history_data[pressure_max_index]);
		GrStringDrawCentered(&sContext, "Najwyzsze odnotowane cisnienie: ", -1, 160, 170, 0);
		GrStringDrawCentered(&sContext, fullText, -1, 70, 190, 0);
		GrStringDrawCentered(&sContext, "[hPa] ", -1, 110, 190, 0);
		GrStringDrawCentered(&sContext, "o godzinie: ", -1, 170, 190, 0);
		sprintf(fullText, "%02d:00", pressure_history_data_time[pressure_max_index].hours);
		GrStringDrawCentered(&sContext, fullText, -1, 230, 190, 0);
	}
	else
	{
		GrStringDrawCentered(&sContext, "Brak danych ", -1, 160, 155, 0);
	}

}



// This function draws the default screen 1 or screen 2

void draw_graph_screen()
{
	tRectangle rect;
	rect.sXMin = 0;
	rect.sXMax = 320;
	rect.sYMin = 0;
	rect.sYMax = 240;
	GrContextForegroundSet(&sContext, ClrBlack);
	GrRectFill(&sContext, &rect);
	GrContextForegroundSet(&sContext, ClrWhite);
	GrLineDrawH(&sContext, 0, 320, 40);

	//back button
	rect.sXMin = 0;
	rect.sXMax = 100;
	rect.sYMin = 0;
	rect.sYMax = 35;
	GrContextForegroundSet(&sContext, ClrGold);
	GrRectDraw(&sContext, &rect);
	rect.sXMin = 2;
	rect.sXMax = 98;
	rect.sYMin = 2;
	rect.sYMax = 33;
	GrContextForegroundSet(&sContext, ClrBlack);
	GrRectFill(&sContext, &rect);
	GrContextForegroundSet(&sContext, ClrWhite);
	GrContextFontSet(&sContext, g_pFontCmss12);
	GrStringDrawCentered(&sContext, "Wstecz", -1, 50, 17, 0);

	//weekly temp button
	rect.sXMin = 218;
	rect.sXMax = 318;
	rect.sYMin = 0;
	rect.sYMax = 35;
	GrContextForegroundSet(&sContext, ClrGold);
	GrRectDraw(&sContext, &rect);
	GrContextForegroundSet(&sContext, ClrWhite);
	GrContextFontSet(&sContext, g_pFontCmss12);
	if (daily_flag) {
		GrContextForegroundSet(&sContext, ClrBlack);
		GrStringDrawCentered(&sContext, "Dzisiaj", -1, 268, 17, 0);
		GrContextForegroundSet(&sContext, ClrWhite);
		GrStringDrawCentered(&sContext, "Ten tydzien", -1, 268, 17, 0);
	}
	else {
		GrContextForegroundSet(&sContext, ClrBlack);
		GrStringDrawCentered(&sContext, "Ten tydzien", -1, 268, 17, 0);
		GrContextForegroundSet(&sContext, ClrWhite);
		GrStringDrawCentered(&sContext, "Dzisiaj", -1, 268, 17, 0);
	}
}



//This function draws on the display selected screen

void draw_background(int screen)
{
	//Home screen
	if(screen == 0)
	{
		if(!screen_0b)
		{
			GrImageDraw(&sContext, g_pucBackground, 0, 0);
			if(pressure_graph_ready)
				draw_pressure_history_graph();
		}
		else
		{
			GrImageDraw(&sContext, g_pucBackground, 0, 0);
			draw_pressure_info();
		}
		draw_sensor_data(displayed_data.temperature,displayed_data.pressure, displayed_data.humidity);
	}
	//Temperature graphs
	if(screen == 1)
	{
		draw_graph_screen();

		clear_daily_graph();
		if(daily_flag)
			draw_temperature_graph(24,daily_temp,true);
		else
		{
			if(!screen_1b)
				draw_temperature_graph(7,weekly_max_temp,false);
			else
				draw_temperature_graph(7,weekly_min_temp,false);
		}
	}
	//Humidity graphs
	if(screen == 2)
	{
		draw_graph_screen();
		clear_daily_graph();
		if(daily_flag)
			draw_humidity_graph(24,daily_humidity,true);
		else
			draw_humidity_graph(7,weekly_humidity,false);
	}
}



//This function is used when the power off button is pressed

void power_off_dialog()
{
	tRectangle rect;
	rect.sXMin = 0;
	rect.sXMax = 320;
	rect.sYMin = 0;
	rect.sYMax = 240;
	GrContextForegroundSet(&sContext, ClrBlack);
	GrRectFill(&sContext, &rect);
	rect.sXMin = 10;
	rect.sXMax = 310;
	rect.sYMin = 30;
	rect.sYMax = 210;
	GrContextForegroundSet(&sContext, ClrGold);
	GrRectDraw(&sContext, &rect);
	GrContextFontSet(&sContext, g_pFontCmss16b);
	GrContextForegroundSet(&sContext, ClrWhite);
	GrStringDrawCentered(&sContext, "Czy na pewno chcesz zakonczyc", -1, 160, 70,0);
	GrStringDrawCentered(&sContext, "dzialanie stacji pogodowej?", -1, 160, 90,0);
	GrContextForegroundSet(&sContext, ClrGold);
	//"yes"
	rect.sXMin = 50;
	rect.sXMax = 120;
	rect.sYMin = 120;
	rect.sYMax = 170;
	GrRectDraw(&sContext, &rect);
	GrContextForegroundSet(&sContext, ClrWhite);
	GrStringDrawCentered(&sContext, "Tak", -1,
			((rect.sXMax - rect.sXMin) / 2) + rect.sXMin,
			((rect.sYMax - rect.sYMin) / 2) + rect.sYMin, 0);
	//"no"
	rect.sXMin = 200;
	rect.sXMax = 270;
	rect.sYMin = 120;
	rect.sYMax = 170;
	GrContextForegroundSet(&sContext, ClrGold);
	GrRectDraw(&sContext, &rect);
	GrContextForegroundSet(&sContext, ClrWhite);
	GrStringDrawCentered(&sContext, "Nie", -1,
			((rect.sXMax - rect.sXMin) / 2) + rect.sXMin,
			((rect.sYMax - rect.sYMin) / 2) + rect.sYMin, 0);
	while(!dialog_answer){}
	dialog_answer = false;



}


int main(void)
{
	//Setting clock to 80MHz
	ROM_SysCtlClockSet (SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ);
	// The I2C1 peripheral must be enabled before use.
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
	GPIOPinTypeI2C(GPIO_PORTG_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	//pin muxing for i2c
	GPIOPinConfigure(GPIO_PG0_I2C1SCL);
	GPIOPinConfigure(GPIO_PG1_I2C1SDA);

	//Initialize BME280
	struct bme280_dev dev;
	int8_t rslt = BME280_OK;
	uint8_t dev_addr = BME280_I2C_ADDR_PRIM;

	dev.intf_ptr = &dev_addr;
	dev.intf = BME280_I2C_INTF;
	dev.read = i2c_read;
	dev.write = i2c_write;
	dev.delay_us = delay_ms;

	SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C1);
	I2CMasterInitExpClk(I2C1_MASTER_BASE, SysCtlClockGet(), false);
	SysCtlDelay(66670);
	I2CMasterEnable(I2C1_MASTER_BASE);

	//Initialize the display driver
	ILI9341_240x320x262K_Init();
	GrContextInit(&sContext, &g_sILI9341_240x320x262K);
	TouchScreenInit();
	TouchScreenCallbackSet(TouchCallback);
	//Initialize microSD card
	initializeCard();

	rslt = bme280_init(&dev);
	SysCtlDelay(13334);

	uint8_t rtc_rslt = 0;
	RTC_Time updated_time;

	init_data();
	draw_background(screen);
	tBoolean draw_once = true;


	while(RTC_Get(&updated_time)!=0)
	{
		if(draw_once)
		{
			GrContextForegroundSet(&sContext, ClrWhite);
			GrContextFontSet(&sContext, g_pFontCmss12b);
			GrStringDrawCentered(&sContext, "Nie wykryto modulu RTC", -1, 160, 55, 0);
			draw_once = false;
		}
	}
	if(!draw_once)
	{
		GrContextForegroundSet(&sContext, ClrBlack);
		GrContextFontSet(&sContext, g_pFontCmss12b);
		GrStringDrawCentered(&sContext, "Nie wykryto modulu RTC", -1, 160, 55, 0);
	}

		check_time(&updated_time);

	//first measurement is innacurate, so it's nessesary to make another one

	int i;
	for(i=0;i<2;i++)
	{
		stream_sensor_data_forced_mode(&dev);
		SysCtlDelay(833375);
	}
	phd_save_locked = false; //pressure data can be saved on the microSD card
	//clear_loaded_data();


	// load pressure history data
	
	load_pressure_history_data(1, 52, pressure_history_loaded_data, pressure_history_loaded_data_time);
	check_loaded_pressure_history_data();
	draw_pressure_history_graph();

	
	//load daily_temp
	
	load_temp_or_humidity(3, 24, loaded_daily_temp, loaded_daily_temp_time,true);
	check_loaded_daily_temp();

	
	// load weekly_max_temp
	
	load_temp_or_humidity(5, 7, loaded_weekly_max_temp, loaded_weekly_max_temp_time,false);
	check_loaded_weekly_max_temp(weekly_max_temp, weekly_max_temp_time);

	
	// load daily humidity
	
	load_temp_or_humidity(7,24,loaded_daily_humidity, loaded_daily_humidity_time, true);
	check_loaded_daily_humidity();

	
	//load weekly humidity
	
	load_temp_or_humidity(9, 7, loaded_weekly_humidity, loaded_weekly_humidity_time,false);
	check_loaded_weekly_humidity();

	
	// load weekly min temp
	
	load_temp_or_humidity(11,7, loaded_weekly_min_temp, loaded_weekly_min_temp_time,false);
	check_loaded_weekly_min_temp(weekly_min_temp, weekly_min_temp_time);

	tBoolean bme280_start_measurement = true;
	tBoolean measure_temp = true;
	last_hour_temp = displayed_data.temperature;
	last_hour_humidity = displayed_data.humidity;

	//when it's the first iteration of the main loop, don't communicate with BME280, because it can freeze the program
	tBoolean first_iteration = true;
	
	 //Main loop
	for(;;)
	{
		//Update the display if a screen was changed
		if(screen_changed)
		{
			draw_background(screen);
			check_time(&updated_time);
			screen_changed = false;
		}
		else
		{
			rtc_rslt = RTC_Get(&updated_time);
			if(rtc_rslt==0)
				check_time(&updated_time);
			//get data from the bme280 every 3 seconds
			if(updated_time.seconds%3 == 0 && bme280_start_measurement && !first_iteration)
			{
				stream_sensor_data_forced_mode(&dev);
				bme280_start_measurement = false;
			}
			else if(updated_time.seconds%3 != 0)
			{
				bme280_start_measurement = true;
			}
			
			// daily temp check
			
			if(updated_time.minutes%5 == 0 && measure_temp)
			{
				measure_temp = false;
				temp_measurements_counter++;
				humidity_measurements_counter++;
				last_hour_temp+=displayed_data.temperature;
				last_hour_humidity+=displayed_data.humidity;
			}
			else if(updated_time.minutes%5 != 0)
			{
				measure_temp = true;
			}
		}
		if(power_off)
		{
			power_off_dialog();
			if(power_off)
			{
				tRectangle rect;
				rect.sXMin = 0;
				rect.sXMax = 320;
				rect.sYMin = 0;
				rect.sYMax = 240;
				GrContextForegroundSet(&sContext, ClrBlack);
				GrRectFill(&sContext, &rect);
				GrContextFontSet(&sContext, g_pFontCmss26b);
				GrContextForegroundSet(&sContext, ClrWhite);
				GrStringDrawCentered(&sContext, "Mozesz wylaczyc zasilanie", -1, 160, 120, 0);
				for(;;)
				{

				}

			}
			else
			{
				//update display
				screen_changed = true;
			}

		}
		if(first_iteration)
			first_iteration = false;
		SysCtlDelay(1333400);
	}

	return 0;
}
