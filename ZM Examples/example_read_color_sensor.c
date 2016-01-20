/**
 * @ingroup hwExamples
 * @{
 * @file example_read_color_sensor.c
 *
 * @brief Uses the MSP430's I2C interface to read the external color sensor.
 *
 * Uses the Hardware Abstraction Layer (hal).
 *
 * $Rev: 1770 $
 * $Author: dsmith $
 * $Date: 2013-03-07 14:55:49 -0800 (Thu, 07 Mar 2013) $
 *
 * @section support Support
 * Please refer to the wiki at www.anaren.com/air-wiki-zigbee for more information. Additional support
 * is available via email at the following addresses:
 * - Questions on how to use the product: AIR@anaren.com
 * - Feature requests, comments, and improvements:  featurerequests@teslacontrols.com
 * - Consulting engagements: sales@teslacontrols.com
 *
 * @section license License
 * Copyright (c) 2012 Anaren Microwave. All rights reserved. This Software may only be used with an 
 * Anaren A2530E24AZ1, A2530E24CZ1, A2530R24AZ1, or A2530R24CZ1 module. Redistribution and use in 
 * source and binary forms, with or without modification, are subject to the Software License 
 * Agreement in the file "anaren_eula.txt"
 * 
 * YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE PROVIDED �AS IS� 
 * WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY 
 * WARRANTY OF MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO 
 * EVENT SHALL ANAREN MICROWAVE OR TESLA CONTROLS BE LIABLE OR OBLIGATED UNDER CONTRACT, NEGLIGENCE, 
 * STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR 
 * INDIRECT DAMAGES OR EXPENSE INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, 
 * PUNITIVE OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT OF SUBSTITUTE 
 * GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES (INCLUDING BUT NOT LIMITED TO ANY 
 * DEFENSE THEREOF), OR OTHER SIMILAR COSTS.
 */

#include "../HAL/hal.h"
#include "../HAL/hal_AMS_TCS3414_color_sensor.h"
#include "../Common/utilities.h"
#include <stdint.h>

int main( void )
{
    halInit();
    printf("Read Color Sensor Example\r\n");

    /* Initialize the I2C interface */
    i2cInit(COLOR_SENSOR_I2C_ADDRESS);

    /* Initialize the color sensor */
    colorSensorInit();

    delayMs(100);  // Wait for the first integration (set to 100mSec)

    while (1) 
    {
        struct ColorReading color;
        setLed(1);
        printf("Getting Color Reading: ");
        color = getColor();
        printf("RED: %05u - BLUE: %05u - GREEN: %05u - CLEAR: %05u\r\n\r\n", color.red, color.blue, color.green, color.clear);
        clearLeds();
        delayMs(1000);      
    }

}

/* @} */
