/**
* @ingroup apps
* @{
*
* @file example_network_explorer.c
*
* @brief Allows the user to configure this device as either a Coordinator, Router, or End device and
* explore various features of the Zigbee Module, such as sending messages via short address or
* long address, finding devices, etc. Uses the AF/ZDO interface. Uses a command line interface (CLI).
*
* @note Different functions can be turned on/off by defining the compile time options. While the
* functions will work on various processors, due to flash space restrictions you may not be able to
* use all functions on a smaller processor (like the MSP430 value line). The best tool to evaluate
* this Network Explorer is the Stellaris LaunchPad with Code Composer Studio. The Stellaris LaunchPad
* is able to run all features of the Network Explorer and is hard to beat for < $5US!
*
* $Rev: 2158 $
* $Author: dsmith $
* $Date: 2014-05-14 18:37:56 -0700 (Wed, 14 May 2014) $
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

#include "../HAL/hal.h"
#include "../ZM/module.h"
#include "../ZM/application_configuration.h"
#include "../ZM/af.h"
#include "../ZM/zdo.h"
#include "../ZM/zm_phy_spi.h"
#include "../ZM/module_errors.h"
#include "../ZM/module_utilities.h"
#include "../Common/utilities.h"
#include "module_example_utils.h"
#include "set.h"
#include <stdint.h>
#include <stdlib.h>     // for strtol
#include <errno.h>      // for errno (in strtol)
#include <string.h>     // for memcpy

/** This removes the blocking wait in afSendData() but this means that we have to process the AF_DATA_CONFIRM */
//#define AF_DATA_CONFIRM_HANDLED_BY_APPLICATION

/** This bypasses the blocking wait in zdoRequestIeeeAddress() */
//#define ZDO_IEEE_ADDR_RSP_HANDLED_BY_APPLICATION

/** This bypasses the blocking wait in zdoRequestNwkAddress() */
//#define ZDO_NWK_ADDR_RSP_HANDLED_BY_APPLICATION

/** The main application state machine */
void stateMachine();

/** Process a serial ISR */
void handleDebugConsoleInterrupt(int8_t rxByte);

// VARIABLES

#define NO_CHARACTER_RECEIVED 0xFF
/** the command that was entered by the user */
volatile static char command = NO_CHARACTER_RECEIVED;

/** STATES for state machine. NOTE: not all may be used, depending on #defined options*/
enum STATE
{
    STATE_IDLE,
    STATE_INIT,
    STATE_MODULE_STARTUP,
    STATE_GET_DEVICE_TYPE,
    STATE_DISPLAY_NETWORK_INFORMATION,
    STATE_SEND_MESSAGE_VIA_LONG_ADDRESS,
    STATE_SEND_MESSAGE_VIA_SHORT_ADDRESS,
    STATE_VALID_SHORT_ADDRESS_ENTERED,
    STATE_VALID_LONG_ADDRESS_ENTERED,
    STATE_FIND_VIA_SHORT_ADDRESS,
    STATE_SET_PERMIT_JOIN_ON,    
    STATE_SET_PERMIT_JOIN_OFF,      
    STATE_FIND_VIA_LONG_ADDRESS,
    STATE_NETWORK_DISCOVERY_REQUEST,
    STATE_MANAGEMENT_NETWORK_DISCOVERY_REQUEST,
    STATE_MANAGEMENT_LEAVE_REQUEST,
    STATE_GET_NETWORK_TOPOLOGY,
    STATE_JOIN_REQUEST
};

/** Command Line Interface processor */
enum STATE processCommand(char cmd);

/** This is the current state of the application and is modified by other states, or based on the
contents of an incoming message. */
enum STATE state = STATE_INIT;

/** This stores what to do once a valid shortAddress or longAddress has been parsed by the command
line processor.*/
enum STATE pendingState = STATE_IDLE;

/** Various flags between states */
uint16_t stateFlags = 0;

extern uint8_t zmBuf[ZIGBEE_MODULE_BUFFER_SIZE];

/** Which device type to use to start the module. This can be changed via CLI. */
uint8_t zigbeeDeviceType = ROUTER;

#define CLI_MODE_NORMAL                 0
#define CLI_MODE_ENTER_SHORT_ADDRESS    1
#define CLI_MODE_ENTER_LONG_ADDRESS     2
static uint8_t commandLineInterfaceMode = CLI_MODE_NORMAL;

#define CLI_INPUT_BUFFER_SIZE           24
static char cliInputBuffer[CLI_INPUT_BUFFER_SIZE];
static uint8_t cliInputBufferIndex = 0;

#define NWK_OFFLINE                     0
#define NWK_ONLINE                      1
/** Whether the zigbee network has been started, etc.*/
uint8_t zigbeeNetworkStatus = NWK_OFFLINE;

uint16_t shortAddressEntered;  //the short address entered by the user; used in several commands
uint8_t longAddressEntered[8]; //the long address entered by the user; used in several commands

/** The test message to send */
unsigned char testMessage[] = {0xE0,0xE1,0xE2,0xE3,0xE4};

/** Which cluster our test message uses */
#define TEST_CLUSTER 0x77

/* FUNCTION POINTERS for HAL callback methods */
extern void (*debugConsoleIsr)(int8_t); 

//uncomment below to see more information about the messages received.
//#define VERBOSE_MESSAGE_DISPLAY

int main( void )
{
    halInit();
    moduleInit();
    printf("\r\n****************************************************\r\n");    
    printf("Zigbee Network Explorer\r\n");
    debugConsoleIsr = &handleDebugConsoleInterrupt;
    HAL_ENABLE_INTERRUPTS();
    clearLeds();
    
    /* Now the network is running - wait for any received messages from the ZM */
#ifdef VERBOSE_MESSAGE_DISPLAY    
    printAfIncomingMsgHeaderNames();
#endif
    
    stateMachine();
}

/** Displays a list of all commands */
static void displayCommandLineInterfaceHelp()
{
    printf("\r\nCommands:\r\n");
    printf("    ?   Displays this\r\n");
#ifdef INCLUDE_PERMIT_JOIN
    printf("    A   Turn ON Joining for short address\r\n");
    printf("    B   Turn OFF Joining for short address\r\n");    
#endif
    
#ifdef DISCOVER_NETWORKS    
    printf("    D   Discover Networks (only when not on NWK)\r\n");
    printf("    M   Discover Networks using zdoManagementNetworkDiscoveryRequest\r\n");
    printf("    K   Test Join Request (must #define first!)\r\n");
#endif
#ifdef LEAVE_REQUEST
    printf("    E   Request Device to Leave\r\n");
#endif
    printf("    N   Display Network Information\r\n");
    printf("    R   Restart Zigbee Module\r\n");
    printf("    S   Send a message by Short Address\r\n");
    printf("    L   Send a message by Long Address\r\n");
    
    printf("    H   Find via short address & display children\r\n");
    printf("    J   Find via long address (not for End Devices)\r\n");    
    
#ifdef INCLUDE_NETWORK_TOPOLOGY
    printf("    I   Get Topology of short address\r\n");
#endif    
    
    printf("    V   Display Module Version Information\r\n");
    printf("\r\n");
}

#ifdef INCLUDE_NETWORK_TOPOLOGY
#include "set.h"

/** Contains all the short addresses that we've searched for already */
set searchedSet;

/** 
* Utility function only used when displaying the network topology.
* Recursively iterates through the network and displays the children of a node and then the children
* of those children, and then the children of those children, etc. using recursion.
* @param shortAddress the root at which to start getting the topology. Use 0000 for the Coordinator.
* @note does not retrieve children for the same node twice. Uses a Set to keep track of which nodes
* have been searched. This is done because occasionally two nodes will list the same child.
* @see http://en.wikipedia.org/wiki/Recursion_%28computer_science%29
* @return the number of children
* @todo handle case where starting index != 0. We will have to keep track of which nodes had
* additional children, and then call zdoRequestIeeeAddress() for the next group.
*/
static uint8_t displayChildren(uint16_t shortAddress)
{
    printf("Device %04X has ", shortAddress);
    
    moduleResult_t result = zdoRequestIeeeAddress(shortAddress, INCLUDE_ASSOCIATED_DEVICES, 0);
#define MAX_CHILDREN 5  // Max of 20 from Z-Stack. Of these 20, 7 can be routers, the rest end devices. 
                        // We reduce to fit in RAM on the LaunchPad since this is called recursively
    uint8_t childCount = 0;
    uint16_t children[MAX_CHILDREN];
    
    if (result == MODULE_SUCCESS)
    {
        if (!(addToSet(&searchedSet, shortAddress)))
        {
            printf("[addToSet failed for 0x%04X\r\n]", shortAddress);
        }
        
        childCount = zmBuf[SRSP_PAYLOAD_START + ZDO_IEEE_ADDR_RSP_NUMBER_OF_ASSOCIATED_DEVICES_FIELD];
        
        if (childCount > 0)
        {
            printf("%u children, starting with #%u:", childCount, zmBuf[SRSP_PAYLOAD_START + ZDO_IEEE_ADDR_RSP_START_INDEX_FIELD]);
            int i=0;
            int j;
            for (j = 0; j < (childCount*2) - 1; j += 2) // Iterate through all the children. Each is a 2B integer
            {
                children[i] = CONVERT_TO_INT(zmBuf[(j + SRSP_PAYLOAD_START + ZDO_IEEE_ADDR_RSP_ASSOCIATED_DEVICE_FIELD_START)], zmBuf[( j + SRSP_PAYLOAD_START + ZDO_IEEE_ADDR_RSP_ASSOCIATED_DEVICE_FIELD_START + 1)]);
                printf("<%04X> ", children[i]);
                i++;
            }
            printf("\r\n");
            
            for (i=0; i<childCount; i++)                     // Iterate through all children
            {
                if (!setContains(&searchedSet, children[i])) // Don't process the same short address twice!
                {
                    displayChildren(children[i]);            // Note use of recursion here
                }
            }
        } else {
            printf("no children\r\n");
        }
    } else {
        printf("Could not locate that device (Error Code 0x%02X)\r\n", result);
    }
    return childCount;
}
#endif

static void handleModuleError(uint8_t err)
{
    printf("Error Code 0x%02X\r\n", err);
    /** More advanced error handling could go here */
}


static void stateMachine()
{
    while (1)
    {
        if (zigbeeNetworkStatus == NWK_ONLINE)
        {
            if(moduleHasMessageWaiting())      //wait until SRDY goes low indicating a message has been received.
            {
                getMessage();                 
                displayMessage();          
            }   
        }
        
        switch (state)
        {
        case STATE_IDLE:
            {
                /* process command line commands only if not doing anything else: */
                if (command != NO_CHARACTER_RECEIVED)
                {
                    /* parse the command entered, and go to the required state */
                    state = processCommand(command);                    
                    command = NO_CHARACTER_RECEIVED;
                }
                /* note: other flags (for different messages or events) can be added here */
                break;
            }
        case STATE_INIT:
            {
                printf("Starting State Machine\r\n");
                state = STATE_GET_DEVICE_TYPE;
                break;
            }
            /* A button press during startup will cause the application to prompt for device type */
        case STATE_GET_DEVICE_TYPE: 
            {
            set_type:                
                while (command == NO_CHARACTER_RECEIVED)
                {
                    printf("Setting Device Type: Press C for Coordinator, R for Router, or E for End Device.\r\n");
#define DEVICE_TYPE_DELAY_MS    2000
#define DEVICE_TYPE_DELAY_PER_CYCLE_MS  100
                    uint16_t delayCycles = DEVICE_TYPE_DELAY_MS / DEVICE_TYPE_DELAY_PER_CYCLE_MS;
                    while ((command == NO_CHARACTER_RECEIVED) && (delayCycles--) > 0)
                        delayMs(DEVICE_TYPE_DELAY_PER_CYCLE_MS);
                }
                
                switch (command)
                {
                case 'C':
                case 'c':
                    printf("Coordinator it is...\r\n");
                    zigbeeDeviceType = COORDINATOR;
                    break;
                case 'R':
                case 'r':
                    printf("Router it is...\r\n");
                    zigbeeDeviceType = ROUTER;
                    break;
                    
                case 'E':
                case 'e':
                    printf("End Device it is...\r\n");
                    zigbeeDeviceType = END_DEVICE;
                    break;
                default:
                    command = NO_CHARACTER_RECEIVED;
                    goto set_type;
                    
                }
                command = NO_CHARACTER_RECEIVED;
                state = STATE_MODULE_STARTUP;
                break;
            }
        case STATE_MODULE_STARTUP:
            {
#define MODULE_START_DELAY_IF_FAIL_MS 5000
                moduleResult_t result;
                /* Start with the default module configuration */
                struct moduleConfiguration defaultConfiguration = DEFAULT_MODULE_CONFIGURATION_COORDINATOR;
                /* Make any changes needed here (channel list, PAN ID, etc.)
                We Configure the Zigbee Device Type (Router, Coordinator, End Device) based on what user selected */
                defaultConfiguration.deviceType = zigbeeDeviceType;
                
                /* Change this below to be your operating region - MODULE_REGION_NORTH_AMERICA or MODULE_REGION_EUROPE */
#define OPERATING_REGION    (MODULE_REGION_NORTH_AMERICA) // or MODULE_REGION_EUROPE
                
                while ((result = expressStartModule(&defaultConfiguration, GENERIC_APPLICATION_CONFIGURATION, OPERATING_REGION)) != MODULE_SUCCESS)
                {
                    SET_NETWORK_FAILURE_LED_ON();          // Turn on the LED to show failure
                    handleModuleError(result);
                    printf("Retrying...\r\n");
                    delayMs(MODULE_START_DELAY_IF_FAIL_MS/2);                    
                    SET_NETWORK_FAILURE_LED_OFF();
                    delayMs(MODULE_START_DELAY_IF_FAIL_MS/2);
                }                
                printf("Success\r\n");
                state = STATE_DISPLAY_NETWORK_INFORMATION;
                zigbeeNetworkStatus = NWK_ONLINE;
                break;
            }
        case STATE_DISPLAY_NETWORK_INFORMATION:                 
            {
                printf("Module Information:\r\n");
                /* On network, display info about this network */
                displayNetworkConfigurationParameters();
                displayDeviceInformation();
                displayCommandLineInterfaceHelp();
                state = STATE_IDLE;   //startup is done!
                break;
            }
        case STATE_VALID_SHORT_ADDRESS_ENTERED:  //command line processor has a valid shortAddressEntered
            {
                state = pendingState;
                break;
            }
        case STATE_VALID_LONG_ADDRESS_ENTERED:
            {
                /* Flip byte order because we need to send it LSB first */
                int8_t temp[8];
                int i;
                for (i=0; i<8; i++)
                    temp[7-i] = longAddressEntered[i];
                
                memcpy(longAddressEntered, temp, 8);  //Store LSB first since that is how it will be sent:
                state = pendingState;
                break;
            }
        case STATE_SEND_MESSAGE_VIA_SHORT_ADDRESS:
            {
                printf("Send to 0x%04X\r\n", shortAddressEntered);
                moduleResult_t result = afSendData(DEFAULT_ENDPOINT,DEFAULT_ENDPOINT,shortAddressEntered, TEST_CLUSTER, testMessage, 5);
                if (result == MODULE_SUCCESS)
                {
                    printf("Success\r\n");
                } else {
                    handleModuleError(result);                    
#ifdef RESTART_AFTER_ZM_FAILURE
                    printf("\r\nRestarting\r\n");
                    state = STATE_MODULE_STARTUP;
                    continue;
#endif
                }
                state = STATE_IDLE;
                break;
            }
        case STATE_SEND_MESSAGE_VIA_LONG_ADDRESS:
            {
                printf("Send to (LSB first)");
                printHexBytes(longAddressEntered, 8);
                moduleResult_t result = afSendDataExtended(DEFAULT_ENDPOINT, DEFAULT_ENDPOINT, longAddressEntered,
                                                           DESTINATION_ADDRESS_MODE_LONG, TEST_CLUSTER, testMessage, 5);
                if (result == MODULE_SUCCESS)
                {
                    printf("Success\r\n");
                } else {
                    handleModuleError(result);
#ifdef RESTART_AFTER_ZM_FAILURE
                    printf("\r\nRestarting\r\n");
                    state = STATE_MODULE_STARTUP;
                    continue;
#endif
                }
                state = STATE_IDLE;
                break;
            }
            
#ifdef LEAVE_REQUEST            
        case STATE_MANAGEMENT_LEAVE_REQUEST:
            {
                printf("Sending Leave Request for MAC (LSB first)");
                printHexBytes(longAddressEntered, 8);
                moduleResult_t result = zdoManagementLeaveRequest(longAddressEntered, 0);
                
                if (result == MODULE_SUCCESS)
                {
                    printf("Success\r\n");
                } else {
                    handleModuleError(result);
                }
                state = STATE_IDLE;
                break;
            }
#endif
            
        case STATE_FIND_VIA_SHORT_ADDRESS:
            {
                printf("Looking for that device...\r\n");
                moduleResult_t result = zdoRequestIeeeAddress(shortAddressEntered, INCLUDE_ASSOCIATED_DEVICES, 0);
                if (result == MODULE_SUCCESS)
                {
#ifndef ZDO_NWK_ADDR_RSP_HANDLED_BY_APPLICATION
                    displayZdoAddressResponse(zmBuf + SRSP_PAYLOAD_START);
#endif
                } else {
                    handleModuleError(result);
                }
                state = STATE_IDLE;
                break;
            }
            
        case STATE_FIND_VIA_LONG_ADDRESS:
            {
                printf("Looking for that device...\r\n");
                moduleResult_t result = zdoNetworkAddressRequest(longAddressEntered, INCLUDE_ASSOCIATED_DEVICES, 0);
                if (result == MODULE_SUCCESS)
                {
#ifndef ZDO_NWK_ADDR_RSP_HANDLED_BY_APPLICATION
                    displayZdoAddressResponse(zmBuf + SRSP_PAYLOAD_START);
#endif
                } else {
                    handleModuleError(result);
                }
                state = STATE_IDLE;
                break;
            }
            
#ifdef INCLUDE_PERMIT_JOIN        
        case STATE_SET_PERMIT_JOIN_ON:
            {
                printf("Turning joining ON...\r\n");
                moduleResult_t result = zdoManagementPermitJoinRequest(shortAddressEntered, PERMIT_JOIN_ON_INDEFINITELY, 0);
                if (result == MODULE_SUCCESS)
                {
                    printf("Success\r\n");
                } else {
                    handleModuleError(result);
                }
                state = STATE_IDLE;
                break; 
            }
            
        case STATE_SET_PERMIT_JOIN_OFF:
            {
                printf("Turning joining OFF...\r\n");
                moduleResult_t result = zdoManagementPermitJoinRequest(shortAddressEntered, PERMIT_JOIN_OFF, 0);
                if (result == MODULE_SUCCESS)
                {
                    printf("Success\r\n");
                } else {
                    handleModuleError(result);
                }
                state = STATE_IDLE;
                break; 
            }   
#endif
            
#ifdef DISCOVER_NETWORKS             
        case STATE_NETWORK_DISCOVERY_REQUEST:
            {
                printf("Scanning: ");
                moduleResult_t result = zdoNetworkDiscoveryRequest(ANY_CHANNEL_MASK, BEACON_ORDER_480_MSEC);
                if (result == MODULE_SUCCESS)
                {
                    printf("Success\r\n");
                } else {
                    handleModuleError(result);
                }
                state = STATE_IDLE;
                break; 
            }
        case STATE_MANAGEMENT_NETWORK_DISCOVERY_REQUEST:
            {
                printf("Scanning: ");
                moduleResult_t result = zdoManagementNetworkDiscoveryRequest(0x0000, ANY_CHANNEL_MASK, BEACON_ORDER_480_MSEC, 0);
                if (result == MODULE_SUCCESS)
                {
                    printf("Success\r\n");
                } else {
                    handleModuleError(result);
                }
                state = STATE_IDLE;
                break;
            }
        case STATE_JOIN_REQUEST:
            {
                printf("Join Request: ");
                /* Define the values below for the network you wish to join */
#define JOIN_REQUEST_CHANNEL	(11)
#define JOIN_REQUEST_PANID		(0xC30B)
#define JOIN_REQUEST_PARENT		(0)
#define JOIN_REQUEST_DEPTH		(0)

                moduleResult_t result = zdoJoinRequest(JOIN_REQUEST_CHANNEL, JOIN_REQUEST_PANID, JOIN_REQUEST_PARENT, JOIN_REQUEST_DEPTH);
                if (result == MODULE_SUCCESS)
                {
                    printf("Success\r\n");
                } else {
                    handleModuleError(result);
                }
                state = STATE_IDLE;
                break;
            }

#endif
            
#ifdef INCLUDE_NETWORK_TOPOLOGY
        case STATE_GET_NETWORK_TOPOLOGY:
            {
                printf("Displaying NWK Topology...........\r\n");                
                initSet(&searchedSet);
                int8_t numChildren = displayChildren(shortAddressEntered);
                state = STATE_IDLE;
                break;
            }
#endif
            
        default:     //should never happen
            {
                printf("UNKNOWN STATE (%u)\r\n", state);
                state = STATE_IDLE;
            }
            break;
        }
    }
}


/** Method to handle bytes received on the debug console. Just stores it to the variable <code>command</code>.
This gets called by the ISR in the hal file since we set the debugConsoleIsr function pointer (defined in hal file) to point to this function.
*/
void handleDebugConsoleInterrupt(int8_t rxByte)
{
    command = rxByte;
}

/** Process characters typed in by the user. Normally this will fire off a menu command, unless
we are awaiting the user to enter a two-byte (four ascii character) short address or eight byte
(16 ascii character) long address.
@note: processCommand modifies variables shortAddressEntered and longAddressEntered
@return the state to transition to next
*/
enum STATE processCommand(char cmd)
{
#define ESCAPE_KEY 0x1B
    if (cmd == ESCAPE_KEY)
    {
        printf("Resetting Command Line Interpreter\r\n");
        commandLineInterfaceMode = CLI_MODE_NORMAL;
        cliInputBufferIndex = 0;
        pendingState = STATE_IDLE;
        displayCommandLineInterfaceHelp();
        return STATE_IDLE;
    }
    
    if (cmd == '\r')
    {
        printf("\r\n");
        return STATE_IDLE;
    }
    
    if (commandLineInterfaceMode == CLI_MODE_NORMAL)
    {
        printf("Press Escape Key to exit\r\n");        
        switch (cmd)
        {
        case '?':
            displayCommandLineInterfaceHelp();
            return STATE_IDLE;
#ifdef INCLUDE_PERMIT_JOIN
        case 'a':
        case 'A':
            {
                printf("Enter two byte short address to turn ON joining, for example '2F6B'\r\n");
                commandLineInterfaceMode = CLI_MODE_ENTER_SHORT_ADDRESS;
                pendingState = STATE_SET_PERMIT_JOIN_ON;  //when a full short address has been entered
                return STATE_IDLE;
            } 
        case 'b':
        case 'B':
            {
                printf("Enter two byte short address to turn OFF joining, for example '2F6B'\r\n");
                commandLineInterfaceMode = CLI_MODE_ENTER_SHORT_ADDRESS;
                pendingState = STATE_SET_PERMIT_JOIN_OFF;  //when a full short address has been entered
                return STATE_IDLE;
            }         
#endif          
            
#ifdef DISCOVER_NETWORKS         
        case 'd':
        case 'D':
            {
                printf("Scanning\r\n");
                return STATE_NETWORK_DISCOVERY_REQUEST;
            }
        case 'm':
        case 'M':
            {
                printf("zdoManagementNetworkDiscoveryRequest\r\n");
                return STATE_MANAGEMENT_NETWORK_DISCOVERY_REQUEST;
            }
        case 'k':
        case 'K':
            {
                printf("Join Request\r\n");
                return STATE_JOIN_REQUEST;
            }
#endif
            
#ifdef LEAVE_REQUEST            
        case 'e':
        case 'E':
            printf("Enter long (MAC) address of device to leave '00124B0012345678'\r\n");
            commandLineInterfaceMode = CLI_MODE_ENTER_LONG_ADDRESS;
            pendingState = STATE_MANAGEMENT_LEAVE_REQUEST;  //when a full long address has been entered
            return STATE_IDLE;
#endif
            
        case 'h':
        case 'H':
            {
                printf("Enter two byte short address to find, for example '2F6B'\r\n");
                commandLineInterfaceMode = CLI_MODE_ENTER_SHORT_ADDRESS;
                pendingState = STATE_FIND_VIA_SHORT_ADDRESS;  //when a full short address has been entered
                return STATE_IDLE;
            }
            
#ifdef INCLUDE_NETWORK_TOPOLOGY
        case 'i':
        case 'I':
            {
                printf("Enter two byte short address to get topology, for example '2F6B'\r\n");
                commandLineInterfaceMode = CLI_MODE_ENTER_SHORT_ADDRESS;
                pendingState = STATE_GET_NETWORK_TOPOLOGY;  //when a full short address has been entered
                return STATE_IDLE;
            } 
#endif          
        case 'j':
        case 'J':
            {
                printf("Enter eight byte long (MAC) address to find, for example '00124B0012345678'\r\n");
                commandLineInterfaceMode = CLI_MODE_ENTER_LONG_ADDRESS;
                pendingState = STATE_FIND_VIA_LONG_ADDRESS;  //when a full long address has been entered
                return STATE_IDLE;
            }  
            
        case 'L':
        case 'l':
            {
                printf("Enter long (MAC) address of destination, for example '00124B0012345678'\r\n");
                commandLineInterfaceMode = CLI_MODE_ENTER_LONG_ADDRESS;
                pendingState = STATE_SEND_MESSAGE_VIA_LONG_ADDRESS;  //when a full long address has been entered
                return STATE_IDLE;
            }
            
        case 'n':
        case 'N':
            return STATE_DISPLAY_NETWORK_INFORMATION;
        case 'r':
        case 'R':
            return STATE_INIT;
        case 'S':
        case 's':
            {
                printf("Enter short address of destination, for example '2F6B'\r\n");
                commandLineInterfaceMode = CLI_MODE_ENTER_SHORT_ADDRESS;    // Next, get a short address typed by the user
                pendingState = STATE_SEND_MESSAGE_VIA_SHORT_ADDRESS;  // After a full short address has been entered
                return STATE_IDLE;
            }
            
        case 'v':
        case 'V':
            {
                printf("Module Version Information:\r\n");
                if (sysVersion() == MODULE_SUCCESS)                  //gets the version string
                {
                    displaySysVersion();  // Display the contents of the received SYS_VERSION
                } else {
                    printf("ERROR\r\n");
                }
                return STATE_IDLE;
            }
            
            /* Note: more commands can be added here */
            
        default:
            printf("Unknown command %c\r\n", cmd);
        }
        return STATE_IDLE;
        
    } else if (commandLineInterfaceMode == CLI_MODE_ENTER_SHORT_ADDRESS)    //accepts two hex numbers (4 ASCII characters)
    {
        if (IS_VALID_HEXADECIMAL_CHARACTER(cmd))
        {
            TO_UPPER_CASE(cmd);
            printf("%c", cmd);  //echo output
            cliInputBuffer[cliInputBufferIndex++] = (char) cmd;
            if (cliInputBufferIndex == 4)  
            {
                cliInputBuffer[4] = 0; //null terminate it so we can treat it as a string
                //now attempt to convert it:
                long val = 0;
                errno = 0;      // used in stdlib.h
                val = strtol(cliInputBuffer, NULL, 16);    // Interpret the string as a hex number
                if (errno != 0) // Should have already been error checked, but validate anyway
                {
                    printf("strtol parse error\r\n");
                } else { //no errors
                    shortAddressEntered = (uint16_t) val;
                    //printf("Short Address = 0x%04X\r\n", shortAddressEntered);
                    printf("\r\n");
                    //we're all done, so clear out buffers:
                    commandLineInterfaceMode = CLI_MODE_NORMAL;
                    cliInputBufferIndex = 0;
                    return STATE_VALID_SHORT_ADDRESS_ENTERED;
                }
                commandLineInterfaceMode = CLI_MODE_NORMAL;
                cliInputBufferIndex = 0;
                return STATE_IDLE;
            }
            /* Continue, since our buffer isn't full yet - leave settings alone */
            
        } else {
            /* not a hex number! */
            printf("\r\nNumber must be in hex: 0..9 or a..f inclusive\r\nAborting\r\n");
            displayCommandLineInterfaceHelp();
            commandLineInterfaceMode = CLI_MODE_NORMAL;
            cliInputBufferIndex = 0;
        }
        return STATE_IDLE;
        
    } else if (commandLineInterfaceMode == CLI_MODE_ENTER_LONG_ADDRESS)  //accepts eight hex numbers (16 ASCII characters)
    {
        
        if (IS_VALID_HEXADECIMAL_CHARACTER(cmd))
        {
            TO_UPPER_CASE(cmd);
            printf("%c", cmd);  //echo output
            
            cliInputBuffer[cliInputBufferIndex++] = (char) cmd;
            
            /* To used a tokenized substring splitter method, add a '-' at the end of each 2 characters. */
            if ((cliInputBufferIndex != 0) && ((cliInputBufferIndex+1) % 3 == 0) && (cliInputBufferIndex != 23))
            {
                cliInputBuffer[cliInputBufferIndex++] = '-';
                printf("-");
            }
            
            if (cliInputBufferIndex == 23)
            {
#ifdef DISPLAY_LONG_ADDRESS_PARSING
                printf("\r\nParsed '%s' into:", cliInputBuffer);
#endif
                /* now attempt to convert it: */
                char *substr = NULL;
                substr = strtok(cliInputBuffer,"-");    // Initialize string splitter
                
                /* Loops until there are no more substrings*/
                uint8_t parsedMacIndex = 0;
                while(substr!=NULL)
                {
                    /* Process the substring */
                    long val = 0;
                    /* errno is used in stdlib.h to indicate a parse error */
                    errno = 0;     
                    val = strtol(substr, NULL, 16);    // Interpret the string as a hex number
                    if (errno != 0) // Should have already been error checked, but validate anyway
                    {
                        printf("strtol parse error\r\n");
                        commandLineInterfaceMode = CLI_MODE_NORMAL;
                        cliInputBufferIndex = 0;
                        return STATE_IDLE;
                        
                    } else { //no errors
                        longAddressEntered[parsedMacIndex] = (uint16_t) val;
#ifdef DISPLAY_LONG_ADDRESS_PARSING
                        printf("%02X ", longAddressEntered[parsedMacIndex]);
#endif
                    }
                    parsedMacIndex++;
                    
                    substr = strtok(NULL,"-");  //Get the next substring
                }
                printf("\r\n");
                
                /* Now we have the mac address */
                if (!((longAddressEntered[0] == 0x00) && (longAddressEntered[1] == 0x12) && (longAddressEntered[2] == 0x4b)))
                {
                    printf("Warning - MAC does not have 00-12-4B as first three bytes!\r\n");
                }
                commandLineInterfaceMode = CLI_MODE_NORMAL;
                cliInputBufferIndex = 0;
                return STATE_VALID_LONG_ADDRESS_ENTERED;
            }
            
            /* continue, since our buffer isn't full yet - leave settings alone */
        } else {  //not a hex number!
            printf("Number must be in hex: 0..9 or a..f inclusive\r\nAborting\r\n");
            displayCommandLineInterfaceHelp();
            commandLineInterfaceMode = CLI_MODE_NORMAL;
            cliInputBufferIndex = 0;
        }  
        
        return STATE_IDLE;
    }
    
    return STATE_IDLE;
}


/* @} */
