/**
 * @file hal_gw1.c
 *
 * @brief public methods for hal_gw1.c
 *
 * $Rev: 1916 $
 * $Author: dsmith $
 * $Date: 2013-09-26 17:42:19 -0700 (Thu, 26 Sep 2013) $
 *
 * (c) 2012 Tesla Controls. All Rights Reserved. Proprietary and Confidential.
 *
 * Copyright (c) 2012 Tesla Controls. All rights reserved. Redistribution and use in source and
 * binary forms, with or without modification, are permitted only if used solely and exclusively with
 * an Anaren Zigbee 2007 Module. Refer to the file "eula.txt" for additional restrictions and
 * limitations on this Software.
 */

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "inc/hw_ssi.h"
#include <stdint.h>

//general includes
#include "utils/uartstdio.h"
#define printf UARTprintf

void halInit();
void delayMs(uint16_t ms);
void halSpiInitModule();
void spiWrite(uint8_t *bytes, uint8_t numBytes);
int16_t toggleLed(uint8_t whichLed);
int16_t setLed(uint8_t led);
int16_t clearLed(uint8_t led);
void clearLeds();

void softwareControlEthernetLeds();
void hardwareControlEthernetLeds();
void setEthernetLeds();
void clearEthernetLeds();
void halDisableModuleInterface();

void halSwitchToBootloader();
uint32_t halCheckForApplication();
uint32_t halCheckForBootloader();

uint8_t buttonIsPressed(uint8_t button);
// Buttons
#define ANY_BUTTON          0xFF
#define BUTTON_EXTERNAL            0
#define BUTTON_INTERNAL            1


#define GET_MCLK_FREQ() (SysCtlClockGet())
#define TICKS_PER_MS	25000			// used in FAST_PROCESSOR option of zm_phy

#define WAIT_WHILE_SPI_BUSY()  while ((HWREG(SSI0_BASE + SSI_O_SR)) & SSI_SR_BSY)   //wait while busy

//#define POWER_LED_ON()                  ( GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_0, GPIO_PIN_0) )
//#define POWER_LED_OFF()                 ( GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_0, 0) )
#define STATUS_LED_OFF()                ( GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_5|GPIO_PIN_6, 0) )
#define STATUS_LED_RED()                ( GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_5|GPIO_PIN_6, GPIO_PIN_6) )
#define STATUS_LED_GREEN()              ( GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_5|GPIO_PIN_6, GPIO_PIN_5) )
#define STATUS_LED_YELLOW()             ( GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_5|GPIO_PIN_6, GPIO_PIN_5|GPIO_PIN_6) )

#define GET_STATUS_LED()                (GPIOPinRead(GPIO_PORTB_BASE, GPIO_PIN_5|GPIO_PIN_6))
#define SET_STATUS_LED(value)           ( GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_5|GPIO_PIN_6, (value) ))

//REQUIRED FOR MODULE LIBRARY:

//Module RST = PD5
//Module MRDY = CS = PD4
//Module SRDY = PD6
//PRECONDITIONS: GPIO PORTS HAVE BEEN ENABLED & PINS WERE CONFIGURED: PD4, PD5 as outputs; PD6 as input
#define RADIO_ON()                  ( GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_5, GPIO_PIN_5) )
#define RADIO_OFF()                 ( GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_5, 0) )
#define SPI_SS_SET()                ( GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_4, 0) ) 	//MRDY tied to CS in hardware?
#define SPI_SS_CLEAR()              ( GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_4, GPIO_PIN_4) )
#define SRDY_IS_HIGH()              (GPIOPinRead(GPIO_PORTD_BASE, GPIO_PIN_6) & GPIO_PIN_6)
#define SRDY_IS_LOW()               (~(GPIOPinRead(GPIO_PORTD_BASE, GPIO_PIN_6)) & GPIO_PIN_6)

#define HAL_ENABLE_INTERRUPTS()		IntMasterEnable()
#define HAL_DISABLE_INTERRUPTS()	IntMasterDisable()

#define SPI_FLASH_SS_SET()                ( GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_5, 0) )
#define SPI_FLASH_SS_CLEAR()              ( GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_5, GPIO_PIN_5) )


#define DEBUG_ON()
#define DEBUG_OFF()

#define XTAL                        25000000L
