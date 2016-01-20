/**
* @ingroup apps
* @{
*
* @file example_config_application_coordinator_afzdo.c
*
* @brief Resets Module, configures this device to be a Zigbee Coordinator, and displays any messages
* that are received. If the message contains a known object-identifier (OID) then parses the
* contents of the OID. Also replies to incoming "Config Request" messages with a "Config Response"
* message containing the current color settings. Pushing the button will change which color shall
* be displayed by all nodes
*
* Uses the AF/ZDO interface.
*
* $Rev: 2021 $
* $Author: dsmith $
* $Date: 2014-01-20 17:50:56 -0800 (Mon, 20 Jan 2014) $
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

#include "../HAL/hal.h"
#include "../ZM/module.h"
#include "../ZM/application_configuration.h"
#include "../ZM/af.h"
#include "../ZM/zdo.h"
#include "../ZM/zm_phy_spi.h"
#include "../ZM/module_errors.h"
#include "../ZM/module_utilities.h"
#include "../Common/utilities.h"
#include "Messages/infoMessage.h"
#include "Messages/configRequestMessage.h"
#include "Messages/configResponseMessage.h"  
#include "Messages/kvp.h"
#include "Messages/oids.h"
#include "module_example_utils.h"
#include <stdint.h>

static void parseMessages();

/** Function pointer (in hal file) for the function that gets called when a button is pressed*/
extern void (*buttonIsr)(int8_t);

/** Our button interrupt handler */
static void handleButtonPress(int8_t button);

/** function pointer (in hal file) for the function that gets called when the systick generates an int*/
extern void (*sysTickIsr)(void);

/** Handles systick interrupt */
void sysTick();

/** STATES for state machine */
enum STATE
{
    STATE_IDLE,
    STATE_MODULE_STARTUP,
    STATE_DISPLAY_NETWORK_INFORMATION,
    STATE_BUTTON_PRESSED
};

/** This is the current state of the application. 
* Gets changed by other states, or based on messages that arrive. */
enum STATE state = STATE_MODULE_STARTUP;

/** The main application state machine */
static void stateMachine();

/** Color values that will be transmitted to nodes and also displayed on the LED */
uint8_t red = 0xFF;
uint8_t blue = 0xFF;
uint8_t green = 0xFF;

/** Various utility functions */
static char* getRgbLedColorName(const uint8_t mode);

/** Various flags between states */
volatile uint16_t stateFlags = 0;
#define STATE_FLAG_MESSAGE_WAITING          (0x01)
#define STATE_FLAG_BUTTON_PRESSED           (0x02)

/** Whether the zigbee network has been started, etc.*/
uint8_t zigbeeNetworkStatus = 0;
#define NWK_OFFLINE                         (0)
#define NWK_ONLINE                          (1)
#define NETWORK_IS_ONLINE()                 (zigbeeNetworkStatus & NWK_ONLINE)
#define NETWORK_IS_OFFLINE()                ((~zigbeeNetworkStatus) & NWK_ONLINE)
#define SET_NETWORK_STATUS_ONLINE()         (zigbeeNetworkStatus |= NWK_ONLINE)
#define SET_NETWORK_STATUS_OFFLINE()        (zigbeeNetworkStatus &= ~NWK_ONLINE)

/* What color to display on the RGB LED */
uint8_t rgbLedColor = 0;
#define RGB_LED_COLOR_WHITE                 (0)
#define RGB_LED_COLOR_RED                   (1)
#define RGB_LED_COLOR_VIOLET                (2)
#define RGB_LED_COLOR_BLUE                  (3)
#define RGB_LED_COLOR_CYAN                  (4)
#define RGB_LED_COLOR_GREEN                 (5)
#define RGB_LED_COLOR_YELLOW                (6)
#define RGB_LED_COLOR_MAX                   RGB_LED_COLOR_YELLOW

/** Simple timestamp we send to nodes if they request it */
volatile uint32_t timestamp = 0;//0xDEADBEEF;

/** Number of mS since last sysTick */
volatile uint16_t sysTickMilliseconds = 0;

extern uint8_t zmBuf[ZIGBEE_MODULE_BUFFER_SIZE];

//uncomment below to see more information about the messages received.
//#define VERBOSE_MESSAGE_DISPLAY

int main( void )
{
    halInit();
    moduleInit();
    buttonIsr = &handleButtonPress;
    sysTickIsr = &sysTick;    
    printf("\r\n****************************************************\r\n");
    printf("Config Application Example - COORDINATOR\r\n");    
    clearLeds();    
    halRgbLedPwmInit();
    initSysTick();
    HAL_ENABLE_INTERRUPTS(); //NEW
    stateMachine();    //run the state machine
}


/** 
The main state machine for the application.
Never exits.
*/
void stateMachine()
{
    while (1)
    {
        if (zigbeeNetworkStatus == NWK_ONLINE)
        {
            if(moduleHasMessageWaiting())      //wait until SRDY goes low indicating a message has been received. 
                stateFlags |= STATE_FLAG_MESSAGE_WAITING;
        }
        
        switch (state)
        {
        case STATE_IDLE:
            {
                if (stateFlags & STATE_FLAG_MESSAGE_WAITING)    // If there is a message waiting...
                {
                    parseMessages();                            // ... then display it
                    stateFlags &= ~STATE_FLAG_MESSAGE_WAITING;
                }
                
                if (stateFlags & STATE_FLAG_BUTTON_PRESSED)     // If ISR set this flag...
                {
                    state = STATE_BUTTON_PRESSED;
                    stateFlags &= ~STATE_FLAG_BUTTON_PRESSED;
                }                
                /* Other flags (for different messages or events) can be added here */
            }
            break;            
            
        case STATE_BUTTON_PRESSED:
            {
                rgbLedColor++;
                if (rgbLedColor > RGB_LED_COLOR_MAX)
                {
                    rgbLedColor = 0; 
                }
                printf("Setting Color to %s (%u)\r\n", getRgbLedColorName(rgbLedColor), rgbLedColor);
                switch (rgbLedColor)
                {
                case RGB_LED_COLOR_WHITE: red=RGB_LED_MAX; blue=RGB_LED_MAX; green=RGB_LED_MAX; break;
                case RGB_LED_COLOR_RED: red=RGB_LED_MAX; blue=0; green=0; break;
                case RGB_LED_COLOR_VIOLET: red=RGB_LED_MAX; blue=RGB_LED_MAX; green=0; break;
                case RGB_LED_COLOR_BLUE: red=0; blue=RGB_LED_MAX; green=0; break;        
                case RGB_LED_COLOR_CYAN: red=0; blue=RGB_LED_MAX; green=RGB_LED_MAX; break;
                case RGB_LED_COLOR_GREEN: red=0; blue=0; green=RGB_LED_MAX; break;
                case RGB_LED_COLOR_YELLOW: red=RGB_LED_MAX; blue=0; green=RGB_LED_MAX; break;
                default: red=RGB_LED_MAX; blue=RGB_LED_MAX; green=RGB_LED_MAX; break;
                }
                halRgbSetLeds(red, blue, green);
                state = STATE_IDLE;                
            }
            break;
            
        case STATE_MODULE_STARTUP:              // Start the Zigbee Module on the network
            {
#define MODULE_START_DELAY_IF_FAIL_MS 5000
                moduleResult_t result;
                struct moduleConfiguration defaultConfiguration = DEFAULT_MODULE_CONFIGURATION_COORDINATOR;
                
                /* Change this if you wish to use a custom PAN */
                defaultConfiguration.panId = ANY_PAN;
                
                /*Example of how to use a custom channel:
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
                halRgbSetLeds(RGB_LED_MAX, RGB_LED_MAX, RGB_LED_MAX);
                printf("Module Start Complete\r\n"); 
                SET_NETWORK_STATUS_ONLINE();
                
                state = STATE_DISPLAY_NETWORK_INFORMATION;
            }
            break;            
            
        case STATE_DISPLAY_NETWORK_INFORMATION:
            {
                printf("~ni~");
                /* On network, display info about this network */
                displayNetworkConfigurationParameters();
                displayDeviceInformation();
                
                printf("Press button to change color configuration\r\n");                
                printf("Displaying Messages Received\r\n");
                
                /* Now the network is running - wait for any received messages from the ZM */
#ifdef VERBOSE_MESSAGE_DISPLAY    
                printAfIncomingMsgHeaderNames();
#endif                
                state = STATE_IDLE;
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


/** Converts the message payload in zmBuf to a struct infoMessage and then displays it.
In a more advanced application, you may want to store this or perform additional processing on the 
parameters in the message. */
static void processInfoMessage()
{
    struct infoMessage im;
    deserializeInfoMessage(zmBuf+20, &im);  // Convert the bytes into a Message struct
    printInfoMessage(&im);
}


/** Converts the message payload into a ConfigRequestMessage and then gets the current configuration
and replies to the node with a ConfigResposeMessage containing the configuration information */
static void processConfigRequestMessage()
{    
    struct configRequestMessage req;
    deserializeConfigRequestMessage(zmBuf+AF_INCOMING_MESSAGE_PAYLOAD_START_FIELD, &req);  // Convert the bytes into a Message struct            
    displayConfigRequestMessage(&req);
    
    // Now create  the Config Response message....
    uint16_t shortAddressToReplyTo = AF_INCOMING_MESSAGE_SHORT_ADDRESS();
    
    struct configResponseMessage resp;
    resp.header = req.header;   // First start with the header from the config request msg
    resp.responseSequence = req.header.sequence; // This indicates how the response matches the request            
    resp.header.sequence++;     // Next, increment the sequence number
    resp.header.flags=0;        // Nothing further to do; we don't need a response
    
    resp.reserved1=0xBEAD;        // Helps troubleshooting to give this an obvious value
    resp.timestamp = timestamp; // In a server-side implementation this would be the unix timestamp
    resp.reserved2=0xEE;        // Another troubleshooting help
    resp.numParameters=0;       // Red, Blue, Green
    // Add the red LED value
    resp.kvps[resp.numParameters].oid = OID_RED_LED;
    resp.kvps[resp.numParameters].value = red;
    resp.numParameters++;
    // Add the blue LED value
    resp.kvps[resp.numParameters].oid = OID_BLUE_LED;
    resp.kvps[resp.numParameters].value = blue;
    resp.numParameters++;
    // Add the green LED value
    resp.kvps[resp.numParameters].oid = OID_GREEN_LED;
    resp.kvps[resp.numParameters].value = green;
    resp.numParameters++;
    
    // Now we can send the message
    displayConfigResponseMessage(&resp);
    uint8_t buffer[32];
    //uint8_t* buffer = (zmBuf + 100);         // To conserve RAM you could use the tail of zmBuf for serialization buffer
    serializeConfigResponseMessage(&resp, buffer);       // Convert our message struct to an array of bytes
    
    uint8_t numBytesSerialized = getSizeOfConfigResponseMessage(&resp);
    //printf("numBytesSerialized = %u; send to %04X\r\n", numBytesSerialized, shortAddressToReplyTo);
    
    moduleResult_t result = 0;
    result = afSendData(DEFAULT_ENDPOINT, DEFAULT_ENDPOINT, shortAddressToReplyTo, CONFIG_RESPONSE_MESSAGE_CLUSTER, buffer, getSizeOfConfigResponseMessage(&resp)); // Send the message
    if (result != MODULE_SUCCESS)
    {
        printf("afSendData error 0x%02X; ignoring...\r\n", result);
    } else {   
        printf("Success\r\n");
    }
}


/** Parse any received messages. If it's one of our OIDs then display it. */
void parseMessages()
{
    getMessage();
    if ((zmBuf[SRSP_LENGTH_FIELD] > 0) && (IS_AF_INCOMING_MESSAGE()))
    {
        setLed(0);                                  //LED will blink to indicate a message was received
#ifdef VERBOSE_MESSAGE_DISPLAY
        printAfIncomingMsgHeader(zmBuf);
        printf("\r\n");
#endif
        if ((AF_INCOMING_MESSAGE_CLUSTER()) == INFO_MESSAGE_CLUSTER)
        {
            processInfoMessage();            
        } else if ((AF_INCOMING_MESSAGE_CLUSTER()) == CONFIG_REQUEST_MESSAGE_CLUSTER) {
            processConfigRequestMessage();            
        } else {    //unknown cluster
            printf("Rx: ");
            printHexBytes(zmBuf+SRSP_HEADER_SIZE+17, zmBuf[SRSP_HEADER_SIZE+16]);   //print out message payload
        }
        clearLed(0);    
    } else if (IS_ZDO_END_DEVICE_ANNCE_IND()) {
        displayZdoEndDeviceAnnounce(zmBuf);
    } else { //unknown message, just print out the whole thing
        printf("MSG: ");
        printHexBytes(zmBuf, (zmBuf[SRSP_LENGTH_FIELD] + SRSP_HEADER_SIZE));
    }
    zmBuf[SRSP_LENGTH_FIELD] = 0;
}


/* 
Displays the pretty name of the LED display mode.
@param mode which LED display mode
@return name of mode, or "UNKNOWN" if not known
*/
static char* getRgbLedColorName(const uint8_t mode)
{
    switch(mode)
    {
    case RGB_LED_COLOR_WHITE:
        return "RGB_LED_COLOR_WHITE";         
    case RGB_LED_COLOR_RED:
        return "RGB_LED_COLOR_RED";
    case RGB_LED_COLOR_VIOLET:
        return "RGB_LED_COLOR_VIOLET";
    case RGB_LED_COLOR_BLUE:
        return "RGB_LED_COLOR_BLUE";
    case RGB_LED_COLOR_CYAN:
        return "RGB_LED_COLOR_CYAN";
    case RGB_LED_COLOR_GREEN:
        return "RGB_LED_COLOR_GREEN";
    case RGB_LED_COLOR_YELLOW:
        return "RGB_LED_COLOR_YELLOW";               
    default:
        return "UNKNOWN";
    }
}


/** 
Button interrupt service routine. Called when interrupt generated on the button.
@pre Button connects input to GND.
@pre pins are configured as interrupts appropriately and have pull-UP resistors.
*/
static void handleButtonPress(int8_t button)
{
    stateFlags |= STATE_FLAG_BUTTON_PRESSED;
}


/** Systick interrupt handler */
void sysTick()
{
    sysTickMilliseconds += SYSTICK_INTERVAL_MS;
    if (sysTickMilliseconds >= 1000)
    {        
        sysTickMilliseconds = 0;
        timestamp++;
    }
}

/* @} */

