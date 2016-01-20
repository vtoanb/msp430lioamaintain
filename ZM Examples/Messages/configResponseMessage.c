/**
*
* @file configResponseMessage.c
*
* @brief message containing configuration information.
* 
* Info Message Cluster = 0x0003
*
* $Rev: 1917 $
* $Author: dsmith $
* $Date: 2013-09-26 17:43:10 -0700 (Thu, 26 Sep 2013) $
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


#include "configResponseMessage.h"
#include "../../HAL/hal.h"
#include "../../Common/utilities.h"

#define INCLUDE_RESERVED_PARAMS

/** 
Display the contents of the configResponseMessage to console 
@param crm the Message to display
*/
void displayConfigResponseMessage(struct configResponseMessage* crm)
{
    printf("CRes ");
    printHeader(&(crm->header));
    printf(", responseSequence=%u, timestamp=0x%04X|%04X, numParameters=%u: ", crm->responseSequence,
           (uint16_t)(crm->timestamp >> 16), (uint16_t)(crm->timestamp & 0xFFFF), crm->numParameters);
    uint_fast8_t parameter = 0;
    for (parameter=0; parameter < crm->numParameters; parameter++)
    {
        printf("[%02X=%04X] ", crm->kvps[parameter].oid, crm->kvps[parameter].value);
    }
#ifdef INCLUDE_RESERVED_PARAMS
    printf("reserved1=0x%04X, reserved2=0x%02X", crm->reserved1, crm->reserved2);
#endif    
    
    printf("\r\n");
}


/** 
Creates a config response message from the stream of bytes starting at source
@param source the beginning of the serialized Message
@param configResponseMessage the Message to create
@return the number of bytes deserialized if success, or else an error code < 0
*/
int16_t deserializeConfigResponseMessage(uint8_t* source, struct configResponseMessage* crm)
{
    uint8_t* sourcePtr = source;    
    crm->header = deserializeHeader(sourcePtr);    // First, deserialize the header
    sourcePtr += (HEADER_SIZE);
    crm->responseSequence = CONVERT_TO_INT( (*sourcePtr), (*(sourcePtr+1)) );
            sourcePtr += 2;
    crm->reserved1 = CONVERT_TO_INT( (*sourcePtr), (*(sourcePtr+1)) );  
            sourcePtr += 2;
    uint16_t lsw = CONVERT_TO_INT( (*sourcePtr), (*(sourcePtr+1)) );  
            sourcePtr += 2;
    uint16_t msw = CONVERT_TO_INT( (*sourcePtr), (*(sourcePtr+1)) );  
            sourcePtr += 2;

    crm->timestamp = (((uint32_t)msw) << 16) | (((uint32_t) lsw));
    crm->reserved2 = *sourcePtr++;             // ... and the number of Parameters    
    crm->numParameters = *sourcePtr++;             // ... and the number of Parameters
    if (crm->numParameters > MAX_KVPS_IN_CONFIG_RESPONSE_MESSAGE)
        return -1;
    int i;
    for (i=0; i < crm->numParameters; i++)  //for each Parameter:
    {
        crm->kvps[i].oid = *sourcePtr++;
        crm->kvps[i].value = CONVERT_TO_INT( (*sourcePtr), (*(sourcePtr+1)) );
        sourcePtr += 2;
    }
    return (sourcePtr - source);
}


/** 
Converts the message into a stream of bytes to memory pointed to by destinationPtr
Number of bytes streamed is equal to getSizeOfConfigResponseMessage() 
@param crm the message to serialize
@param destination where to copy the stream of bytes to
@pre destination points to a region of memory at least getSizeOfConfigResponseMessage() bytes in size
@return number of bytes serialized
*/
void serializeConfigResponseMessage(struct configResponseMessage* crm, uint8_t* destination)
{  
  uint8_t *destinationPtr = destination;
    
  serializeHeader((&(crm->header)), destinationPtr);
  destinationPtr += (HEADER_SIZE);
  *destinationPtr++ = (crm->responseSequence) & 0xFF;   
  *destinationPtr++ = (crm->responseSequence) >> 8;           
  *destinationPtr++ = (crm->reserved1) & 0xFF;          
  *destinationPtr++ = (crm->reserved1) >> 8;             
  
  uint32_t ts = crm->timestamp;
  uint8_t val = (uint8_t) (ts & 0x000000FF);
  *destinationPtr++ = val;
  ts >>= 8;
  val = (uint8_t) (ts & 0x000000FF);
  *destinationPtr++ = val; 
  ts >>= 8;
  val = (uint8_t) (ts & 0x000000FF);
  *destinationPtr++ = val; 
  ts >>= 8;
  val = (uint8_t) (ts & 0x000000FF);
  *destinationPtr++ = val;
  
  *destinationPtr++ = crm->reserved2;
  *destinationPtr++ = crm->numParameters;
    int i;
    for (i=0; i < crm->numParameters; i++)  //for each Parameter:
    {
          *destinationPtr++ = crm->kvps[i].oid;
          *destinationPtr++ = (crm->kvps[i].value) & 0xFF;          
          *destinationPtr++ = ( crm->kvps[i].value) >> 8;    
    }
    //return (destinationPtr - destination);
}


/** Gets the size of this message.
@param crm the Message to get the size of
@return the size of the Message, including all KVPs.
This is implemented as a method (not macro) for consistency with other messages.
*/
uint16_t getSizeOfConfigResponseMessage(struct configResponseMessage* crm)
{ 
  return (HEADER_SIZE) + 10 + ((crm->numParameters) * SIZE_OF_KVP_IN_BYTES);
}
