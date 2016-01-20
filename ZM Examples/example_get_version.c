/**
 * @ingroup moduleInterface
 * @{
 *
 * @file example_get_version.c
 *
 * @brief Resets Radio, gets module version information using SYS_VERSION command and displays it.
 *
 * Configures the application processor to communicate with the radio, resets the radio, and sends the
 * SYS_VERSION command and parses the received response and displays it to the console.
 * Demonstrates basic request-response functionality of the module.
 *
 * @note these fields should not be all zeroes. If you're only seeing zeroes then you may have a bug.
 * Typical values seen are <i>Version: TransportRev=2, ProductId=126, FW Rev=2.5.1</i>
 *
 * $Rev: 1883 $
 * $Author: dsmith $
 * $Date: 2013-08-22 11:18:21 -0700 (Thu, 22 Aug 2013) $
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
#include "../ZM/module.h"
#include "../ZM/module_errors.h"
#include <stdint.h>

moduleResult_t result = MODULE_SUCCESS;

int main( void )
{
    halInit();
    moduleInit();      
    printf("\r\nResetting Module, then getting Version Information\r\n");
    moduleReset();  
    while (1)
    {
        result = sysVersion();
        if (result == MODULE_SUCCESS)                  //gets the version string
        {
            displaySysVersion();  // Display the contents of the received SYS_VERSION
        } else {
            printf("ERROR 0x%02X\r\n", result);
        }    
        toggleLed(1);
        delayMs(1000);
    }
}

/* @} */
