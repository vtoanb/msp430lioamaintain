/**
*
* @file configRequestMessage.c
*
* @brief message to ask server or other device for new configuration data.
* 
* Config Request Message Cluster = 0x0002
*
* $Rev: 1972 $
* $Author: dsmith $
* $Date: 2013-11-25 17:48:16 -0800 (Mon, 25 Nov 2013) $
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


#include "configRequestMessage.h"
#include "../../HAL/hal.h"
#include "../../Common/utilities.h"

/** 
Display the contents of the message to console 
@param crm the message to display
*/
void displayConfigRequestMessage(struct configRequestMessage* crm)
{
    printf("CReq ");
    printHeader(&(crm->header));
    printf(", Dev Type/SubType=%02X/%02X, FW = %u\r\n", crm->deviceType, crm->deviceSubType, crm->firmwareVersion);
}


/** 
Converts the message into a stream of bytes to memory pointed to by destinationPtr
Number of bytes streamed is equal to getSizeOfConfigRequestMessage() 
@param crm the message to serialize
@param destinationPtr where to copy the stream of bytes to
@pre destinationPtr points to a region of memory at least getSizeOfConfigRequestMessage() bytes in size
*/
void serializeConfigRequestMessage(struct configRequestMessage* crm, unsigned char* destinationPtr)
{  
  serializeHeader((&(crm->header)), destinationPtr);
  destinationPtr += (HEADER_SIZE);
  *destinationPtr++ = crm->deviceType;
  *destinationPtr++ = crm->deviceSubType;
  *destinationPtr++ = (crm->firmwareVersion) & 0xFF;   //LSB of value
  *destinationPtr++ = (crm->firmwareVersion) >> 8;          //MSB of value
  *destinationPtr++ = (crm->reserved1) & 0xFF;   //LSB of value
  *destinationPtr++ = (crm->reserved1) >> 8;          //MSB of value  
  *destinationPtr++ = (crm->reserved2) & 0xFF;   //LSB of value
  *destinationPtr++ = (crm->reserved2) >> 8;          //MSB of value  
  *destinationPtr++ = (crm->reserved3) & 0xFF;   //LSB of value
  *destinationPtr++ = (crm->reserved3) >> 8;          //MSB of value 
}

/** 
Creates a Config Request message from the stream of bytes starting at source
@param source the beginning of the serialized Info Message
@param req the Message to create
@return the number of bytes deserialized if success, or else an error code < 0
*/
int16_t deserializeConfigRequestMessage(uint8_t* source, struct configRequestMessage* req)
{
    uint8_t* sourcePtr = source;    
    req->header = deserializeHeader(sourcePtr);    // First, deserialize the header
    sourcePtr += (HEADER_SIZE);
    req->deviceType = *sourcePtr++;                // Get the deviceType
    req->deviceSubType = *sourcePtr++;                // Get the deviceSubType 
    req->firmwareVersion = CONVERT_TO_INT( (*sourcePtr), (*(sourcePtr+1)) );
    sourcePtr += 2;
    
    req->reserved1 = CONVERT_TO_INT( (*sourcePtr), (*(sourcePtr+1)) );
    sourcePtr += 2;
    req->reserved2 = CONVERT_TO_INT( (*sourcePtr), (*(sourcePtr+1)) );
    sourcePtr += 2;
    req->reserved3 = CONVERT_TO_INT( (*sourcePtr), (*(sourcePtr+1)) );
    sourcePtr += 2;

    return (sourcePtr - source);
}


/** Gets the size of this info message.
@param im the infoMessage to get the size of
@return the size of the Info Message, including all KVPs.
This is implemented as a method (not macro) for consistency with other messages.
*/
uint16_t getSizeOfConfigRequestMessage(struct configRequestMessage* im)
{ 
  return (HEADER_SIZE) + 10;
}

