/**
* @ingroup moduleCommunications
* @{
*
* @file example_per_tester_router.c
*
* @brief Packet Error Rate Tester: Resets Radio, configures this device to be a Zigbee Router, joins 
* a network, then sends 1000 messages to coordinator. Pressing the button will restart the application.
*
* Uses the AF/ZDO interface.
*
* If devices are not communicating, look at the device information fields and verify that both
* the coordinator and router have the same PAN ID and that the Extended PAN ID of the router
* matches the MAC address of the coordinator.
*
* $Rev: 1968 $
* $Author: dsmith $
* $Date: 2013-11-25 17:14:50 -0800 (Mon, 25 Nov 2013) $
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
*/

#include "../HAL/hal.h"
#include "../ZM/module.h"
#include "../ZM/module_errors.h"
#include "../ZM/module_utilities.h"
#include "../ZM/af.h"
#include "../ZM/zdo.h"
#include "../ZM/zm_phy.h"
#include "../Common/utilities.h"
#include "module_example_utils.h"
#include <string.h>                 //for memcpy()

/** function pointer (in hal file) for the function that gets called when a button is pressed*/
extern void (*buttonIsr)(int8_t);

/** This will hold the result of various module functions. Define once here so we can reuse. */
moduleResult_t result;              

/** Handles button interrupt */
void handleButtonPress(int8_t whichButton);

uint8_t counter = 0;

/* Note: TEST_MESSAGE_PAYLOAD_LENGTH + MESSAGE_HEADER_LENGTH must be less than length of zmBuf */
#define TEST_MESSAGE_PAYLOAD_LENGTH     10  

/* The message that will be sent each time */
//uint8_t testMessage[];   

#define NUMBER_OF_PACKETS_TO_SEND 1000    

extern uint8_t zmBuf[ZIGBEE_MODULE_BUFFER_SIZE];

/** The number of failed messages before initiating a network restart */
uint8_t failCount = 0;
#define MAX_FAILED_MESSAGES_BEFORE_RESTART  2

int main( void )
{
    halInit();
    moduleInit();
    printf("\r\n****************************************************\r\n");
    printf("Packet Error Rate Tester - ROUTER\r\n");
    buttonIsr = &handleButtonPress;
    
#define MODULE_START_DELAY_IF_FAIL_MS 5000
    
    /* Use the default module configuration */
    struct moduleConfiguration defaultConfiguration = DEFAULT_MODULE_CONFIGURATION_ROUTER;
    
    /* Turn Off nwk status LED if on */
    clearLed(ON_NETWORK_LED);
    
    /* Loop until module starts */
    while ((result = expressStartModule(&defaultConfiguration, GENERIC_APPLICATION_CONFIGURATION, MODULE_REGION_NORTH_AMERICA)) != MODULE_SUCCESS)
    {
        /* Module startup failed; display error and blink LED */
        setLed(NETWORK_FAILURE_LED);                    
        printf("Module start unsuccessful. Error Code 0x%02X. Retrying...\r\n", result);
        delayMs(MODULE_START_DELAY_IF_FAIL_MS/2);                    
        clearLed(NETWORK_FAILURE_LED);
        delayMs(MODULE_START_DELAY_IF_FAIL_MS/2);
    }
    printf("On Network!\r\n");
    
    /* Indicate we got on the network */
    setLed(ON_NETWORK_LED); 
    
    /* On network, display info about this network */
#ifdef DISPLAY_NETWORK_INFORMATION     
    displayNetworkConfigurationParameters();                
    displayDeviceInformation();
#else
    displayBasicDeviceInformation();
#endif
    
    HAL_ENABLE_INTERRUPTS();
    
    /* Now the network is running - send messages to the Coordinator.*/    
#define TEST_CLUSTER 0x77
    
#define MESSAGE_HEADER_LENGTH    13
#define MESSAGE_LENGTH  (TEST_MESSAGE_PAYLOAD_LENGTH + MESSAGE_HEADER_LENGTH)
    /* Here we precompute zmBuf contents so that we don't have to do it in the loop. This is faster.
    This is the equivalent of afSendData(DEFAULT_ENDPOINT,DEFAULT_ENDPOINT,0, TEST_CLUSTER, testMessage, 10); */
    uint8_t testBuf[MESSAGE_LENGTH];
    testBuf[0] = MESSAGE_LENGTH;
    testBuf[1] = MSB(AF_DATA_REQUEST);
    testBuf[2] = LSB(AF_DATA_REQUEST);      
    
    testBuf[3] = 0; 
    testBuf[4] = 0;
    testBuf[5] = DEFAULT_ENDPOINT;
    testBuf[6] = DEFAULT_ENDPOINT;
    testBuf[7] = LSB(TEST_CLUSTER); 
    testBuf[8] = MSB(TEST_CLUSTER); 
    testBuf[9] = 0xFF;  // Sequence: we don't care
    testBuf[10] = AF_MAC_ACK; //Could also use AF_APS_ACK;
    testBuf[11] = DEFAULT_RADIUS;
    testBuf[12] = TEST_MESSAGE_PAYLOAD_LENGTH; // Datalength
    //memcpy(testBuf+MESSAGE_HEADER_LENGTH, testMessage, TEST_MESSAGE_PAYLOAD_LENGTH);
    //testBuf is now loaded with our test message.        
    
    printf("!!  Sending %u messages  !!\r\n", NUMBER_OF_PACKETS_TO_SEND);    
    
    while (1)
    {
    	uint16_t packetCounter;
        for (packetCounter = 0; packetCounter<NUMBER_OF_PACKETS_TO_SEND; packetCounter++)
        {
            /* Copy our message over to zmBuf because zmBuf gets overwritten when the AF_DATA_CONFIRM is received */
            memcpy(zmBuf, testBuf, MESSAGE_LENGTH);
            
            /* Now initialize the payload */
            int index;
            for (index = MESSAGE_HEADER_LENGTH; index < (TEST_MESSAGE_PAYLOAD_LENGTH + MESSAGE_HEADER_LENGTH); index++)
            {
                testBuf[index] = index;
            }

            /* Send the message to the Coordinator */
            result = sendMessage();       
            if (result != MODULE_SUCCESS)
            {
                printf("afSendData Error %02X; stopping\r\n", result);
                while (1);
            }        
            
            /* Now, wait for the AF_DATA_CONFIRM to verify that the message was successfully sent*/
            while (!(MODULE_HAS_MESSAGE_WAITING()));
            
            /* Retrieve the AF_DATA_CONFIRM message */
            getMessage();
            
            if (!(IS_AF_DATA_CONFIRM())) 
            {
                /* Stop if we receive a different message */
                printf("Error; stopped after packet %u", packetCounter);
            }        
            
            toggleLed(1); 
            /* If you want to slow down the rate of sending packets then add:
            delayMs(1);
            which will add a one mSec delay after each packet is sent. */
            
            if (((packetCounter % 100) == 0) && (packetCounter != 0))
            {
                printf("%u\r\n", packetCounter);
            }
        }
        printf("Done! Sent %u packets!\r\nPress button to start again\r\n", NUMBER_OF_PACKETS_TO_SEND);        
        
        /* Wait until a button is pressed, then send another 1000 */
        while (!(buttonIsPressed(ANY_BUTTON)));
    }
}

/** When a button is pressed, display device information */
void handleButtonPress(int8_t whichButton)
{
#ifdef DISPLAY_NETWORK_INFORMATION     
    displayNetworkConfigurationParameters();                
    displayDeviceInformation();
#else
    displayBasicDeviceInformation();
#endif
}

/* @} */
