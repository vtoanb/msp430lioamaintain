/**
* @ingroup moduleCommunications
* @{ 
*
* @file example_per_tester_coordinator.c
*
* @brief Packet Error Rate Tester: configures this device to be a Zigbee Coordinator and counts the 
* number of messages received. Stops once 1000 messages are received. LED 0 will toggle each time
* a message is received. Pressing the button will display the message count.
* Uses the AF/ZDO interface.
*
* This matches example_per_tester_router.c
* Get this running before the router, or else the router won't have anything to join to.
*
* $Rev: 1820 $
* $Author: dsmith $
* $Date: 2013-05-22 16:21:05 -0700 (Wed, 22 May 2013) $
*
* @section support Support
* Please refer to the wiki at www.anaren.com/air-wiki-zigbee for more information. Additional support
* is available via email at the following addresses:
* - Questions on how to use the product: AIR@anaren.com
* - Feature requests, comments, and improvements:  featurerequests@teslacontrols.com
* - Consulting engagements: sales@teslacontrols.com
*
* @section license License
* Copyright (c) 2013 Tesla Controls. All rights reserved. This Software may only be used with an 
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
* 
*/
#include "../HAL/hal.h"
#include "../ZM/module.h"
#include "../ZM/af.h"
#include "../ZM/zdo.h"
#include "../ZM/zm_phy.h"
#include "../ZM/module_errors.h"
#include "../ZM/module_utilities.h"
#include "../Common/utilities.h"
#include "module_example_utils.h"
#include <stdint.h>

/** This will hold the result of various module functions. Define once here so we can reuse. */
moduleResult_t result;

extern uint8_t zmBuf[ZIGBEE_MODULE_BUFFER_SIZE];

uint16_t packetCounter = 0;

void waitForPackets();

/** function pointer (in hal file) for the function that gets called when a button is pressed*/
extern void (*buttonIsr)(int8_t);

void handleButtonPress(int8_t whichButton);

#define NUMBER_OF_PACKETS_TO_RECEIVE 1000

/** Set this to the part of the world where you are located to ensure compliance with FCC/ETSI etc. */
#define MODULE_REGION   MODULE_REGION_NORTH_AMERICA

int main( void )
{
    halInit();
    moduleInit();    
    printf("\r\n****************************************************\r\n");
    printf("Packet Error Rate Tester - COORDINATOR\r\n");
    buttonIsr = &handleButtonPress;       
    setLed(0);
    
#define MODULE_START_DELAY_IF_FAIL_MS 5000
    /* Use the default module configuration */
    struct moduleConfiguration defaultConfiguration = DEFAULT_MODULE_CONFIGURATION_COORDINATOR;
    
    /* Try to start module with the default configuration */
    while ((result = expressStartModule(&defaultConfiguration, GENERIC_APPLICATION_CONFIGURATION, MODULE_REGION)) != MODULE_SUCCESS)
    {
        /* Module startup failed; display error */
        printf("Module start unsuccessful. Error Code 0x%02X. Retrying...\r\n", result);
        
        /* Wait a few seconds before trying again, in case the rest of the network is restarting too */
        delayMs(MODULE_START_DELAY_IF_FAIL_MS);
    }
    printf("On Network!\r\n");  
    setLed(1);
    
    /* On network, so display info about this network */
#ifdef DISPLAY_NETWORK_INFORMATION     
    displayNetworkConfigurationParameters();                
    displayDeviceInformation();
#else
    displayBasicDeviceInformation();
#endif 
    HAL_ENABLE_INTERRUPTS();
    printf("Silently listening for %u packets; press button to display count\r\n", NUMBER_OF_PACKETS_TO_RECEIVE);
    printf("Count = %u; Start router sending packets now.\r\n", packetCounter);

    
    /* Now the network is running - continually poll for any received messages from the Module */
    while (1)
    {
        waitForPackets();
        printf("DONE: %u packets received! Resetting Counter and receiving again.\r\nPress button to view count\r\n", NUMBER_OF_PACKETS_TO_RECEIVE); 
    }
}

/* Function that gets called via buttonIsr function pointer whenever a button is pressed. */
void handleButtonPress(int8_t whichButton)
{
    printf("Count = %u\r\n", packetCounter);
}

/** Waits for messages; increments a counter when a packet was received.*/
void waitForPackets()
{
    zmBuf[0] = 0;    
    packetCounter = 0;  
    while (packetCounter < NUMBER_OF_PACKETS_TO_RECEIVE) 
    {
        /* Wait until a message arrives */
        while (!(MODULE_HAS_MESSAGE_WAITING()));
        
        /* Retrieve the message */
        getMessage();      
        
        /* If the length is greater than 0 and it is an incoming message then increment counter */
        if ((zmBuf[SRSP_LENGTH_FIELD] > 0) && (IS_AF_INCOMING_MESSAGE()))
        {
            toggleLed(0);
            packetCounter++;
            zmBuf[0] = 0;  
        }
        /* Otherwise, ignore the message. */
    }   
}

/* @} */