/**
*
* @file oids.c
*
* @brief Utility methods for object identifiers (oids)
*
* Of course you will probably want to change these for your own use. If you are implementing a
* sensor-to-server system then be sure that these match the OIDS table in your server.
*
* $Rev: 1798 $
* $Author: dsmith $
* $Date: 2013-04-22 03:03:14 -0700 (Mon, 22 Apr 2013) $
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

#include "oids.h"
#include "../../HAL/hal.h"

/** 
Gets the name of the OID.
@param oid the Object Identifier to look up.
@return The name of the Object Identifier (oid) in human readable format, or "UNKNOWN" if not known.
*/
char* getOidName(uint8_t oid)
{
    switch (oid)
    {
    case OID_SUPPLY_VOLTAGE_MV:
        return "SUPPLY_VOLTAGE_MV";
    case OID_TEMPERATURE_LOCAL:
        return "TEMPERATURE_LOCAL";
    case OID_TEMPERATURE_IR:
        return "TEMPERATURE_IR";
    case OID_LIGHT_SENSOR_AMBIENT_LUX:
        return "LIGHT_SENSOR_AMBIENT_LUX";
    case OID_COLOR_SENSOR_RED:
        return "COLOR_SENSOR_RED";
    case OID_COLOR_SENSOR_BLUE:
        return "COLOR_SENSOR_BLUE";
    case OID_COLOR_SENSOR_GREEN:
        return "COLOR_SENSOR_GREEN";
    case OID_COLOR_SENSOR_CLEAR:
        return "COLOR_SENSOR_CLEAR";

        //new:
    case OID_STATUS_MESSAGE_INTERVAL:
        return "STATUS_MESSAGE_INTERVAL";
    case OID_CONFIG_REQUEST_MESSAGE_INTERVAL:
        return "CONFIG_REQUEST_MESSAGE_INTERVAL";

    case OID_MODULE_PRODUCT_ID:
        return "MODULE_PRODUCT_ID";
    case OID_MODULE_FIRMWARE_MAJOR:
        return "MODULE_FIRMWARE_MAJOR";
    case OID_MODULE_FIRMWARE_MINOR:
        return "MODULE_FIRMWARE_MINOR";
    case OID_MODULE_FIRMWARE_BUILD:
        return "MODULE_FIRMWARE_BUILD";



    default:
        return "UNKNOWN";
    }
}

/**
Gets the name of the OID.
@param oid the Object Identifier to look up.
@return The name of the Object Identifier (oid) in human readable format, or "UNKNOWN" if not known.
*/
void displayFormattedOidValue(uint8_t oid, int16_t value)
{
    switch (oid)
    {
    case OID_SUPPLY_VOLTAGE_MV:
        printf("(%d.%03dV)", value/1000, value%1000);
        return;
    case OID_TEMPERATURE_LOCAL:
        printf("(%d.%02dC)", value/100, value%100);
        return;
    case OID_TEMPERATURE_IR:
        printf("(%d.%02dC)", value/100, value%100);
        return;
    case OID_LIGHT_SENSOR_AMBIENT_LUX:
        printf("(%dLux)", value);
        return;

        //new
    case OID_STATUS_MESSAGE_INTERVAL:
        /* fall-through */
    case OID_CONFIG_REQUEST_MESSAGE_INTERVAL:
        printf("(%dSec)", value);
        return;

        // Zigbee module OIDs; just display as hex
    case OID_MODULE_PRODUCT_ID:
        /* fall-through */
    case OID_MODULE_FIRMWARE_MAJOR:
        /* fall-through */
    case OID_MODULE_FIRMWARE_MINOR:
        /* fall-through */
    case OID_MODULE_FIRMWARE_BUILD:
        printf("(%02X)", value);
        return;


    default:        // Don't display anything
        return;
    }
}
