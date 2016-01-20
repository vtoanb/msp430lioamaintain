/**
* @file set.c
*
* @brief Simple Set abstract data type
*
* @see http://en.wikipedia.org/wiki/Set_%28computer_science%29
* @see http://docs.oracle.com/javase/7/docs/api/java/util/Set.html
*
* $Rev: 2063 $
* $Author: dsmith $
* $Date: 2014-03-06 10:13:04 -0800 (Thu, 06 Mar 2014) $
*
* @section example1 Set Creation
* The set must be declared and initialized before using
<pre>
    set exampleSet;
    initSet(&exampleSet);
</pre>
*
* @section example2 Adding a value to a Set
* Assuming exampleSet has already been initialized as above, adding a value is easy:
<pre>
    addToSet(&searchedSet, shortAddress);
</pre>
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
#include "set.h"
#include "../HAL/hal.h" // Needed for printf


/** 
 * Initializes the set by setting the size to zero.
 * @param S the set to initialize
 * @post the set is ready to be used.
 */
void initSet(set* S)
{
    S->size = 0;
}


/** 
 * Searches for the value in the Set.
 * @param S the set to search
 * @param value the value to find
 * @return 1 if in Set, 0 if not in Set
 */
uint8_t setContains(set* S, uint16_t value)
{
    int i = 0;
    for (i=0; i < (S->size); i++)
    {
        if (value == S->values[i])
        {
            return 1;
        }
    }
    return 0;   // We've searched through the whole Set but haven't found the value
}


/**
 * Whether the Set is full or not
 * @param S the Set to search
 * @return 1 if the Set is full, or 0 if the Set can still accept more entries
 */
uint8_t setIsFull(set* S)
{
    return (S->size == MAX_ELEMENTS_IN_SET);
}


/** 
 * Adds the value to the Set; specifically inserts the value at current index and increments index.
 * @param S the set to add the value to
 * @param value the value to add
 * @return 1 if add was successful or the value was already in the Set; or 0 if there's no more room.
 */
uint8_t addToSet(set* S, uint16_t value)
{
    if ((setContains(S, value)) == 1)   // First see if this value is already in the list
    {
        return 1;                       // Stop now; value is already in the list so we don't need to insert it
    }
        
    if (setIsFull(S))                   // Check to see if there is room in the Set for a new entry
    {
        return 0;                       // No room at the inn
    }
    
    S->values[S->size] = value;             // Add the value to the Set
    S->size++;                              // And increment the setSize
    return 1;
}


/** Displays the size and values of the Set */
void displaySet(set* S)
{
    printf("Set has %u elements: ", S->size);
    int i;
    for (i = 0; i < S->size; i++)
    {
        printf("%04X ", S->values[i]);
    }
    printf("\r\n");
}

