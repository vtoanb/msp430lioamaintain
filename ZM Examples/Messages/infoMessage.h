/**
*
* @file infoMessage.h
*
* @brief Public methods for infoMessage.c
*
* $Rev: 1899 $
* $Author: dsmith $
* $Date: 2013-09-14 18:09:07 -0700 (Sat, 14 Sep 2013) $
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

#ifndef INFO_MESSAGE_H
#define INFO_MESSAGE_H

#include "header.h"
#include "kvp.h"
#include <stdint.h>

/**
* INFO MESSAGE
* FROM: Device
* TO: Server
* RESPONSE: none
*
* Contains device information, like manufacturer, model, etc.
*/
#define MAX_KVPS_IN_STATUS_MESSAGE                  12

#define INFO_MESSAGE_CLUSTER                        0x07
#define INFO_MESSAGE_VERSION                        0x03
#define INFO_MESSAGE_FLAGS_NONE                     0x00
#define INFO_MESSAGE_SERVER_ACK_REQUESTED           0x01

#define DEVICETYPE_TESLA_CONTROLS_SIMPLE_DEMO       0x01  // Old versions
#define DEVICETYPE_TESLA_CONTROLS_ROUTER_DEMO       0x02
#define DEVICETYPE_TESLA_CONTROLS_END_DEVICE_DEMO   0x03

//Placeholder for when we're not sending a full set of parameters
#define EMPTY_PARAMETER                             0xFFFC

/** An Info Message, containing the header, various fields, and parameters.
*/
struct infoMessage   
{
  struct header header;
  uint8_t deviceType; 
  uint8_t numParameters;
  struct kvp kvps[MAX_KVPS_IN_STATUS_MESSAGE];
};
int8_t addKvpToInfoMessage(struct infoMessage* im, struct kvp *kvp);
void printInfoMessage(struct infoMessage* im);
void serializeInfoMessage(struct infoMessage* im, uint8_t* destinationPtr);
int16_t deserializeInfoMessage(uint8_t* source, struct infoMessage* info);
uint16_t getSizeOfInfoMessage(struct infoMessage* im);


#define MAX_INFO_MESSAGE_SIZE   (HEADER_SIZE + 2 +(MAX_KVPS_IN_STATUS_MESSAGE * SIZE_OF_KVP_IN_BYTES))

#endif
