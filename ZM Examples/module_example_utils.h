/**
*  @file module_examples_utilities.h
*
*  @brief  public methods for module_examples_utilities.c
*
* $Rev: 2200 $
* $Author: dsmith $
* $Date: 2014-06-19 11:48:25 -0700 (Thu, 19 Jun 2014) $
*
* @section support Support
* Please refer to the wiki at www.anaren.com/air-wiki-zigbee for more information. Additional support
* is available via email at the following addresses:
* - Questions on how to use the product: AIR@anaren.com
* - Feature requests, comments, and improvements:  featurerequests@teslacontrols.com
* - Consulting engagements: sales@teslacontrols.com
*
 * @section license License
 * Copyright (c) 2012 Tesla Controls. All rights reserved. This Software may only be used with an 
 * Anaren A2530E24AZ1, A2530E24CZ1, A2530R24AZ1, or A2530R24CZ1 module. Redistribution and use in 
 * source and binary forms, with or without modification, are subject to the Software License 
 * Agreement in the file "anaren_eula.txt"
 * 
 * YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE PROVIDED “AS IS” 
 * WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY 
 * WARRANTY OF MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO 
 * EVENT SHALL ANAREN MICROWAVE OR TESLA CONTROLS BE LIABLE OR OBLIGATED UNDER CONTRACT, NEGLIGENCE, 
 * STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR 
 * INDIRECT DAMAGES OR EXPENSE INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, 
 * PUNITIVE OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT OF SUBSTITUTE 
 * GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES (INCLUDING BUT NOT LIMITED TO ANY 
 * DEFENSE THEREOF), OR OTHER SIMILAR COSTS.
 */

#ifndef MODULE_EXAMPLES_UTILS_H
#define MODULE_EXAMPLES_UTILS_H

#include "Messages/kvp.h"

//#define IS_INFO_MESSAGE_CLUSTER()  ((AF_INCOMING_MESSAGE_CLUSTER()) == INFO_MESSAGE_CLUSTER)


/* We use the Module LEDs to indicate whether or not we're on the network */
#define BOOSTER_BACK_MODULE_LED_YELLOW      (0x04)
#define BOOSTER_BACK_MODULE_LED_RED         (0x08)



#define INIT_BOOSTER_PACK_LEDS()            (sysGpio(GPIO_SET_DIRECTION , ALL_GPIO_PINS))  //Configures these as outputs
#define SET_NETWORK_LED_ON()                (sysGpio(GPIO_SET, BOOSTER_BACK_MODULE_LED_YELLOW))
#define SET_NETWORK_LED_OFF()               (sysGpio(GPIO_CLEAR, BOOSTER_BACK_MODULE_LED_YELLOW))
#define SET_NETWORK_FAILURE_LED_ON()        (sysGpio(GPIO_SET, BOOSTER_BACK_MODULE_LED_RED))
#define SET_NETWORK_FAILURE_LED_OFF()       (sysGpio(GPIO_CLEAR, BOOSTER_BACK_MODULE_LED_RED))





void pollAndDisplay();

#define MAX_SENSORS 10

void initializeSensors();
int8_t getSensorValues(struct kvp *kvps);
void testSensors();
void displayColorOnRgbLed(uint16_t red, uint16_t blue, uint16_t green);
void displayTemperatureOnRgbLed(int16_t value);
void resetNominalTemperature();
void resetNominalColor();

#endif
