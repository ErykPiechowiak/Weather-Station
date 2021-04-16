///////////////////////////////////////////////////////////////////////////
////                                                                   ////
////                             DS3231.c                              ////
////                                                                   ////
////                      Driver for CCS C compiler                    ////
////                                                                   ////
////     Driver for Maxim DS3231 serial I2C real-time clock (RTC).     ////
////                                                                   ////
///////////////////////////////////////////////////////////////////////////
////                                                                   ////
////                     https://simple-circuit.com/                   ////
////                                                                   ////
///////////////////////////////////////////////////////////////////////////


#include "DS3231.h"
#include <stdint.h>
#include "stdbool.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/i2c.h"






RTC_Time c_time, c_alarm1, c_alarm2;


// converts BCD to decimal
uint8_t bcd_to_decimal(uint8_t number)
{
  return ( (number >> 4) * 10 + (number & 0x0F) );
}

// converts decimal to BCD
uint8_t decimal_to_bcd(uint8_t number)
{
  return ( ((number / 10) << 4) + (number % 10) );
}




// sets time and date
void RTC_Set(RTC_Time *time_t)
{
  // convert decimal to BCD
  time_t->day     = decimal_to_bcd(time_t->day);
  time_t->month   = decimal_to_bcd(time_t->month);
  time_t->year    = decimal_to_bcd(time_t->year);
  time_t->hours   = decimal_to_bcd(time_t->hours);
  time_t->minutes = decimal_to_bcd(time_t->minutes);
  time_t->seconds = decimal_to_bcd(time_t->seconds);
  // end conversion

  // write data to the RTC chip
  RTC_Write_Date(time_t);
}

// reads time and date
uint8_t RTC_Get(RTC_Time *c_time)
{

	//RTC_Time c_time;
	uint8_t rslt = 0; /* Return 0 for Success, non-zero for failure */

	I2CMasterSlaveAddrSet(I2C1_MASTER_BASE, DS3231_ADDRESS, false); //write mode
	I2CMasterDataPut(I2C1_MASTER_BASE, DS3231_REG_SECONDS);
	I2CMasterControl(I2C1_MASTER_BASE, I2C_MASTER_CMD_SINGLE_SEND);
	while (I2CMasterBusy(I2C1_MASTER_BASE)) {
	} //wait till transmission is done
	//check if transmission was successful
	if (I2CMasterErr(I2C1_MASTER_BASE) != I2C_MASTER_ERR_NONE) {
		rslt = 1;
		return rslt;          //transmission failed
	}

	I2CMasterSlaveAddrSet(I2C1_MASTER_BASE, DS3231_ADDRESS, true); //read mode
	I2CMasterControl(I2C1_MASTER_BASE,I2C_MASTER_CMD_BURST_RECEIVE_START);
	while (I2CMasterBusy(I2C1_MASTER_BASE)) {}
	if (I2CMasterErr(I2C1_MASTER_BASE) != I2C_MASTER_ERR_NONE) {
		rslt = 1;
		return rslt;
	}
	c_time->seconds = I2CMasterDataGet(I2C1_MASTER_BASE);

	I2CMasterControl(I2C1_MASTER_BASE,
						I2C_MASTER_CMD_BURST_RECEIVE_CONT);
	while (I2CMasterBusy(I2C1_MASTER_BASE)) {}
	if (I2CMasterErr(I2C1_MASTER_BASE) != I2C_MASTER_ERR_NONE) {
		rslt = 1;
		return rslt;}

	c_time->minutes = I2CMasterDataGet(I2C1_MASTER_BASE);

	I2CMasterControl(I2C1_MASTER_BASE, I2C_MASTER_CMD_BURST_RECEIVE_CONT);
	while (I2CMasterBusy(I2C1_MASTER_BASE)) {}
	if (I2CMasterErr(I2C1_MASTER_BASE) != I2C_MASTER_ERR_NONE) {
		rslt = 1;
		return rslt;}

	c_time->hours = I2CMasterDataGet(I2C1_MASTER_BASE);

	I2CMasterControl(I2C1_MASTER_BASE, I2C_MASTER_CMD_BURST_RECEIVE_CONT);
	while (I2CMasterBusy(I2C1_MASTER_BASE)) {}
	if (I2CMasterErr(I2C1_MASTER_BASE) != I2C_MASTER_ERR_NONE) {
		rslt = 1;
		return rslt;
	}

	c_time->dow = I2CMasterDataGet(I2C1_MASTER_BASE);

	I2CMasterControl(I2C1_MASTER_BASE, I2C_MASTER_CMD_BURST_RECEIVE_CONT);
	while (I2CMasterBusy(I2C1_MASTER_BASE)) {}
	if (I2CMasterErr(I2C1_MASTER_BASE) != I2C_MASTER_ERR_NONE) {
		rslt = 1;
		return rslt;
	}

	c_time->day = I2CMasterDataGet(I2C1_MASTER_BASE);

	I2CMasterControl(I2C1_MASTER_BASE, I2C_MASTER_CMD_BURST_RECEIVE_CONT);
	while (I2CMasterBusy(I2C1_MASTER_BASE)) {}
	if (I2CMasterErr(I2C1_MASTER_BASE) != I2C_MASTER_ERR_NONE) {
		rslt = 1;
		return rslt;
	}

	c_time->month = I2CMasterDataGet(I2C1_MASTER_BASE);

	I2CMasterControl(I2C1_MASTER_BASE, I2C_MASTER_CMD_BURST_RECEIVE_FINISH);
	while (I2CMasterBusy(I2C1_MASTER_BASE)) {}

	if (I2CMasterErr(I2C1_MASTER_BASE) != I2C_MASTER_ERR_NONE)
	{
		rslt = 1;
		return rslt;
	}
	c_time->year = I2CMasterDataGet(I2C1_MASTER_BASE);

	c_time->seconds = bcd_to_decimal(c_time->seconds);
	c_time->minutes = bcd_to_decimal(c_time->minutes);
	c_time->hours = bcd_to_decimal(c_time->hours);
	c_time->day = bcd_to_decimal(c_time->day);
	c_time->month = bcd_to_decimal(c_time->month);
	c_time->year = bcd_to_decimal(c_time->year);

	return rslt;


}

int8_t RTC_Write_Date_Start()
{
	int8_t rslt = 0; /* Return 0 for Success, non-zero for failure */


	while(I2CMasterBusy(I2C1_MASTER_BASE)){}
	I2CMasterSlaveAddrSet(I2C1_MASTER_BASE, DS3231_ADDRESS , false);	    //Sending reg_addr
    I2CMasterDataPut(I2C1_MASTER_BASE, DS3231_REG_SECONDS);
	I2CMasterControl(I2C1_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_START);
    while(I2CMasterBusy(I2C1_MASTER_BASE)) {} //wait till transmission is done
	//check if transmission was successful
	if(I2CMasterErr(I2C1_MASTER_BASE) != I2C_MASTER_ERR_NONE)
    {
		return 1;
    }
	return rslt;
}

int8_t RTC_Set_Date_Registers(RTC_Time *time_t)
{
	int8_t rslt = 0;
	I2CMasterDataPut(I2C1_MASTER_BASE, time_t->seconds);
	I2CMasterControl(I2C1_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_CONT);
	while (I2CMasterBusy(I2C1_MASTER_BASE)) {
	}
	if (I2CMasterErr(I2C1_MASTER_BASE) != I2C_MASTER_ERR_NONE) {
		return 1;
	}
	I2CMasterDataPut(I2C1_MASTER_BASE, time_t->minutes);
	I2CMasterControl(I2C1_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_CONT);
	while (I2CMasterBusy(I2C1_MASTER_BASE)) {
	}
	if (I2CMasterErr(I2C1_MASTER_BASE) != I2C_MASTER_ERR_NONE) {
		return 1;
	}
	I2CMasterDataPut(I2C1_MASTER_BASE, time_t->hours);
	I2CMasterControl(I2C1_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_CONT);
	while (I2CMasterBusy(I2C1_MASTER_BASE)) {
	}
	if (I2CMasterErr(I2C1_MASTER_BASE) != I2C_MASTER_ERR_NONE) {
		return 1;
	}
	I2CMasterDataPut(I2C1_MASTER_BASE, time_t->dow);
	I2CMasterControl(I2C1_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_CONT);
	while (I2CMasterBusy(I2C1_MASTER_BASE)) {
	}
	if (I2CMasterErr(I2C1_MASTER_BASE) != I2C_MASTER_ERR_NONE) {
		return 1;
	}
	I2CMasterDataPut(I2C1_MASTER_BASE, time_t->day);
	I2CMasterControl(I2C1_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_CONT);
	while (I2CMasterBusy(I2C1_MASTER_BASE)) {
	}
	if (I2CMasterErr(I2C1_MASTER_BASE) != I2C_MASTER_ERR_NONE) {
		return 1;
	}
	I2CMasterDataPut(I2C1_MASTER_BASE, time_t->month);
	I2CMasterControl(I2C1_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_CONT);
	while (I2CMasterBusy(I2C1_MASTER_BASE)) {
	}
	if (I2CMasterErr(I2C1_MASTER_BASE) != I2C_MASTER_ERR_NONE) {
		return 1;
	}
	I2CMasterDataPut(I2C1_MASTER_BASE, time_t->year);
	I2CMasterControl(I2C1_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);
	while (I2CMasterBusy(I2C1_MASTER_BASE)) {
	}
	if (I2CMasterErr(I2C1_MASTER_BASE) != I2C_MASTER_ERR_NONE) {
		return 1;
	}


	return rslt;
}

int8_t RTC_Write_Date(RTC_Time *time_t)
{

	 int8_t rslt = 0; /* Return 0 for Success, non-zero for failure */

	 RTC_Write_Date_Start();
	 RTC_Set_Date_Registers(time_t);

	 return rslt;


}


// writes 'reg_value' to register of address 'reg_address'
int8_t RTC_Write_Reg(uint8_t reg_address, uint8_t reg_value)
{

  int8_t rslt = 0; /* Return 0 for Success, non-zero for failure */


  	while (I2CMasterBusy(I2C0_MASTER_BASE)) {
	}
	I2CMasterSlaveAddrSet(I2C0_MASTER_BASE, DS3231_ADDRESS, false);//Sending reg_addr
	I2CMasterDataPut(I2C0_MASTER_BASE, reg_address);
	I2CMasterControl(I2C0_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_START);
	while (I2CMasterBusy(I2C0_MASTER_BASE)) {
	} //wait till transmission is done
	//check if transmission was successful
	if (I2CMasterErr(I2C0_MASTER_BASE) != I2C_MASTER_ERR_NONE) {
		return 1;
	}
	I2CMasterDataPut(I2C0_MASTER_BASE, reg_value);
	I2CMasterControl(I2C0_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);
	while (I2CMasterBusy(I2C0_MASTER_BASE)) {}
	if (I2CMasterErr(I2C0_MASTER_BASE) != I2C_MASTER_ERR_NONE) {
		return 1;
	}

	return rslt;


}

// returns the value stored in register of address 'reg_address'
uint8_t RTC_Read_Reg(uint8_t reg_address)
{
	uint8_t reg_data;

  	I2CMasterSlaveAddrSet(I2C0_MASTER_BASE, DS3231_ADDRESS, false); //write mode
	I2CMasterDataPut(I2C0_MASTER_BASE, reg_address);
	I2CMasterControl(I2C0_MASTER_BASE, I2C_MASTER_CMD_SINGLE_SEND);
	while (I2CMasterBusy(I2C0_MASTER_BASE)) {
	} //wait till transmission is done
	  //check if transmission was successful
	if (I2CMasterErr(I2C0_MASTER_BASE) != I2C_MASTER_ERR_NONE) {
		return 1;          //transmission failed
	}

	I2CMasterSlaveAddrSet(I2C0_MASTER_BASE, DS3231_ADDRESS, true);//read mode
	I2CMasterControl(I2C0_MASTER_BASE, I2C_MASTER_CMD_SINGLE_RECEIVE);
	while (I2CMasterBusy(I2C0_MASTER_BASE)) {
	}
	if (I2CMasterErr(I2C0_MASTER_BASE) != I2C_MASTER_ERR_NONE) {
		return 1;
	}
	reg_data = I2CMasterDataGet(I2C0_MASTER_BASE);




  return reg_data;
}




