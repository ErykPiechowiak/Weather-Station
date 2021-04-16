/*
 * DS3231.h
 *
 *  Created on: 1 sty 2021
 *      Author: Eryk Piechowiak
 */

#ifndef DS3231_H_
#define DS3231_H_


#include "DS3231_defs.h"

uint8_t bcd_to_decimal(uint8_t number);
uint8_t decimal_to_bcd(uint8_t number);
void RTC_Set(RTC_Time *time_t);
uint8_t RTC_Get(RTC_Time *c_time);
int8_t RTC_Write_Date_Start();//
int8_t RTC_Set_Date_Registers(RTC_Time *time_t);
int8_t RTC_Write_Date(RTC_Time *time_t);
uint8_t RTC_Read_Reg(uint8_t reg_address);
int8_t RTC_Write_Reg(uint8_t reg_address, uint8_t reg_value);




#endif /* DS3231_H_ */
