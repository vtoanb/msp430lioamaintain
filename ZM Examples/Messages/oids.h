/**
* @file oids.h
*
* @brief description of Object Identifiers (OIDs). These are used in key-value-pairs in status messages, etc.
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


#ifndef OIDS_H
#define OIDS_H

#include <stdint.h>

#define OID_LENGTH_BYTES                2

char* getOidName(uint8_t oid);
void displayFormattedOidValue(uint8_t oid, int16_t value);

//
//  General OIDs - start with 0x0n
//  Reminder - if using the S2S Dev Kit, these must match up with the server's parameters
//

/** Supply voltage, in mV */
#define OID_SUPPLY_VOLTAGE_MV                       0x02

//
//  Temperature OIDs - start with 0x1n
//  Reminder - if using the S2S Dev Kit, these must match up with the server's parameters
//

/** Temperature of the PCB */
#define OID_TEMPERATURE_LOCAL                       0x11

/** Temperature of an external attached sensor */
#define OID_TEMPERATURE_IR                          0x12

//
//  Light Sensor OIDs - start with 0x2n
//  Reminder - if using the S2S Dev Kit, these must match up with the server's parameters
//

/** Generic Light sensor reading, in Lux */
#define OID_LIGHT_SENSOR_AMBIENT_LUX                0x21

/** Color sensor reading, red channel */
#define OID_COLOR_SENSOR_RED                        0x22

/** Color sensor reading, blue channel */
#define OID_COLOR_SENSOR_BLUE                       0x23

/** Color sensor reading, green channel */
#define OID_COLOR_SENSOR_GREEN                      0x24

/** Color sensor reading, clear channel */
#define OID_COLOR_SENSOR_CLEAR                      0x25

/** Zigbee Module Product ID */
#define OID_MODULE_PRODUCT_ID                       0x06

/** Zigbee Module Firmware Major */
#define OID_MODULE_FIRMWARE_MAJOR                   0x07

/** Zigbee Module Firmware Minor */
#define OID_MODULE_FIRMWARE_MINOR                   0x08

/** Zigbee Module Firmware Build */
#define OID_MODULE_FIRMWARE_BUILD                   0x09

//
//  See server for more parameters. We make use of some pre-defined parameters
//

/** How often staus messages are sent, in seconds */
#define OID_STATUS_MESSAGE_INTERVAL                 0xAC

/** How often config request messages are sent, in seconds */
#define OID_CONFIG_REQUEST_MESSAGE_INTERVAL         0xAD


//
// Outputs
//
#define OID_RED_LED                                 0x61

#define OID_BLUE_LED                                0x62

#define OID_GREEN_LED                               0x63

#endif
