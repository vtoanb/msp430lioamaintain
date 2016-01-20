/**
* @file zdo.c
*
* @brief Methods that implement the Zigbee Device Objects (ZDO) interface.
* 
* The AF/ZDO interface is a more powerful version of the Simple API and allows you to configure, send, and receive Zigbee data.
* This file acts as an interface between the user's application and the Module physical interface.
* Module interface could be either SPI or UART.
* Refer to Interface Specification for more information.
*
* @note For more information, define ZDO_VERBOSE. It is recommended to define this on a per-project basis. 
* In IAR, this can be done in Project Options : C/C++ compiler : Preprocessor
* In the defined symbols box, add:
* ZDO_VERBOSE
*
* $Rev: 2099 $
* $Author: dsmith $
* $Date: 2014-04-09 09:33:18 -0700 (Wed, 09 Apr 2014) $
*
* @section support Support
* Please refer to the wiki at www.anaren.com/air-wiki-zigbee for more information. Additional support
* is available via email at the following addresses:
* - Questions on how to use the product: AIR@anaren.com
* - Feature requests, comments, and improvements:  featurerequests@teslacontrols.com
* - Consulting engagements: sales@teslacontrols.com
*
* @section license License
* Copyright (c) 2014 Tesla Controls. All rights reserved. This Software may only be used with an
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

#include "zdo.h"
#include "module.h"
#include "../HAL/hal.h"
#include "../Common/utilities.h"
#include "module_errors.h"
#include "zm_phy_spi.h"
#include <string.h>                 //for memcpy()
#include <stdint.h>

extern uint8_t zmBuf[ZIGBEE_MODULE_BUFFER_SIZE];

#define METHOD_ZDO_STARTUP_FROM_APP                    0x31
/** Starts the Zigbee stack in the Module using the settings from a previous afRegisterApplication().
After this start request process completes, the device is ready to send, receive, and route network traffic.
@note On a coordinator in a trivial test setup, it takes approximately 300mSec between sending 
START_REQUEST and receiving START_REQUEST_SRSP and then another 200-1000mSec from when we receive 
START_REQUEST_SRSP to when we receive START_CONFIRM. Set START_CONFIRM_TIMEOUT based on size of your network.
@note ZDO_STARTUP_FROM_APP field StartDelay not used
@pre afRegisterApplication() was a success.
@post We will see Device Status change to DEV_ROUTER, DEV_ZB_COORD, or DEV_END_DEVICE correspondingly if everything was ok.
*/
moduleResult_t zdoStartApplication()
{
#ifdef ZDO_VERBOSE    
    printf("Start Application with AF/ZDO...");
#endif    
#define ZDO_STARTUP_FROM_APP_PAYLOAD_LEN 1    
    zmBuf[0] = ZDO_STARTUP_FROM_APP_PAYLOAD_LEN;
    zmBuf[1] = MSB(ZDO_STARTUP_FROM_APP);
    zmBuf[2] = LSB(ZDO_STARTUP_FROM_APP);      
    
#define NO_START_DELAY 0
    zmBuf[3] = NO_START_DELAY;
    RETURN_RESULT(sendMessage(), ZDO_STARTUP_FROM_APP);
}


#define METHOD_ZDO_IEEE_ADDR_REQ                    0x32
#define METHOD_ZDO_IEEE_ADDR_RSP                    0x33
/** Requests a remote device's MAC Address (64-bit IEEE Address) given a short address.
@param shortAddress the short address to locate
@param requestType must be SINGLE_DEVICE_RESPONSE or INCLUDE_ASSOCIATED_DEVICES. 
If SINGLE_DEVICE_RESPONSE is selected, then only information about the requested device will be returned. 
If INCLUDE_ASSOCIATED_DEVICES is selected, then the short addresses of the selected device's children will be returned too.
@param startIndex If INCLUDE_ASSOCIATED_DEVICES was selected, then there may be too many children to 
fit in one ZDO_IEEE_ADDR_RSP message. So, use startIndex to get the next set of children's short addresses.
@post zmBuf contains the ZDO_IEEE_ADDR_RSP message.
*/
moduleResult_t zdoRequestIeeeAddress(uint16_t shortAddress, uint8_t requestType, uint8_t startIndex)
{
    RETURN_INVALID_PARAMETER_IF_TRUE(((requestType != SINGLE_DEVICE_RESPONSE) && (requestType != INCLUDE_ASSOCIATED_DEVICES)), METHOD_ZDO_IEEE_ADDR_REQ);
#ifdef ZDO_VERBOSE     
    printf("Requesting IEEE Address for short address %04X, requestType %s, startIndex %u\r\n", 
           shortAddress, (requestType == 0) ? "Single" : "Extended", startIndex);
#endif 
    
#define ZDO_IEEE_ADDR_REQ_PAYLOAD_LEN 4
    zmBuf[0] = ZDO_IEEE_ADDR_REQ_PAYLOAD_LEN;
    zmBuf[1] = MSB(ZDO_IEEE_ADDR_REQ);             
    zmBuf[2] = LSB(ZDO_IEEE_ADDR_REQ);      
    
    zmBuf[3] = LSB(shortAddress);
    zmBuf[4] = MSB(shortAddress);
    zmBuf[5] = requestType;
    zmBuf[6] = startIndex;
    
#ifdef ZDO_IEEE_ADDR_RSP_HANDLED_BY_APPLICATION           //Return control to main application
    RETURN_RESULT(sendMessage(), METHOD_ZDO_IEEE_ADDR_REQ);
#else
    RETURN_RESULT_IF_FAIL(sendMessage(), METHOD_ZDO_IEEE_ADDR_REQ);     
    
#define ZDO_IEEE_ADDR_RSP_TIMEOUT 10
    RETURN_RESULT_IF_FAIL(waitForMessage(ZDO_IEEE_ADDR_RSP, ZDO_IEEE_ADDR_RSP_TIMEOUT), METHOD_ZDO_IEEE_ADDR_RSP);
    RETURN_RESULT(zmBuf[ZDO_IEEE_ADDR_RSP_STATUS_FIELD], METHOD_ZDO_IEEE_ADDR_RSP);
#endif
}


#define METHOD_ZDO_NWK_ADDR_REQ                     0x34
#define METHOD_ZDO_NWK_ADDR_RSP                     0x35
/** Requests a remote device's Short Address for a given long address.
@param ieeeAddress the long address to locate, LSB first!
@param requestType must be SINGLE_DEVICE_RESPONSE or INCLUDE_ASSOCIATED_DEVICES. 
If SINGLE_DEVICE_RESPONSE is selected, then only information about the requested device will be returned. 
If INCLUDE_ASSOCIATED_DEVICES is selected, then the short addresses of the selected device's children will be returned too.
@param startIndex If INCLUDE_ASSOCIATED_DEVICES was selected, then there may be too many children to 
fit in one ZDO_NWK_ADDR_RSP message. So, use startIndex to get the next set of children's short addresses.
@note DOES NOT WORK FOR SLEEPING END DEVICES
@note may not work correctly when using UART interface
@post An ZDO_NWK_ADDR_RSP message will be received, with one or more entries.
*/
moduleResult_t zdoNetworkAddressRequest(uint8_t* ieeeAddress, uint8_t requestType, uint8_t startIndex)
{
    RETURN_INVALID_PARAMETER_IF_TRUE(((requestType != SINGLE_DEVICE_RESPONSE) && (requestType != INCLUDE_ASSOCIATED_DEVICES)), METHOD_ZDO_NWK_ADDR_REQ);
    
#ifdef ZDO_VERBOSE     
    printf("Requesting Network Address for long address ");
    printHexBytes(ieeeAddress, 8);
    printf("requestType %s, startIndex %u\r\n", (requestType == 0) ? "Single" : "Extended", startIndex);
#endif
#define ZDO_NWK_ADDR_REQ_PAYLOAD_LEN 10
    zmBuf[0] = ZDO_NWK_ADDR_REQ_PAYLOAD_LEN;
    zmBuf[1] = MSB(ZDO_NWK_ADDR_REQ);
    zmBuf[2] = LSB(ZDO_NWK_ADDR_REQ);      
    
    memcpy(zmBuf+3, ieeeAddress, 8);
    zmBuf[11] = requestType;
    zmBuf[12] = startIndex;
    
#ifdef ZDO_NWK_ADDR_RSP_HANDLED_BY_APPLICATION  //Main application will wait for ZDO_NWK_ADDR_RSP message.    
    RETURN_RESULT(sendMessage(), METHOD_ZDO_NWK_ADDR_REQ);
#else
    RETURN_RESULT_IF_FAIL(sendMessage(), METHOD_ZDO_NWK_ADDR_REQ);     
    
#define ZDO_NWK_ADDR_RSP_TIMEOUT 10
    RETURN_RESULT_IF_FAIL(waitForMessage(ZDO_NWK_ADDR_RSP, ZDO_NWK_ADDR_RSP_TIMEOUT), METHOD_ZDO_NWK_ADDR_RSP);
    RETURN_RESULT(zmBuf[ZDO_NWK_ADDR_RSP_STATUS_FIELD], METHOD_ZDO_NWK_ADDR_RSP);
#endif
}


/** Displays the returned value of ZdoUserDescriptorRequest()
@param rsp points to the beginning of the response
@pre rsp holds a valid response
*/
void displayZdoUserDescriptorResponse(uint8_t* rsp)
{
    if (rsp[2] != MODULE_SUCCESS)
    {
        printf("Failed (Error Code %02X)\r\n", rsp[0]);
        return;
    }
        printf("User Descriptor Response Received - Source Address=0x%04X, Network Address=0x%04X\r\n", CONVERT_TO_INT(rsp[0], rsp[1]), CONVERT_TO_INT(rsp[3], rsp[4]));
        printf("Length=%u, User Descriptor=", rsp[5]);
        // Display the user descriptor
        int i;
        for (i=0; i<rsp[5]; i++)
            printf("%c", rsp[6+i]);
        printf("\r\n");
}


/** Displays the returned value of ZdoNodeDescriptorRequest()
@param rsp points to the beginning of the response
@pre rsp holds a valid response
*/
void displayZdoNodeDescriptorResponse(uint8_t* rsp)
{
    
    if (rsp[2] != MODULE_SUCCESS)
    {
        printf("Failed (Error Code %02X)\r\n", rsp[0]);
        return;
    }
    
    printf("Node Descriptor Response Received - Source Address=0x%04X, Network Address=0x%04X\r\n", CONVERT_TO_INT(rsp[0], rsp[1]), CONVERT_TO_INT(rsp[3], rsp[4]));
    printf("    Logical Type / ComplexDesc / User Desc = 0x%02X\r\n", rsp[5]);
    printf("    APSFlags / Frequency Band = %02X\r\n", rsp[6]);
    printf("    Mac Capabilities = 0x%02X\r\n", rsp[7]);
    printf("    Manufacturer Code = 0x%04X\r\n", CONVERT_TO_INT(rsp[8], rsp[9]));
    printf("    Max Buffer Size = 0x%02X\r\n", rsp[10]);
    printf("    Max In Transfer Size = 0x%04X\r\n", CONVERT_TO_INT(rsp[11], rsp[12]));
    printf("    Server Mask = 0x%04X\r\n", CONVERT_TO_INT(rsp[13], rsp[14]));
    printf("    Max Out Transfer Size = 0x%04X\r\n", CONVERT_TO_INT(rsp[15], rsp[16]));
    printf("    Descriptor Capabilities = 0x%02X\r\n", rsp[17]);    
}


/** Displays the returned value of zdoNetworkAddressRequest() or zdoRequestIeeeAddress()
Both response messages use the same format.
@param rsp points to the beginning of the response
@pre rsp holds a valid response
*/
void displayZdoAddressResponse(uint8_t* rsp)
{
    if (rsp[0] != MODULE_SUCCESS)
    {
        printf("Failed to find that device (Error Code %02X)\r\n", rsp[0]);
    }
    else
    {
        printf("Device Found! MAC (MSB first): ");
        int i;
        for (i=8; i > 0; i--)
            printf("%02X ", rsp[i]);
        //printf("%02X", rsp[1]); // Don't include space after last byte of MAC
        printf(", Short Address:%04X\r\n", CONVERT_TO_INT(rsp[ZDO_IEEE_ADDR_RSP_SHORT_ADDRESS_FIELD_START], rsp[ZDO_IEEE_ADDR_RSP_SHORT_ADDRESS_FIELD_START + 1]));
        uint8_t numAssociatedDevices = rsp[ZDO_IEEE_ADDR_RSP_NUMBER_OF_ASSOCIATED_DEVICES_FIELD];
        if (numAssociatedDevices > 0)
        {
            printf("%u Associated Devices, starting with #%u:", numAssociatedDevices, rsp[ZDO_IEEE_ADDR_RSP_START_INDEX_FIELD]);
            int j;
            for (j = 0; j < (rsp[ZDO_IEEE_ADDR_RSP_NUMBER_OF_ASSOCIATED_DEVICES_FIELD] + 1); j += 2)
                printf("(%04X) ", CONVERT_TO_INT(rsp[(j+ZDO_IEEE_ADDR_RSP_ASSOCIATED_DEVICE_FIELD_START)], rsp[( j + ZDO_IEEE_ADDR_RSP_ASSOCIATED_DEVICE_FIELD_START + 1)]));
            printf("\r\n");
        } else {
            printf("No Associated Devices\r\n");
        }
    }
}


// Referenced from start of payload (byte 3 in SPI)
#define ZDO_MGMT_PERMIT_JOIN_SHORT_ADDRESS_FIELD_START           0
#define ZDO_MGMT_PERMIT_JOIN_STATUS_FIELD                        2
/** Parses and displays a ZD0_MGMT_PERMIT_JOIN_RSP message which is received after a ZDO_MGMT_PERMIT_JOIN_REQ.
@param rsp points to the beginning of the response
@pre rsp holds a valid response
*/
void displayZdoManagementPermitJoinResponse(uint8_t* rsp)
{
    printf("Permit Join for Short Address %04X: ", CONVERT_TO_INT(rsp[ZDO_MGMT_PERMIT_JOIN_SHORT_ADDRESS_FIELD_START], 
                                                                 rsp[ZDO_MGMT_PERMIT_JOIN_SHORT_ADDRESS_FIELD_START + 1]));
    if (rsp[ZDO_MGMT_PERMIT_JOIN_STATUS_FIELD] == MODULE_SUCCESS)
        printf("Success\r\n");
    else
        printf("Failure\r\n");
}


/** Display a ZDO_MGMT_LEAVE_RSP message contents */
void displayZdoManagementLeaveResponse(uint8_t* rsp)
{
    printf("Leave Response from %04X: ", (GET_ZDO_MGMT_LEAVE_RSP_SRC_ADDRESS()));
    if (rsp[ZDO_MGMT_LEAVE_RSP_STATUS_FIELD] == MODULE_SUCCESS)
        printf("Success\r\n");
    else
        printf("Failure\r\n");
}


/** Display a ZDO_JOIN_CNF message contents */
void displayZdoJoinConfirm(uint8_t* rsp)
{
    printf("Join Confirm from Device %04X, Parent %04X: ", (GET_ZDO_JOIN_CNF_SRC_ADDRESS()), (GET_ZDO_JOIN_CNF_PARENT_ADDRESS()));
    if (rsp[ZDO_JOIN_CNF_STATUS_FIELD] == MODULE_SUCCESS)
        printf("Success\r\n");
    else
        printf("Failure\r\n");
}

/** Display the contents of a ZDO_STATE_CHANGE_IND message */
void displayZdoStateChangeInd(uint8_t* rsp)
{
    printf("Module State Change to %s (%02X)\r\n", getDeviceStateName(rsp[ZDO_STATE_CHANGE_IND_STATE]), rsp[ZDO_STATE_CHANGE_IND_STATE]);
}


/** Displays the parsed fields in a ZDO_END_DEVICE_ANNCE_IND message 
@param announce points to the start of a ZDO_END_DEVICE_ANNCE_IND message (start of zmBuf)
*/
void displayZdoEndDeviceAnnounce(uint8_t* announce)
{    
    if (announce[ZDO_END_DEVICE_ANNCE_IND_CAPABILITIES_FIELD] & 
        ZDO_END_DEVICE_ANNCE_IND_CAPABILITIES_FLAG_DEVICETYPE_ROUTER)
        printf("ROUTER ");
    else
        printf("END DEVICE ");
    printf("Announce From:%04X Addr:%04X MAC:", GET_ZDO_END_DEVICE_ANNCE_IND_FROM_ADDRESS(), GET_ZDO_END_DEVICE_ANNCE_IND_SRC_ADDRESS());
    int i;
    for (i=ZDO_END_DEVICE_ANNCE_IND_MAC_START_FIELD + 7; i>=ZDO_END_DEVICE_ANNCE_IND_MAC_START_FIELD; i--)
    {
        printf("%02X", announce[i]);
    }
    printf(" Capabilities:%02X\r\n", announce[ZDO_END_DEVICE_ANNCE_IND_CAPABILITIES_FIELD]);
}

/** Utility method used to display a Network List array contained in a displayZdoNetworkDiscoveryResponse */
static void displayNetworkList(uint8_t* nwkListPtr)
{
	printf("MAC:");
    displayReverseHexBytes(nwkListPtr, 8, DISPLAY_HEX_BYTES_NO_SEPARATOR);
	printf(" Chan=%u Stack/Ver:%02X Beacon/SupFrame:%02X Join=", nwkListPtr[8], nwkListPtr[9], nwkListPtr[10]);
	if (nwkListPtr[11] == 1)
	{
		printf("ON\r\n");
	} else {
		printf("OFF\r\n");
	}
}


#define ZDO_MGMT_NWK_DISC_RSP__STATUS                   (SRSP_PAYLOAD_START+2)
#define ZDO_MGMT_NWK_DISC_RSP__NWK_CNT                  (SRSP_PAYLOAD_START+3)
#define ZDO_MGMT_NWK_DISC_RSP__START                    (SRSP_PAYLOAD_START+4)
#define ZDO_MGMT_NWK_DISC_RSP__NWK_LST_CNT          	(SRSP_PAYLOAD_START+5)
#define GET_ZDO_MGMT_NWK_DISC_RSP_SRC_ADDRESS()      	(CONVERT_TO_INT(zmBuf[ZDO_MGMT_NWK_DISC_RSP__SRC_ADDRESS_LSB], zmBuf[ZDO_MGMT_NWK_DISC_RSP__SRC_ADDRESS_MSB]))

/** Displays the parsed fields in a ZDO_MGMT_NWK_DISC_RSP message */
void displayZdoNetworkDiscoveryResponse(uint8_t* rsp)
{
#define NETWORK_LIST_LENGTH								12
    printf("ZdoNetworkDiscoveryResponse From:%04X Status:%02X NwkCnt:%02X Start:%02X NwkLstCnt:%02X\r\n",
    		GET_ZDO_MGMT_NWK_DISC_RSP_SRC_ADDRESS(), zmBuf[ZDO_MGMT_NWK_DISC_RSP__STATUS],
    		zmBuf[ZDO_MGMT_NWK_DISC_RSP__NWK_CNT], zmBuf[ZDO_MGMT_NWK_DISC_RSP__START],
    		zmBuf[ZDO_MGMT_NWK_DISC_RSP__NWK_LST_CNT]);
    uint8_t recordCount = zmBuf[ZDO_MGMT_NWK_DISC_RSP__NWK_LST_CNT];
    int i;
    /* nwkListPtr will point to the beginning of each Network List Record */
    uint8_t* nwkListPtr = zmBuf + ZDO_MGMT_NWK_DISC_RSP__NWK_LST_CNT + 1;
    for ( i =0; i<recordCount; i++)
    {
    	displayNetworkList(nwkListPtr);
    	nwkListPtr += NETWORK_LIST_LENGTH;
    }
}


#define METHOD_ZDO_USER_DESC_REQ                    0x36
#define METHOD_ZDO_USER_DESC_RSP                    0x37
/** Requests a remote device's user descriptor. This is a 16 byte text field that may be used for
anything and may be read/written remotely.
@param destinationAddress the short address of the destination
@param networkAddressOfInterest the short address of the device the query is intended for
@post zmBuf contains the ZDO_USER_DESC_RSP message.
*/
moduleResult_t zdoUserDescriptorRequest(uint16_t destinationAddress, uint16_t networkAddressOfInterest)
{
#ifdef ZDO_VERBOSE     
    printf("Requesting User Descriptor for destination %04X, NWK address %04X\r\n", destinationAddress, networkAddressOfInterest);
#endif 
    
#define ZDO_USER_DESC_REQ_PAYLOAD_LEN 4
    zmBuf[0] = ZDO_USER_DESC_REQ_PAYLOAD_LEN;
    zmBuf[1] = MSB(ZDO_USER_DESC_REQ);             
    zmBuf[2] = LSB(ZDO_USER_DESC_REQ);      
    
    zmBuf[3] = LSB(destinationAddress);
    zmBuf[4] = MSB(destinationAddress);
    zmBuf[5] = LSB(networkAddressOfInterest);
    zmBuf[6] = MSB(networkAddressOfInterest);
    
#ifdef ZDO_USER_DESC_RSP_HANDLED_BY_APPLICATION           //Return control to main application
    RETURN_RESULT(sendMessage(), METHOD_ZDO_USER_DESC_REQ);
#else
    RETURN_RESULT_IF_FAIL(sendMessage(), METHOD_ZDO_USER_DESC_REQ);     
    
    // Now wait for the response...
#define ZDO_USER_DESC_RSP_TIMEOUT 10
    RETURN_RESULT_IF_FAIL(waitForMessage(ZDO_USER_DESC_RSP, ZDO_USER_DESC_RSP_TIMEOUT), METHOD_ZDO_USER_DESC_RSP);
    RETURN_RESULT(zmBuf[ZDO_USER_DESC_RSP_STATUS_FIELD], METHOD_ZDO_USER_DESC_RSP);
#endif
}


#define METHOD_ZDO_NODE_DESC_REQ                    0x38
#define METHOD_ZDO_NODE_DESC_RSP                    0x39
/** Requests a remote device's node descriptor. 
@param destinationAddress the short address of the destination
@param networkAddressOfInterest the short address of the device the query is intended for
@post zmBuf contains the ZDO_NODE_DESC_RSP message.
*/
moduleResult_t zdoNodeDescriptorRequest(uint16_t destinationAddress, uint16_t networkAddressOfInterest)
{
#ifdef ZDO_VERBOSE     
    printf("Requesting Node Descriptor for destination %04X, NWK address %04X\r\n", destinationAddress, networkAddressOfInterest);
#endif 
    
#define ZDO_NODE_DESC_REQ_PAYLOAD_LEN 4
    zmBuf[0] = ZDO_USER_DESC_REQ_PAYLOAD_LEN;
    zmBuf[1] = MSB(ZDO_NODE_DESC_REQ);             
    zmBuf[2] = LSB(ZDO_NODE_DESC_REQ);      
    
    zmBuf[3] = LSB(destinationAddress);
    zmBuf[4] = MSB(destinationAddress);
    zmBuf[5] = LSB(networkAddressOfInterest);
    zmBuf[6] = MSB(networkAddressOfInterest);
    
#ifdef ZDO_NODE_DESC_RSP_HANDLED_BY_APPLICATION           //Return control to main application
    RETURN_RESULT(sendMessage(), METHOD_ZDO_NODE_DESC_REQ);
#else
    RETURN_RESULT_IF_FAIL(sendMessage(), METHOD_ZDO_NODE_DESC_REQ);     
    
    // Now wait for the response...
#define ZDO_NODE_DESC_RSP_TIMEOUT 10
    RETURN_RESULT_IF_FAIL(waitForMessage(ZDO_NODE_DESC_RSP, ZDO_NODE_DESC_RSP_TIMEOUT), METHOD_ZDO_NODE_DESC_RSP);
    RETURN_RESULT(zmBuf[ZDO_NODE_DESC_RSP_STATUS_FIELD], METHOD_ZDO_NODE_DESC_RSP);
#endif
}


#define METHOD_ZDO_USER_DESC_SET                    0x3A
/** Sets a remote device's user descriptor. 
@param destinationAddress the short address of the destination
@param networkAddressOfInterest the short address of the device
*/
moduleResult_t zdoUserDescriptorSet(uint16_t destinationAddress, uint16_t networkAddressOfInterest, 
                                    uint8_t* userDescriptor, uint8_t userDescriptorLength)
{
#ifdef ZDO_VERBOSE     
    printf("Setting User Descriptor for destination %04X, NWK address %04X, length %u\r\n", 
           destinationAddress, networkAddressOfInterest, userDescriptorLength);
#endif 
    
#define ZDO_USER_DESC_SET_PAYLOAD_LEN_HEADER   5
    zmBuf[0] = ZDO_USER_DESC_SET_PAYLOAD_LEN_HEADER + userDescriptorLength;
    zmBuf[1] = MSB(ZDO_USER_DESC_SET);             
    zmBuf[2] = LSB(ZDO_USER_DESC_SET);      
    
    zmBuf[3] = LSB(destinationAddress);
    zmBuf[4] = MSB(destinationAddress);
    zmBuf[5] = LSB(networkAddressOfInterest);
    zmBuf[6] = MSB(networkAddressOfInterest);
    zmBuf[7] = userDescriptorLength;
    memcpy(zmBuf+8, userDescriptor, userDescriptorLength);
    
    displayZmBuf();

    RETURN_RESULT(sendMessage(), METHOD_ZDO_USER_DESC_SET);
}


#define METHOD_ZDO_MGMT_PERMIT_JOIN_REQ 0x3B
#define METHOD_ZDO_MGMT_PERMIT_JOIN_RSP 0x3C
/** Set joining permission on or off for the destination device as specified by short address. If
joining is on then other devices can join this device. If joining is off then other devices will not
be able to join.
@param destinationAddress the short address of the device to set joining permissions
@param duration how long to allow joining for:
- PERMIT_JOIN_OFF (0x00): join disabled
- 0x01-0xFE: the number of seconds to permit joining
- PERMIT_JOIN_ON_INDEFINITELY (0xFF): join enabled indefinitely
@param tcSignificance trust center significance; for future use but for now must be set to 0
@post zmBuf contains the ZDO_MGMT_PERMIT_JOIN_RSP message.
*/
moduleResult_t zdoManagementPermitJoinRequest(uint16_t destinationAddress, uint8_t duration, uint8_t tcSignificance)
{
    RETURN_INVALID_PARAMETER_IF_TRUE((tcSignificance != 0), METHOD_ZDO_MGMT_PERMIT_JOIN_REQ);

#ifdef ZDO_VERBOSE     
    printf("Setting Joining Permissions for destination %04X to ", destinationAddress);
    if (duration == PERMIT_JOIN_OFF)
        printf("OFF\r\n");
    else if (duration == PERMIT_JOIN_ON_INDEFINITELY)
        printf("ON indefinitely\r\n");
    else
        printf("ON for %uS\r\n", duration);
#endif 
    
#define ZDO_MGMT_PERMIT_JOIN_REQ_PAYLOAD_LEN 3
    zmBuf[0] = ZDO_MGMT_PERMIT_JOIN_REQ_PAYLOAD_LEN;
    zmBuf[1] = MSB(ZDO_MGMT_PERMIT_JOIN_REQ);             
    zmBuf[2] = LSB(ZDO_MGMT_PERMIT_JOIN_REQ);      
    
    zmBuf[3] = LSB(destinationAddress);
    zmBuf[4] = MSB(destinationAddress);
    zmBuf[5] = duration;
    zmBuf[6] = tcSignificance;
    
#ifdef ZDO_MGMT_PERMIT_JOIN_RSP_HANDLED_BY_APPLICATION           //Return control to main application
    RETURN_RESULT(sendMessage(), METHOD_ZDO_MGMT_PERMIT_JOIN_REQ);
#else
    RETURN_RESULT_IF_FAIL(sendMessage(), METHOD_ZDO_MGMT_PERMIT_JOIN_REQ);     
    
    // Now wait for the response...
#define ZDO_MGMT_PERMIT_JOIN_RSP_TIMEOUT 10
    RETURN_RESULT_IF_FAIL(waitForMessage(ZDO_MGMT_PERMIT_JOIN_RSP, ZDO_MGMT_PERMIT_JOIN_RSP_TIMEOUT), METHOD_ZDO_MGMT_PERMIT_JOIN_RSP);
    // Note: we do not verify that the source address of the received ZDO_MGMT_PERMIT_JOIN_RSP is the same as the destinationAddress method parameter
    RETURN_RESULT(zmBuf[ZDO_MGMT_PERMIT_JOIN_RSP_STATUS_FIELD], METHOD_ZDO_MGMT_PERMIT_JOIN_RSP);
#endif
}


#define METHOD_ZDO_NWK_DISCOVERY_REQ                     0x3D
/** Performs an active network scan by sending out beacons. The device must NOT be on a network. If
it is then an error will be returned.
@param channelMask bitmap of which channels to scan. If channelMask = ANY_CHANNEL then the radio
will scan all channels. Must be between MIN_CHANNEL_MASK and MAX_CHANNEL_MASK, or ANY_CHANNEL.
@param scanDuration how long to scan each channel - must be a BEACON_ORDER_x value. Refer to zdo.h
for a list of allowed values. Available values are BEACON_ORDER_15_MSEC to BEACON_ORDER_4_MINUTES.
Usually we use BEACON_ORDER_480_MSEC, but you may want to adjust for your application.@post A ZDO_BEACON_NOTIFY_IND message will be received for each network found, followed by a
ZDO_NWK_DISCOVERY_CNF when the scan is complete.
@note example usage: zdoNetworkDiscoveryRequest(ANY_CHANNEL_MASK, BEACON_ORDER_480_MSEC)
@note The time that it takes to run a scan is equal to (scan time) * (number of channels). So if
using ANY_CHANNEL and BEACON_ORDER_480_MSEC then it will take approximately 7 seconds.
@return the standard error code, including:
- MODULE_SUCCESS if success
- 0x02:Invalid Parameter
- 0xC2:ZNwkInvalidRequest if the device is already on the network then use ZDO_MGMT_NWK_DISC_REQ 
instead, or just leave the network with a ZDO_MGMT_LEAVE_REQ command.
- 0xFC: MAC scan in progress
- 0x1A: no memory resource available inside the module
*/
moduleResult_t zdoNetworkDiscoveryRequest(uint32_t channelMask, uint8_t scanDuration)
{
    RETURN_INVALID_PARAMETER_IF_TRUE( ((channelMask < MIN_CHANNEL_MASK) || (channelMask > MAX_CHANNEL_MASK)), METHOD_ZDO_NWK_DISCOVERY_REQ);

#define ZDO_NWK_DISCOVERY_REQ_PAYLOAD_LEN 5
    zmBuf[0] = ZDO_NWK_DISCOVERY_REQ_PAYLOAD_LEN;
    zmBuf[1] = MSB(ZDO_NWK_DISCOVERY_REQ);
    zmBuf[2] = LSB(ZDO_NWK_DISCOVERY_REQ);  
    
    zmBuf[3] = LSB(channelMask);
    zmBuf[4] = (channelMask & 0xFF00) >> 8;
    zmBuf[5] = (channelMask & 0xFF0000) >> 16;
    zmBuf[6] = channelMask >> 24;    
    zmBuf[7] = scanDuration;

#ifdef MODULE_INTERFACE_VERBOSE 
    printf("Scanning for Networks with duration %02X and Channel List (LSB first): %02X %02X %02X %02X\r\n", scanDuration, zmBuf[3], zmBuf[4], zmBuf[5], zmBuf[6]);
#endif   
    
    RETURN_RESULT(sendMessage(), METHOD_ZDO_NWK_DISCOVERY_REQ);
}


#define METHOD_ZDO_MGMT_NWK_DISCOVERY_REQ                     0x71
/** Request that the destination device performs an active network scan by sending out beacons.
Unlike zdoNetworkDiscoveryRequest, this function MAY be used while the device is ON the network.
Note that if the device is doing a scan on other RF channels then it will temporarily not be able to
receive messages since the radio may not be on the same channel. In this case, another device
trying to send a message to this device while it's busy scanning will return an error of 0x19.
@param destinationAddress the short address of the device which we want to do the scan.
@param channelMask bitmap of which channels to scan. If channelMask = ANY_CHANNEL then the radio
will scan all channels. Must be between MIN_CHANNEL_MASK and MAX_CHANNEL_MASK, or ANY_CHANNEL.
@param scanDuration how long to scan each channel - must be a BEACON_ORDER_x value. Refer to zdo.h
for a list of allowed values. Available values are BEACON_ORDER_15_MSEC to BEACON_ORDER_4_MINUTES.
Usually we use BEACON_ORDER_480_MSEC, but you may want to adjust for your application.
@param startIndex where to start in the response array list. The result can contain more entries
than can be reported, so this field allows the user to retrieve the responses anywhere in the array list.
@post A ZDO_MGMT_NWK_DISC_RSP message will be received containing the results of the scan
@note example usage to have the coordinator do a scan: zdoManagementNetworkDiscoveryRequest(0x0000, ANY_CHANNEL_MASK, BEACON_ORDER_480_MSEC, 0)
@note The time that it takes to run a scan is equal to (scan time) * (number of channels). So if
using ANY_CHANNEL and BEACON_ORDER_480_MSEC then it will take approximately 7 seconds.
@return the standard error code
*/
moduleResult_t zdoManagementNetworkDiscoveryRequest(uint16_t destinationAddress,
		uint32_t channelMask, uint8_t scanDuration, uint8_t startIndex)
{
    RETURN_INVALID_PARAMETER_IF_TRUE( ((channelMask < MIN_CHANNEL_MASK) || (channelMask > MAX_CHANNEL_MASK)), METHOD_ZDO_MGMT_NWK_DISCOVERY_REQ);

#define ZDO_MGMT_NWK_DISCOVERY_REQ_PAYLOAD_LEN 8
    zmBuf[0] = ZDO_MGMT_NWK_DISCOVERY_REQ_PAYLOAD_LEN;
    zmBuf[1] = MSB(ZDO_MGMT_NWK_DISCOVERY_REQ);
    zmBuf[2] = LSB(ZDO_MGMT_NWK_DISCOVERY_REQ);

    zmBuf[3] = LSB(destinationAddress);
    zmBuf[4] = MSB(destinationAddress);
    zmBuf[5] = LSB(channelMask);
    zmBuf[6] = (channelMask & 0xFF00) >> 8;
    zmBuf[7] = (channelMask & 0xFF0000) >> 16;
    zmBuf[8] = channelMask >> 24;
    zmBuf[9] = scanDuration;
    zmBuf[10] = startIndex;

#ifdef MODULE_INTERFACE_VERBOSE
    printf("Scanning for Networks with Dest:%04X Duration:%02X #%u ChanList:", destinationAddress, scanDuration, startIndex);
    displayReverseHexBytes(zmBuf+5, 4, DISPLAY_HEX_BYTES_NO_SEPARATOR);
    printf("\r\n");
#endif

    RETURN_RESULT(sendMessage(), METHOD_ZDO_MGMT_NWK_DISCOVERY_REQ);
}


#define METHOD_ZDO_JOIN_REQ									0x72
/** Join a specific network, selected by its RF channel and panId.
This is an advanced feature and should not normally be needed. Use with caution.
@param channel which RF channel we want to use for our join - this must be known
@param panId the PAN ID which we wish to join - this must be known
@param parent the Short Address of who we want to be our parent. Normally we set to 0 for Coordinator.
@param depth the desired depth that we want to be on. Normally we set to 0.
@note example usage to join the coordinator of PANID 0x8325 on channel 20: zdoJoinRequest(20, 0x8325, 0, 0);
*/
moduleResult_t zdoJoinRequest(uint8_t channel, uint16_t panId, uint16_t parent, uint8_t depth)
{
#define ZIGBEE_2007_PRO_STACK_PROFILE		0x02
    RETURN_INVALID_PARAMETER_IF_TRUE((!(IS_VALID_CHANNEL(channel))), METHOD_ZDO_JOIN_REQ);
    RETURN_INVALID_PARAMETER_IF_TRUE((!(IS_VALID_PANID(panId))), METHOD_ZDO_JOIN_REQ);

#define ZDO_JOIN_REQ_PAYLOAD_LEN 0x0F
    zmBuf[0] = ZDO_JOIN_REQ_PAYLOAD_LEN;
    zmBuf[1] = MSB(ZDO_JOIN_REQ);
    zmBuf[2] = LSB(ZDO_JOIN_REQ);

    zmBuf[3] = channel;
    zmBuf[4] = LSB(panId);
    zmBuf[5] = MSB(panId);
    /* zmBuf[6] ... zmBuf[13] should be all 0xFF since the version of Z-Stack we use is > version 1.1 */
    memset(zmBuf+6, 0xFF, 8);
    zmBuf[14] = LSB(parent);
    zmBuf[15] = MSB(parent);
    zmBuf[16] = depth;
    zmBuf[17] = ZIGBEE_2007_PRO_STACK_PROFILE;

#ifdef MODULE_INTERFACE_VERBOSE
    printf("Request Join on Channel=%u panID:%04X parent:%04X depth:%02X\r\n", channel, panId, parent, depth);
#endif

    /* Note: we do NOT wrap sendMessage() with RETURN_RESULT() macro because ZDO_JOIN_REQ is an
     asynchronous message - it does not return a SRSP. If we did wrap with RETURN_RESULT then we'll
     have an error. So, we perform the error suppression below - if we get an ZM_PHY_INCORRECT_SRSP
      then ignore it */
    moduleResult_t err = sendMessage();
    if ((err == ZM_PHY_INCORRECT_SRSP) || (err == MODULE_SUCCESS)) // Ignore ZM_PHY_INCORRECT_SRSP
    {
    	return MODULE_SUCCESS;
    } else {
    	return err; // But return any other kind of error
    }
}


#define METHOD_ZDO_MGMT_LEAVE_REQ                     0x3F
#define METHOD_ZDO_MGMT_LEAVE_RSP                     0x70
/** Requests a device to leave the network.
@param ieeeAddress the long address to request to leave
@param destinationAddress the short address of the device generating the request.
*/
moduleResult_t zdoManagementLeaveRequest(uint8_t* ieeeAddress, uint16_t destinationAddress)
{
#ifdef ZDO_VERBOSE
    printf("Requesting Leave for long address ");
    printHexBytes(ieeeAddress, 8);
    printf(" and destinationAddress 0x%04X\r\n", destinationAddress);
#endif
#define ZDO_MGMT_LEAVE_REQ_PAYLOAD_LEN 0x0B
    zmBuf[0] = ZDO_MGMT_LEAVE_REQ_PAYLOAD_LEN;
    zmBuf[1] = MSB(ZDO_MGMT_LEAVE_REQ);
    zmBuf[2] = LSB(ZDO_MGMT_LEAVE_REQ);

    zmBuf[3] = MSB(destinationAddress);
    zmBuf[4] = LSB(destinationAddress);

    memcpy(zmBuf+5, ieeeAddress, 8);
#define NO_REMOVE_CHILDREN_REJOIN  0   // Must be set to 0; see spec
    zmBuf[13] = NO_REMOVE_CHILDREN_REJOIN;

    //RETURN_RESULT(sendMessage(), METHOD_ZDO_MGMT_LEAVE_REQ);

#ifdef ZDO_MGMT_LEAVE_RSP_HANDLED_BY_APPLICATION           //Return control to main application
    RETURN_RESULT(sendMessage(), METHOD_ZDO_MGMT_LEAVE_REQ);
#else
    RETURN_RESULT_IF_FAIL(sendMessage(), METHOD_ZDO_MGMT_LEAVE_REQ);
printf(":) ");
    // Now wait for the response...
#define ZDO_MGMT_LEAVE_RSP_TIMEOUT 10
    RETURN_RESULT_IF_FAIL(waitForMessage(ZDO_MGMT_LEAVE_RSP, ZDO_MGMT_LEAVE_RSP_TIMEOUT), METHOD_ZDO_MGMT_LEAVE_RSP);
    // Note: we do not verify that the source address of the received ZDO_MGMT_LEAVE_RSP is the same as the destinationAddress method parameter
    printf(":) \r\n");
    RETURN_RESULT(zmBuf[ZDO_MGMT_LEAVE_RSP_STATUS_FIELD], METHOD_ZDO_MGMT_LEAVE_RSP);
#endif
}

