/**
*  @file set.h
*
*  @brief  public methods for set.c
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

#ifndef SET_H
#define SET_H
#include <stdint.h>

#define MAX_ELEMENTS_IN_SET 20       // the number of short addresses to keep in the list

typedef struct {
    /** The current size of the Set. Also is the index where the next value will be placed. */
    uint8_t size;

    /** The elements in the Set. */
    uint16_t values[MAX_ELEMENTS_IN_SET];

} set;

void initSet(set* S);
uint8_t setContains(set* S, uint16_t value);
uint8_t addToSet(set* S, uint16_t value);
void displaySet(set* S);

#endif
