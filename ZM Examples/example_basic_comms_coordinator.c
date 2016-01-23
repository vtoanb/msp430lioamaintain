/**
* @ingroup moduleCommunications
* @{
*
* @file example_basic_comms_coordinator_afzdo.c
*
* @brief Resets Module, configures this device to be a Zigbee Coordinator, and displays any messages
* that are received. Uses the AF/ZDO interface.
*
* This matches example_basic_communications_ROUTER.c
* Get this running before the router, or else the router won't have anything to join to.
*
* $Rev: 1885 $
* $Author: dsmith $
* $Date: 2013-08-23 09:04:48 -0700 (Fri, 23 Aug 2013) $
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
#include "../ZM/af.h"
#include "../ZM/zdo.h"
#include "../ZM/module_errors.h"
#include "../ZM/module_utilities.h"
#include "../ZM/zm_phy.h"
#include "module_example_utils.h"
#include <stdint.h>
#include <stddef.h>

/** This will hold the result of various module functions. Define once here so we can reuse. */
moduleResult_t result;

/** function pointer (in hal file) for the function that gets called when a button is pressed*/
extern void (*debugConsoleIsr)(char);

/** handler for UART RX **/
void handleUARTRX(char rxchar);
void handleUARTRXref(char rxchar);
/** handle for receving command **/
uint8_t commandline[9] = {' ',' ',' ',' ',' ',' ',' ',' ',' '};
uint8_t commandlinecpy[9];
uint16_t zmMesAdd;
uint8_t zmReady = 0;
uint8_t commandlinecnt = 0;
uint8_t parsing_state = 0;

/** Set this to the part of the world where you are located to ensure compliance with FCC/ETSI etc. */
#define MODULE_REGION   MODULE_REGION_NORTH_AMERICA

int main( void )
{
    halInit();
    moduleInit();
    printf("\r\n****************************************************\r\n");
    printf("Basic Communications Example - COORDINATOR - using AFZDO\r\n");
    setLed(0);
    
#define MODULE_START_DELAY_IF_FAIL_MS 5000
#define TEST_CLUSTER 0x77

    /* Use the default module configuration */
    struct moduleConfiguration defaultConfiguration = DEFAULT_MODULE_CONFIGURATION_COORDINATOR;
    
    /* Change this if you are using a custom PAN */
    defaultConfiguration.panId = ANY_PAN;
    
    /* Try to start module with the default configuration */    
    while ((result = expressStartModule(&defaultConfiguration, GENERIC_APPLICATION_CONFIGURATION, MODULE_REGION)) != MODULE_SUCCESS)
    {
        /* Module startup failed; display error */
        printf("Module start unsuccessful. Error Code 0x%02X. Retrying...\r\n", result);
        
        /* Wait a few seconds before trying again, in case the rest of the network is restarting too */        
        delayMs(MODULE_START_DELAY_IF_FAIL_MS);
    }
    printf("Success\r\n"); 
    
    /* Assign handler for UART RX interrupt */
    debugConsoleIsr = &handleUARTRX;

    /* On network, so display info about this network */
#ifdef DISPLAY_NETWORK_INFORMATION     
    displayNetworkConfigurationParameters();                
    displayDeviceInformation();
#else
    displayBasicDeviceInformation();
#endif  
    printf("Displaying Received Messages...\r\n");
    HAL_ENABLE_INTERRUPTS();
    
    /* heartbeat counter; roughly once per second */
    uint32_t heartBeatCounter = 0;
#define HEARTBEAT_INTERVAL          ((GET_MCLK_FREQ()) / 128)
    while (1)
    {
        /* Continually loop, displaying any messages that are received */
        if (MODULE_HAS_MESSAGE_WAITING())
        {
            pollAndDisplay();
        }

        /* Simple heartbeat indicator to show that the application is doing something*/
        heartBeatCounter++;
        if ((heartBeatCounter % HEARTBEAT_INTERVAL) == 0)
        {
            toggleLed(1);
        }

        if(zmReady == 1){
        	zmReady = 0;
        	printf("sending command ...\r\n");
        	result = afSendData(DEFAULT_ENDPOINT,DEFAULT_ENDPOINT,\
        			zmMesAdd, TEST_CLUSTER, commandlinecpy, 9);
        	if(result == MODULE_SUCCESS){
        		printf("send command success !\r\n");
        	}
        	else{
        		printf("send command failed !\r\n");
        	}

        }
    }
}
#define PARSE_WAIT 0
#define PARSE_CMD 1
#define PARSE_MACHINE_NAME 2
#define PARSE_SHORT_ADD 3
#define PARSE_ENERGY 4
#define PARSE_PRODUCT 5
void handleUARTRX(char rxchar){
	int i;
	if(rxchar == '<'){
		commandlinecnt = 1;
	}
	else{
		if(commandlinecnt < 10 && commandlinecnt > 0){
			commandline[commandlinecnt - 1] = rxchar;
			commandlinecnt++;
		}
		if(commandlinecnt == 10){
			//get address
			zmMesAdd  = (uint16_t)(commandline[3] << 8) + commandline[4];
			//copy recevie buffer
			for (i=0;i<9;i++){
				commandlinecpy[i] = commandline[i];
			}
			zmReady   = 1;
		}

	}
}
void handleUARTRXref(char rxchar)
{
	uint16_t shortadd;
	switch(parsing_state){
	case PARSE_WAIT:
		if(rxchar == '<'){
		parsing_state = PARSE_CMD;
		}
		break;
	case PARSE_CMD:
		if(rxchar > '0' && rxchar < '9' && rxchar != '<'){
			commandline[0] = rxchar;
			// because command just 1 byte, next state
			parsing_state = PARSE_MACHINE_NAME;
			// go to machine name holder
			commandlinecnt = 1;
		}
		else{
			commandlinecnt = 0;
			parsing_state = PARSE_WAIT;
		}
		break;
	case PARSE_MACHINE_NAME:
		if((rxchar != '<') &&\
		   ((rxchar >= '0' && rxchar <= '9') ||\
		    (rxchar >= 'A' && rxchar <= 'Z'))&&\
			(commandlinecnt == 1 || commandlinecnt == 2)){
				commandline[commandlinecnt] = rxchar;
				commandlinecnt++;
				if(commandlinecnt==3){
					parsing_state = PARSE_SHORT_ADD;
				}
		}
		else{
			parsing_state = PARSE_WAIT;
			commandlinecnt = 0;
		}
		break;
	case PARSE_SHORT_ADD:
		if((commandlinecnt == 3 || commandlinecnt == 4)&&\
				(rxchar != '<')){
			//PARSING ROUTER SHORT ADDRESS
			commandline[commandlinecnt] = rxchar;
			commandlinecnt++;
			if(commandlinecnt==5){
				parsing_state = PARSE_ENERGY;
			}
		}
		else{
			parsing_state = PARSE_WAIT;
			commandlinecnt = 0;
		}
		break;
	case PARSE_ENERGY:
		if((rxchar != '<') && (commandlinecnt == 5 || commandlinecnt == 6)){
			commandline[commandlinecnt] = rxchar;
			commandlinecnt++;
			if(commandlinecnt == 7){
				parsing_state = PARSE_PRODUCT;
			}
		}
		else{
			parsing_state = PARSE_WAIT;
			commandlinecnt= 0;
		}
		break;
	case PARSE_PRODUCT:
		if((rxchar != '<') && commandlinecnt == 7 || commandlinecnt == 8){
			commandline[commandlinecnt] = rxchar;
			commandlinecnt++;
			if(commandlinecnt == 9){
				parsing_state = PARSE_WAIT;
				commandlinecnt = 0;
				/*parsing complete, do send this command*/
				shortadd = commandline[3] << 8 + commandline[4];
		        result = afSendData(DEFAULT_ENDPOINT,DEFAULT_ENDPOINT,shortadd, TEST_CLUSTER, commandline, 9);
		        toggleLed(1);
			}
		}
		else{
			parsing_state = PARSE_WAIT;
			commandlinecnt= 0;
		}
		break;
	default:
		parsing_state = PARSE_WAIT;
		commandlinecnt = 0;
		break;

	}
//    printf("%c\r\n",rxchar);
}

/* @} */
