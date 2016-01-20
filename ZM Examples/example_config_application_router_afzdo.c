/**
* @ingroup apps
* @{
*
* @file example_config_application_router_afzdo.c
*
* @brief Resets Module, configures this device to be a Zigbee Router, joins a network, then sends a 
* message to the coordinator periodically. Also periodically requests new configuration data from
* the Coordinator. 
* 
* Demonstrates how to implement two-way control of a remote node, and also how to do both send and
* receive of messages in the same state machine.
*
* If you need assistance we're here to help; see below for support options. Tesla has extensive
* experience with sensor-to-server applications and everything inbetween.
*
* @note This matches example_config_application_coordinator.c or can just be used with the Gateway
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
#include "../ZM/application_configuration.h"
#include "../ZM/af.h"
#include "../ZM/zdo.h"
#include "../ZM/module_errors.h"
#include "../ZM/module_utilities.h"
#include "../ZM/zm_phy.h"
#include "../Common/utilities.h"
#include "Messages/infoMessage.h"
#include "Messages/configRequestMessage.h"
#include "Messages/configResponseMessage.h"     
#include "Messages/kvp.h"
#include "Messages/oids.h"
#include "module_example_utils.h"
#include <stdint.h>
#include <string.h>  

extern uint8_t zmBuf[ZIGBEE_MODULE_BUFFER_SIZE];

/* An application-level sequence number to track acknowledgements from server */
uint16_t sequenceNumber = 0;  

/** The number of failed messages before initiating a network restart */
uint8_t failCount = 0;

/** Simple interval timer for info messages. Gets incremented by the SysTick ISR. */
volatile uint16_t infoMessageTimer = 0;

/** Simple interval timer for config messages. Gets incremented by the SysTick ISR. */
volatile uint16_t configMessageTimer = 0;

/** function pointer (in hal file) for the function that gets called when the systick generates an int*/
extern void (*sysTickIsr)(void);

/** STATES for state machine */
enum STATE
{
    STATE_IDLE,
    STATE_MODULE_STARTUP,
    STATE_SEND_INFO_MESSAGE,
    STATE_SEND_CONFIG_REQUEST_MESSAGE,
    STATE_DISPLAY_NETWORK_INFORMATION,
    STATE_MESSAGE_SEND_SUCCESS,
    STATE_MESSAGE_SEND_FAILURE
};

/** This is the current state of the application. 
Gets changed by other states, or based on messages that arrive. */
enum STATE state = STATE_MODULE_STARTUP;

/** The main application state machine */
void stateMachine();

/** Various flags between states */
uint16_t stateFlags = 0;
#define STATE_FLAG_SEND_INFO_MESSAGE            0x01
#define STATE_FLAG_SEND_CONFIG_MESSAGE          0x02
#define INFO_MESSAGE_STATE_FLAG_IS_SET()        (stateFlags & STATE_FLAG_SEND_INFO_MESSAGE)
#define CONFIG_MESSAGE_STATE_FLAG_IS_SET()      (stateFlags & STATE_FLAG_SEND_CONFIG_MESSAGE)
//#define CLEAR_STATE_FLAGS()                     (stateFlags = 0)
#define CLEAR_SEND_CONFIG_MESSAGE_STATE_FLAG()  (stateFlags &= ~STATE_FLAG_SEND_CONFIG_MESSAGE)
#define CLEAR_SEND_INFO_MESSAGE_STATE_FLAG()    (stateFlags &= ~STATE_FLAG_SEND_INFO_MESSAGE)
#define SET_SEND_INFO_MESSAGE_FLAG()            (stateFlags |= STATE_FLAG_SEND_INFO_MESSAGE)
#define SET_SEND_CONFIG_MESSAGE_FLAG()          (stateFlags |= STATE_FLAG_SEND_CONFIG_MESSAGE)

/** Handles systick interrupt */
void sysTick();

/** How often to send an info message */
#define INFO_MESSAGE_INTERVAL_MS            3000

/** How often to check for new configuration*/
#define CONFIG_MESSAGE_INTERVAL_MS          7000

#define NWK_OFFLINE                         (0)
#define NWK_ONLINE                          (1)
/** Whether the zigbee network has been started, etc.*/
uint8_t zigbeeNetworkStatus = NWK_OFFLINE;
#define NETWORK_IS_ONLINE()                 (zigbeeNetworkStatus & NWK_ONLINE)
#define NETWORK_IS_OFFLINE()                ((~zigbeeNetworkStatus) & NWK_ONLINE)
#define SET_NETWORK_STATUS_ONLINE()         (zigbeeNetworkStatus |= NWK_ONLINE)
#define SET_NETWORK_STATUS_OFFLINE()        (zigbeeNetworkStatus &= ~NWK_ONLINE)

struct header hdr;


int main( void )
{
    halInit();
    moduleInit();
    printf("\r\n****************************************************\r\n");    
    printf("Config Application Example - ROUTER\r\n");
    
    uint16_t vlo = calibrateVlo();
    printf("VLO = %u Hz\r\n", vlo);   
    clearLeds();
    halRgbLedPwmInit();
    sysTickIsr = &sysTick;
    HAL_ENABLE_INTERRUPTS();
    
    /* Create the infoMessage header. Most of these fields are the same, so we can pre-populate most fields.
    These fields are entirely optional, and are just used for our application. This header is based
    on what we use for typical Zigbee deployments. See header.h for description of header fields. */
    hdr.sequence = 0;
    hdr.version = INFO_MESSAGE_VERSION;
    hdr.flags = INFO_MESSAGE_SERVER_ACK_REQUESTED; //INFO_MESSAGE_FLAGS_NONE;
    
#ifdef INCLUDE_SENSOR_PARAMETERS
    initializeSensors();
#endif    
    initSysTick();
    
    delayMs(100);
    stateMachine();    //run the state machine
}

/** Processes a config response message
@pre zmBuf contains a valid Config Response message
@post RGB LED is updated with values in message
*/
void processConfigResponseMessage()
{
    printf("Config Response Message:\r\n");
    struct configResponseMessage crm;
    int numBytes = deserializeConfigResponseMessage(zmBuf + AF_INCOMING_MESSAGE_PAYLOAD_START_FIELD, &crm);
    displayConfigResponseMessage(&crm);
    printf("%d bytes deserialized\r\n", numBytes);
    
    uint8_t red=0; uint8_t blue=0; uint8_t green=0;
    
    uint_fast8_t parameter = 0;
    for (parameter=0; parameter < crm.numParameters; parameter++)
    {
        printf("{%02X=%04X} ", crm.kvps[parameter].oid, crm.kvps[parameter].value);
        if (crm.kvps[parameter].oid == OID_RED_LED)
        {
            red = crm.kvps[parameter].value;
        } else if (crm.kvps[parameter].oid == OID_BLUE_LED) {
            blue = crm.kvps[parameter].value;
        } else if (crm.kvps[parameter].oid == OID_GREEN_LED) {
            green = crm.kvps[parameter].value;
        }                            
    }
    
    halRgbSetLeds(red, blue, green);  
}


void stateMachine()
{
    while (1)
    {
        if (NETWORK_IS_ONLINE())
        {
            if(moduleHasMessageWaiting())      //wait until SRDY goes low indicating a message has been received.   
            {
                getMessage();                
                if ((IS_AF_INCOMING_MESSAGE()) && ((AF_INCOMING_MESSAGE_CLUSTER()) == CONFIG_RESPONSE_MESSAGE_CLUSTER))
                {
                    processConfigResponseMessage();  
                }                
                displayMessage();
            }
        }
        
        switch (state)
        {
        case STATE_IDLE:
            /* This is the default state, and where we will be spending most time. */
            {
                if (INFO_MESSAGE_STATE_FLAG_IS_SET())  
                {
                    state = STATE_SEND_INFO_MESSAGE;           
                    CLEAR_SEND_INFO_MESSAGE_STATE_FLAG();
                }
                if (CONFIG_MESSAGE_STATE_FLAG_IS_SET())  
                {
                    state = STATE_SEND_CONFIG_REQUEST_MESSAGE;    
                    CLEAR_SEND_CONFIG_MESSAGE_STATE_FLAG();
                }                
                /* Other flags (for different messages or events) can be added here */                
            }
            break;            
            
        case STATE_MODULE_STARTUP:
            /* Start the module on the network */
            {
#define MODULE_START_DELAY_IF_FAIL_MS 3000      // How long to wait between network start attempts
#define MAX_FAILED_MESSAGES_BEFORE_RESTART  1   // Number of times to attempt auto-start before doing a full start                
                moduleResult_t result;
                struct moduleConfiguration defaultConfiguration = DEFAULT_MODULE_CONFIGURATION_ROUTER;
                
                /* Change this if you wish to use a custom PAN */
                defaultConfiguration.panId = ANY_PAN;
                
                /* Example of how to use a custom channel:
                printf("DEMO - USING CUSTOM CHANNEL 25\r\n");
                defaultConfiguration.channelMask = CHANNEL_MASK_25; */
                
                /* Change this below to be your operating region - MODULE_REGION_NORTH_AMERICA or MODULE_REGION_EUROPE */
#define OPERATING_REGION    (MODULE_REGION_NORTH_AMERICA) // or MODULE_REGION_EUROPE
                
                while ((result = expressStartModule(&defaultConfiguration, GENERIC_APPLICATION_CONFIGURATION, OPERATING_REGION)) != MODULE_SUCCESS)
                {
                    SET_NETWORK_FAILURE_LED_ON();          // Turn on the LED to show failure
                    printf("FAILED. Error Code 0x%02X. Retrying...\r\n", result);
                    delayMs(MODULE_START_DELAY_IF_FAIL_MS/2);                    
                    SET_NETWORK_FAILURE_LED_OFF();
                    delayMs(MODULE_START_DELAY_IF_FAIL_MS/2);
                }
                
                INIT_BOOSTER_PACK_LEDS();                
                SET_NETWORK_LED_ON();
                SET_NETWORK_FAILURE_LED_OFF();
                
                printf("Module Start Complete\r\n"); 
                SET_NETWORK_STATUS_ONLINE();
                /* Module Initialized so we can now store the module's MAC Address in the header */
                zbGetDeviceInfo(DIP_MAC_ADDRESS);
                memcpy(hdr.mac, zmBuf+SRSP_DIP_VALUE_FIELD, 8);
                
                /* Clear any message flags: */
                //CLEAR_STATE_FLAGS();
                CLEAR_SEND_INFO_MESSAGE_STATE_FLAG();
                CLEAR_SEND_CONFIG_MESSAGE_STATE_FLAG();
                
                state = STATE_DISPLAY_NETWORK_INFORMATION;
            }
            break;
            
        case STATE_DISPLAY_NETWORK_INFORMATION:
            {
                printf("~ni~");
                /* On network, display info about this network. Note that if we used auto-start then
                this will be the stored network configuration, which we will need to verify by sending a message */ 
                displayNetworkConfigurationParameters();                
                displayDeviceInformation();
                state = STATE_SEND_INFO_MESSAGE; 
            }
            break;  
            
        case STATE_SEND_INFO_MESSAGE:
            {
                if (NETWORK_IS_ONLINE())
                {                       
                    printf("~im~");
                    struct infoMessage im;
                    /* See infoMessage.h for description of these info message fields.*/
                    im.header = hdr;    // deep copy our header into our message
                    im.deviceType = DEVICETYPE_TESLA_CONTROLS_CFG_ROUTER_DEMO;
                    hdr.sequence = sequenceNumber++;
                    
#ifdef INCLUDE_SENSOR_PARAMETERS
                    im.numParameters = getSensorValues(im.kvps);    // Loads infoMessage with sensor value KVPs and gets the number of them
#else
                    im.numParameters = 0;
#endif                
                    // now, add status message interval
                    im.kvps[im.numParameters].oid = OID_STATUS_MESSAGE_INTERVAL;
                    im.kvps[im.numParameters].value = (INFO_MESSAGE_INTERVAL_MS/1000);
                    im.numParameters++;
                    
                    im.kvps[im.numParameters].oid = OID_CONFIG_REQUEST_MESSAGE_INTERVAL;
                    im.kvps[im.numParameters].value = (CONFIG_MESSAGE_INTERVAL_MS/1000);
                    im.numParameters++;
                    
                    // add zigbee module information:
                    if (sysVersion() != MODULE_SUCCESS)
                    {
                        printf("ERROR retriving module information\r\n");
                    } else {                
                        displaySysVersion();
                        
                        // Product ID
                        im.kvps[im.numParameters].oid = OID_MODULE_PRODUCT_ID;
                        im.kvps[im.numParameters].value = zmBuf[SYS_VERSION_RESULT_PRODUCTID_FIELD];
                        im.numParameters++;
                        
                        // FW - Major
                        im.kvps[im.numParameters].oid = OID_MODULE_FIRMWARE_MAJOR;
                        im.kvps[im.numParameters].value = zmBuf[SYS_VERSION_RESULT_FW_MAJOR_FIELD];
                        im.numParameters++;    
                        
                        // FW - Minor
                        im.kvps[im.numParameters].oid = OID_MODULE_FIRMWARE_MINOR;
                        im.kvps[im.numParameters].value = zmBuf[SYS_VERSION_RESULT_FW_MINOR_FIELD];
                        im.numParameters++;  
                        
                        // FW - Build
                        im.kvps[im.numParameters].oid = OID_MODULE_FIRMWARE_BUILD;
                        im.kvps[im.numParameters].value = zmBuf[SYS_VERSION_RESULT_FW_BUILD_FIELD];
                        im.numParameters++;
                    }
                    
                    printInfoMessage(&im);
#define RESTART_DELAY_IF_MESSAGE_FAIL_MS 5000
                    uint8_t* messageBuffer = (zmBuf + 100);         // To conserve RAM we use the tail of zmBuf for our serialization buffer
                    serializeInfoMessage(&im, messageBuffer);       // Convert our message struct to an array of bytes
                    moduleResult_t result;
                    
                    if (buttonIsPressed(ANY_BUTTON))                // If the button is pressed then use MAC ACKing, otherwise use APS ACKing
                    {
                        printf("Sending with MAC ACK\r\n");
                        afSetAckMode(AF_MAC_ACK);
                    } else {
                        
                        printf("Sending with APS ACK \r\n");
                        afSetAckMode(AF_APS_ACK);                    
                    }
                    setLed(SEND_MESSAGE_LED);                       // Indicate that we are sending a message                
                    result = afSendData(DEFAULT_ENDPOINT, DEFAULT_ENDPOINT, 0, INFO_MESSAGE_CLUSTER, messageBuffer, getSizeOfInfoMessage(&im)); // Send the message                
                    clearLed(SEND_MESSAGE_LED);
                    if (result != MODULE_SUCCESS)
                    {
                        printf("afSendData error 0x%02X; restarting...\r\n", result);                    
                        state = STATE_MESSAGE_SEND_FAILURE;
                    } else {   
                        state = STATE_MESSAGE_SEND_SUCCESS;
                    }
                } else {
                    printf("Network offline\r\n");
                    state = STATE_IDLE;
                }                
            }
            break;               
            
        case STATE_SEND_CONFIG_REQUEST_MESSAGE:
            {
                if (NETWORK_IS_ONLINE())
                {                    
                    printf("~crm~");
                    struct configRequestMessage crm;
                    crm.header = hdr;    // deep copy our header into our message
                    crm.deviceType = DEVICETYPE_TESLA_CONTROLS_CFG_ROUTER_DEMO;
                    hdr.sequence = sequenceNumber++;
                    crm.deviceSubType = DEVICESUBTYPE_CFG_PARAMETERS_SET_1;                
                    crm.firmwareVersion = getFirmwareVersion();
                    crm.reserved1 = 1111;
                    crm.reserved2 = 2222;
                    crm.reserved3 = 3333;
                    
                    displayConfigRequestMessage(&crm);
                    uint8_t* messageBuffer = (zmBuf + 100);         // To conserve RAM we use the tail of zmBuf for our serialization buffer
                    serializeConfigRequestMessage(&crm, messageBuffer);       // Convert our message struct to an array of bytes
                    moduleResult_t result;  
                    setLed(SEND_MESSAGE_LED);                       // Indicate that we are sending a message                
                    result = afSendData(DEFAULT_ENDPOINT, DEFAULT_ENDPOINT, 0, CONFIG_REQUEST_MESSAGE_CLUSTER, messageBuffer, getSizeOfConfigRequestMessage(&crm)); // Send the message
                    clearLed(SEND_MESSAGE_LED);                
                    if (result != MODULE_SUCCESS)
                    {
                        printf("afSendData error 0x%02X\r\n", result);                    
                        state = STATE_MESSAGE_SEND_FAILURE;
                    } else {   
                        state = STATE_MESSAGE_SEND_SUCCESS;                     
                    }
                } else {
                    printf("Network offline\r\n");
                    state = STATE_IDLE;
                }
            }               
            break;
            
        case STATE_MESSAGE_SEND_SUCCESS:
            {
                printf("Success\r\n");
                failCount = 0;
                state = STATE_IDLE;   
            }
            break;
            
#define MAXIMUM_FAILED_MESSAGES_BEFORE_RESTART      2
        case STATE_MESSAGE_SEND_FAILURE:
            {
                printf("~msf~");                    
                failCount++;   
                if (failCount > MAXIMUM_FAILED_MESSAGES_BEFORE_RESTART)
                {
                    printf("Over %u messages failed; triggering restart\r\n", MAXIMUM_FAILED_MESSAGES_BEFORE_RESTART);
                    failCount = 0;
                    SET_NETWORK_STATUS_OFFLINE();
                    delayMs(RESTART_DELAY_IF_MESSAGE_FAIL_MS);  // Allow enough time for coordinator to fully restart, if that caused our problem
                    state = STATE_MODULE_STARTUP;
                } else {
                    state = STATE_IDLE;
                }              
            }
            break;  
            
        default:     //should never happen
            {
                printf("UNKNOWN STATE\r\n");
                state = STATE_MODULE_STARTUP;
            }
            break;
        }
    } 
}


/** Systick interrupt handler */
void sysTick()
{
    infoMessageTimer += SYSTICK_INTERVAL_MS;
    if (infoMessageTimer > INFO_MESSAGE_INTERVAL_MS)
    {        
        SET_SEND_INFO_MESSAGE_FLAG();
        infoMessageTimer = 0;
    }
    
    configMessageTimer += SYSTICK_INTERVAL_MS;
    if (configMessageTimer > CONFIG_MESSAGE_INTERVAL_MS)
    {
        SET_SEND_CONFIG_MESSAGE_FLAG();
        configMessageTimer = 0;
    }
}


/* @} */
