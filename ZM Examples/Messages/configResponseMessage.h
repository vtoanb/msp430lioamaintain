/**
*
* @file configResponseMessage.h
*
* @brief Public methods for configResponseMessage.c
*
* $Rev: 1914 $
* $Author: dsmith $
* $Date: 2013-09-26 15:59:58 -0700 (Thu, 26 Sep 2013) $
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


#include "header.h"
#include "kvp.h"

#ifndef CONFIG_RESPONSE_MESSAGE_H
#define CONFIG_RESPONSE_MESSAGE_H

/**
* CONFIG RESPONSE MESSAGE
* FROM: Device
* TO: Server
* RESPONSE: none
*
* Contains new configuration information
*/
#define MAX_KVPS_IN_CONFIG_RESPONSE_MESSAGE          4

#define CONFIG_RESPONSE_MESSAGE_CLUSTER              0x03
#define CONFIG_RESPONSE_MESSAGE_VERSION              0x03
//#define CONFIG_RESPONSE_MESSAGE_FLAGS_NONE           0x00
//#define CONFIG_RESPONSE_MESSAGE_SERVER_ACK_REQUESTED 0x01

//Placeholder for when we're not sending a full set of parameters
//#define EMPTY_PARAMETER                             0xFFFC

/** A Config Response Message, containing the header, various fields, and parameters.
*/
struct configResponseMessage   
{
  struct header header;
  uint16_t responseSequence;    //2
  uint16_t reserved1;           //2
  uint32_t timestamp;           //4     
  uint8_t reserved2;            //1
  uint8_t numParameters;        //1  
  struct kvp kvps[MAX_KVPS_IN_CONFIG_RESPONSE_MESSAGE];
};

void displayConfigResponseMessage(struct configResponseMessage* im);
void serializeConfigResponseMessage(struct configResponseMessage* crm, uint8_t* destination);
int16_t deserializeConfigResponseMessage(uint8_t* source, struct configResponseMessage* crm);
uint16_t getSizeOfConfigResponseMessage(struct configResponseMessage* crm);


#define MAX_CONFIG_RESPONSE_MESSAGE_SIZE   (HEADER_SIZE + 10 + (MAX_KVPS_IN_CONFIG_RESPONSE_MESSAGE * 3))

#endif
