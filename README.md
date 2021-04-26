##  Microcontroller based weather monitoring system

## Table of contents
* [General info](#general-info)
* [Technologies](#technologies)

## General info
This project was finished in January 2021 as part of my engineering thesis.

In this project a microcontroller was used to create a weather station which allows the user to control its interface with gestures. 
The weather station allows monitoring of the current temperature, atmospheric pressure and humidity values, as well as previewing and analysing gathered data.
The main component used to create the weather station was the EasyMX Pro v7 evaluation board equipped with a LCD touch panel and a slot for a microSD card used for gathering data.
The measurements are taken by the BME280 Bosch sensor, which is communicating with the microcontroller through the I2C interface.

Demonstration video: https://drive.google.com/file/d/1c9Z6uB6EpTKl8SJMBl1BelDhtPBgGi30/view?usp=sharing

## Technologies 

* Programming language: C
* Standards used to communicate with peripheral devices: I2C, SPI
* IDE: Code Composer Studio 6.1.0


