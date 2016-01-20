/**
 * @file hal_gw1.c
 *
 * @brief Hardware Abstraction Layer (HAL) for the Zigbee to Ethernet Gateway GW-0.
 *
 * Tools to interact with the GW-0 onboard peripherals for Module interface. The Gw0 is a Zigbee to
 * Ethernet gateway based on the TI LM3S6965 Arm Cortex-M3 processor.
 * Since the GW-0 is an existing Stellaris Project with a Module "grafted in", things are a little different here.
 *
 * The Module library & examples require the following methods to be implemented.
 * Refer to individual method descriptions for more information.
 * - halInit()    //Not implemented, handled elsewhere in the project
 * - putchar()    //Not implemented, handled elsewhere in the project
 * - halSpiInit()
 * - delayMs()
 * - halResetModule()
 * - toggleLed()
 *
 * Copyright (c) 2012 Tesla Controls. All rights reserved. Redistribution and use in source and
 * binary forms, with or without modification, are permitted only if used solely and exclusively with
 * an Anaren Zigbee 2007 Module. Refer to the file "eula.txt" for additional restrictions and
 * limitations on this Software.
 *
 * $Rev: 1863 $
 * $Author: dsmith $
 * $Date: 2013-06-13 15:23:41 -0700 (Thu, 13 Jun 2013) $
 */
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "inc/hw_ssi.h"
#include "inc/hw_i2c.h"
#include "driverlib/ethernet.h"
#include "driverlib/flash.h"
#include "driverlib/gpio.h"
#include "driverlib/ssi.h"
#include "driverlib/i2c.h"
#include "driverlib/interrupt.h"  //for UART2 interrupt
#include "driverlib/uart.h"
#include "driverlib/systick.h"

#include "hal_gw1.h"
//#include "hal.h"
#include <stdint.h>
//for UART:
#include "utils/uartstdio.h"
#include "utils/ustdlib.h"

void doNothing(int8_t a);

/** This is a function pointer for the Interrupt Service Routine called when a debug console
character is received. To use it, declare it with
<code> extern void (*debugConsoleIsr)(int8_t);  </code>
and then point it to a function you created, e.g.
<code> debugConsoleIsr = &handleDebugConsoleInterrupt;  </code>
and your function handleDebugConsoleInterrupt() will be called when a byte is received on the debug
serial port.
Obviously your handleDebugConsoleInterrupt must have the same method signature as the
function pointer. In other words, your function should be declared as:
void handleDebugConsoleInterrupt(int8_t);
*/
void (*debugConsoleIsr)(int8_t);

/** Function pointer for the ISR called when the button is pressed.
Parameter is which button was pressed. */
void (*buttonIsr)(int8_t);

/**
Blocking Delay in Milliseconds - delays by at least the specified number of milliseconds (mSec)
@pre SysCtlClockSet() has been called to set the processor clock rate
@param ms number of milliseconds to delay
@note since the processor has other tasks, don't use this very often, if ever!
*/
void delayMs(uint16_t ms)
{
	SysCtlDelay((SysCtlClockGet() / 3000) * ms);   //should be 1000 * 3 (since each SysCtlClockGet() call is 3 clock cycles)
}
/** Initializes Oscillator: turns off WDT, configures MCLK to 25MHz using internal PLL */
void oscInit()
{
    // Set the clocking to run at 25MHz from the PLL calibrated to an 8MHz external crystal.
    // PLL native frequency = 200MHz, so divide by 8 to get 25MHz
    //SysCtlClockSet(SYSCTL_SYSDIV_8 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ);
    // Divide by 4 to get 50MHz:
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ);
}


void portInit2()
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlDelay(10); //delay by (3 * 10) 30 clock cycles
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlDelay(10); //delay by (3 * 10) 30 clock cycles

    //IntMasterEnable(); //Temporarily put here

    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 9600,
    //                    (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
    //                     UART_CONFIG_PAR_NONE));


    // If interrupts are desired, then enable interrupts on this UART
    IntEnable(INT_UART0);
    UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
    UARTStdioInit(0);  //configures this UART0 as the uart to use for UARTprintf


    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlDelay(10); //delay by (3 * 10) 30 clock cycles
    /* Outputs */
    GPIOPadConfigSet(GPIO_PORTB_BASE, (GPIO_PIN_0 | GPIO_PIN_1), GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD);
    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, (GPIO_PIN_0 | GPIO_PIN_1));  //configure LEDs

    IntMasterEnable();



}


/** Initializes Ports/Pins: sets direction, interrupts, pullup/pulldown resistors etc. */
void portInit()
{
    //Configure SPI port
    //PRECONDITIONS: GPIO PORTS HAVE BEEN ENABLED & PINS WERE CONFIGURED: PC4, PD5 as outputs; PD6 as input
    //SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);  //FOR Module Control ????
    //SysCtlDelay(10); //delay by (3 * 10) 30 clock cycles

    /* Port A
     * PA0      U0Rx (Only for USB UART - N/A if using RS-232)
     * PA1      U0Tx (Only for USB UART - N/A if using RS-232)
     * PA2      SSI0CLK
     * PA3      SSI0 ChipSelect for OLED screen on LM3S6965 board (NOT USED in our application - leave as input in case R25 mistakenly populated)
     * PA4      SSI0Rx (MISO)
     * PA5      SSI0Tx (MOSI)
     * PA6      USB Sleep input
     * PA7
     */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlDelay(10); //delay by (3 * 10) 30 clock cycles
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlDelay(10); //delay by (3 * 10) 30 clock cycles
    /* SSI */
    GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_5);
    GPIOPadConfigSet(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_5, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);
    SysCtlDelay(10); //delay by (3 * 10) 30 clock cycles
    /* Inputs */
    GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, GPIO_PIN_6);
    GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, GPIO_PIN_3);
    /* UART */
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    // If interrupts are desired, then enable interrupts on this UART
    IntEnable(INT_UART0);
    UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
    UARTStdioInit(0);  //configures this UART0 as the uart to use for UARTprintf

    /* Port B
     * Pb0      Internal LED1 - "D2" Blue
     * Pb1      Internal LED2 - "D3" Green
     * Pb2		I2C SCL (connected to EEPROM)
     * Pb3		I2C SDA (connected to EEPROM)
     * Pb4      Spare 1 (on header)
     * Pb5      Status LED - Green
     * Pb6      Status LED - Red
     * Pb7      TRSTn - not used
     */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlDelay(10); //delay by (3 * 10) 30 clock cycles
    /* Outputs */
    GPIOPadConfigSet(GPIO_PORTB_BASE, (GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_5 | GPIO_PIN_6), GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD);
    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, (GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_5 | GPIO_PIN_6));  //configure LEDs

    // I2C:
#ifdef HAL_I2C
    GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_2 | GPIO_PIN_3);
    delayMs(2);
#endif

    /* Port C
     * PC0      JTAG TCK
     * PC1      JTAG TMS
     * PC2      JTAG TDI
     * PC3      JTAG TD0
     * PC4      Module Chip Select
     * PC5      SPI Flash Chip Select
     * PC6      USB RTS (not used)
     * PC7      USB CTS (not used)
     */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    SysCtlDelay(10); //delay by (3 * 10) 30 clock cycles
    /* Outputs */
    GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, (GPIO_PIN_4 | GPIO_PIN_5));
    //GPIOPadConfigSet(GPIO_PORTC_BASE, (GPIO_PIN_4 | GPIO_PIN_5), GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD);


    /* Port D
     * PD0      Spare 0
     * PD1
     * PD2      Spare Rx
     * PD3      Spare Tx
     * PD4
     * PD5      Module Reset
     * PD6      SRDY
     * PD7
     */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);  //FOR Module CONTROL
    SysCtlDelay(10); //delay by (3 * 10) 30 clock cycles
    /* Outputs */
    GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, GPIO_PIN_5);
    //GPIOPadConfigSet(GPIO_PORTD_BASE, GPIO_PIN_5, GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD);
    /* Inputs */
    GPIOPinTypeGPIOInput(GPIO_PORTD_BASE, GPIO_PIN_6);


    RADIO_OFF();
    SPI_SS_CLEAR();

    /* Port E
     * PE0      Spare
     * PE1      Spare
     * PE2      Spare
     * PE3      Internal button
     */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    SysCtlDelay(10); //delay by (3 * 10) 30 clock cycles
    /* Inputs */
    GPIOPadConfigSet(GPIO_PORTE_BASE, GPIO_PIN_3 , GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD_WPD);
    GPIODirModeSet(GPIO_PORTE_BASE, GPIO_PIN_3, GPIO_DIR_MODE_IN);
    GPIOIntTypeSet(GPIO_PORTE_BASE, GPIO_PIN_3, GPIO_RISING_EDGE); // Make button a falling-edge triggered interrupt
    GPIOPinIntEnable(GPIO_PORTE_BASE, GPIO_PIN_3);                  // Enable the interrupt on this pin
    GPIOPinIntClear(GPIO_PORTE_BASE, GPIO_PIN_3);                   //Clear interrupts
    IntEnable(INT_GPIOE);                                           //enable interrupt 18

    /* Port F
     * PF0      Internal LED0 - "D1" Red
     * PF1      External button
     * PF2		Ethernet LED1 (Rx or Tx Activity)
     * PF3		Ethernet LED0 (Link Ok)
     */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlDelay(10); //delay by (3 * 10) 30 clock cycles
    /* Outputs */
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_0);  //configure LED
    /* Inputs */
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_1 , GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD_WPD);
    GPIODirModeSet(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_DIR_MODE_IN);
    GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_RISING_EDGE); // Make button a falling-edge triggered interrupt
    GPIOPinIntEnable(GPIO_PORTF_BASE, GPIO_PIN_1);                  // Enable the interrupt on this pin
    GPIOPinIntClear(GPIO_PORTF_BASE, GPIO_PIN_1);                   //Clear interrupts
    IntEnable(INT_GPIOF);                                           //enable interrupt 18
    /* Ethernet LEDs */
    GPIODirModeSet(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3, GPIO_DIR_MODE_HW);
    GPIOPadConfigSet(GPIO_PORTF_BASE,GPIO_PIN_2|GPIO_PIN_3,GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD);

    return;

    /* Port G
     * PG0      UART2 Rx
     * PG1      UART2 Tx
     */
#ifdef USE_UART2
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART2);
    SysCtlDelay(10); //delay by (3 * 10) 30 clock cycles
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
    SysCtlDelay(10); //delay by (3 * 10) 30 clock cycles

    GPIOPinTypeUART(GPIO_PORTG_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    // Configure the UART for 115,200, 8-N-1 operation.
    //UARTConfigSetExpClk(UART2_BASE, SysCtlClockGet(), 115200,
    //		(UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
    // Enable the UART interrupt.
    IntEnable(INT_UART2);

    UARTIntEnable(UART2_BASE, UART_INT_RX | UART_INT_RT);
    //UARTStdioInit(2);  //configures this UART0 as the uart to use for UARTprintf
#else
    //SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    /* already done in port A
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // If interrupts are desired, then enable interrupts on this UART
    IntEnable(INT_UART0);
    UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
    UARTStdioInit(0);  //configures this UART0 as the uart to use for UARTprintf
	*/
#endif

}

/**
 *
@note It takes five clock cycles after SysCtlPeripheralEnable() is called to enable a peripheral before the the peripheral is
actually enabled. During this time, attempts to access the peripheral will result in a bus fault.
Care should be taken to ensure that the peripheral is not accessed during this brief time period.
 */
void halInit()
{
	oscInit();
	portInit();
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ETH);
	SysCtlPeripheralReset(SYSCTL_PERIPH_ETH);

    //Point the function pointers to doNothing() so that they don't trigger a restart
    //debugConsoleIsr = &doNothing;
    buttonIsr = &doNothing;
}


/** Port E interrupt service routine
@note Must be configured in startup_ccs.c or else will not be called.
*/
void IntGPIOe(void)
{
    buttonIsr(0);   // Internal button - Only Interrupt source for this port is PE3
    GPIOPinIntClear(GPIO_PORTE_BASE, GPIO_PIN_3);                   //Clear interrupts
    toggleLed(2);
}


/** Port F interrupt service routine
@note Must be configured in startup_ccs.c or else will not be called.
*/
void IntGPIOf(void)
{
    buttonIsr(1);   // External button - Only Interrupt source for this port is PF1
    GPIOPinIntClear(GPIO_PORTF_BASE, GPIO_PIN_1);                   //Clear interrupts
    toggleLed(2);
}


/** Simple placeholder function to point the function pointers to so that they don't cause mischief */
void doNothing(int8_t a)
{
    SysCtlDelay(10); //delay by (3 * 10) 30 clock cycles
}


#define BUTTON_EXT_PRESSED	(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_1))
#define BUTTON_INT_PRESSED	(GPIOPinRead(GPIO_PORTE_BASE, GPIO_PIN_3))
/*
Whether the selected button is pressed.
@param button which button - must be ANY_BUTTON or BUTTON_INTERNAL or BUTTON_EXTERNAL on this implementation.
@return > 0 if the selected button is pressed, otherwise 0.
@note buttons are active LOW
*/
uint8_t buttonIsPressed(const uint8_t button)
{
    if (button == ANY_BUTTON)
    	return (BUTTON_EXT_PRESSED || BUTTON_INT_PRESSED);

    if (button == BUTTON_EXTERNAL)
    	return (BUTTON_EXT_PRESSED);

    if (button == BUTTON_INTERNAL)
    	return (BUTTON_INT_PRESSED);

    // invalid buttonId, so return false
    return 0;
}

/** Sets module IOs to be INPUTs so an external programmer can communicate with the module
 * PC4 = Chip Select
 * PD5 = MRST
 * PA2,4,5 = SPI Signals
 * */
void halDisableModuleInterface()
{
    printf("NOTE: DISABLING MODULE INTERFACE\r\nRESET GATEWAY TO RESTORE\r\n");

    GPIOPinTypeGPIOInput(GPIO_PORTC_BASE, GPIO_PIN_4);
    GPIOPinTypeGPIOInput(GPIO_PORTD_BASE, GPIO_PIN_5);
    GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, (GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_5));
}

/**
 Debug console interrupt service routine, called when a byte is received on UART.
 @pre In startup_ccs.c, UARTIntHandler is set as the ISR for the UART0 Rx and Tx interrupt
 @pre In startup_ccs.c, this function is declared, e.g:
 <pre>
 extern void UARTIntHandler(void);
 </pre>
 @pre Interrupts have been enabled, e.g:
 <pre>
 IntMasterEnable();
 IntEnable(INT_UART0);
 UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
 </pre>
 @pre UART has been configured
 @post a byte received on the UART will result in debugConsoleIsr() function pointer called
 */
void UARTIntHandler(void)
{
	toggleLed(4);
	//printf("$");
    unsigned long ulStatus;

    // Get the interrupt status.
    ulStatus = UARTIntStatus(UART0_BASE, true);

    // Clear the asserted interrupts.
    UARTIntClear(UART0_BASE, ulStatus);

    // Loop while there are characters in the receive FIFO.
    while(UARTCharsAvail(UART0_BASE))
    {
        // Read the next character from the UART and write it back to the UART.
    	uint8_t c = (uint8_t) UARTCharGetNonBlocking(UART0_BASE);
        debugConsoleIsr(c);    //reading this register clears the interrupt flag
    }
}

#define NUM_LEDS 5

/* LEDS:
 0  Status - Green (PB5)
 1  Status - Red (PB6)
 2  Internal LED0 (PF0)
 3  Internal LED1 (PB0)
 4  Internal LED2 (PB1)
 */

/**
Toggles the specified LED.
@param led the LED to toggle
@post The specified LED is toggled.
@return 0 if success, -1 if invalid LED specified
*/
int16_t toggleLed(uint8_t led)
{
	if (led >= NUM_LEDS)
		return -1;
	switch(led)
	{
	case 0:
	    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_5,~GPIOPinRead(GPIO_PORTB_BASE, GPIO_PIN_5));
	    return 0;
	case 1:
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6,~GPIOPinRead(GPIO_PORTB_BASE, GPIO_PIN_6));
        return 0;
    case 2:
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0,~GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0));
        return 0;
    case 3:
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0,~GPIOPinRead(GPIO_PORTB_BASE, GPIO_PIN_0));
        return 0;
    case 4:
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_1,~GPIOPinRead(GPIO_PORTB_BASE, GPIO_PIN_1));
        return 0;
    default: //redundant
        return -1;
	}
}

void softwareControlEthernetLeds()
{
    GPIODirModeSet(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3, GPIO_DIR_MODE_OUT);
}

void hardwareControlEthernetLeds()
{
    GPIODirModeSet(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3, GPIO_DIR_MODE_HW);
}

void setEthernetLeds()
{
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3, 0);
}

void clearEthernetLeds()
{
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3, GPIO_PIN_2 | GPIO_PIN_3);
}

/**
Turns ON the specified LED.
@param led the LED to turn on
@post The specified LED is turned on.
@return 0 if success, -1 if invalid LED specified
*/
int16_t setLed(uint8_t led)
{
	if (led >= NUM_LEDS)
		return -1;
    switch(led)
    {
    case 0:
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_5, GPIO_PIN_5);
        return 0;
    case 1:
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, GPIO_PIN_6);
        return 0;
    case 2:
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_PIN_0);
        return 0;
    case 3:
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0, GPIO_PIN_0);
        return 0;
    case 4:
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_1, GPIO_PIN_1);
        return 0;
    default: //redundant
        return -1;
    }
}

/**
Turns OFF the specified LED.
@param led the LED to turn off
@post The specified LED is turned off.
@return 0 if success, -1 if invalid LED specified
*/
int16_t clearLed(uint8_t led)
{
	if (led >= NUM_LEDS)
		return -1;
    switch(led)
    {
    case 0:
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_5, 0);
        return 0;
    case 1:
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, 0);
        return 0;
    case 2:
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, 0);
        return 0;
    case 3:
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0, 0);
        return 0;
    case 4:
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_1, 0);
        return 0;
    default: //redundant
        return -1;
    }
}

/**
Turns OFF LEDs.
@post LEDs are turned off.
 */
void clearLeds()
{

    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_5, 0);
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, 0);
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, 0);
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0, 0);
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_1, 0);
}

/**
Initializes the Serial Peripheral Interface (SPI) interface to the Zigbee Module (ZM).
@note Maximum module SPI clock speed is 4MHz. SPI port configured for clock polarity of 0, clock phase of 0, and MSB first.
@note On the GW the Stellaris uses SSI0 to communicate with module
@pre SPI pins configured correctly:
- Clock, MOSI, MISO configured as SPI function
- Chip Select configured as an output
- SRDY configured as an input.
@post SPI port is configured for communications with the module.
*/
void halSpiInitModule()
{
    SSIDisable(SSI0_BASE);

    // Reconfigure the SSI Port for Module operation.
    // Clock polarity = inactive is LOW (CPOL=0); Clock Phase = 0; MSB first; Master Mode, 2mHz, data is 8bits wide;
    SSIConfigSetExpClk(SSI0_BASE, SysCtlClockGet(), SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER, 1000000, 8);

    // Enable the SSI Port.
    SSIEnable(SSI0_BASE);

    // Hold the module in reset
    SPI_SS_CLEAR();
}


/**
Sends a message over SPI to the Module on USCI_B1.
SPI uses a "write-to-read" approach to read data out, you must write data in.
This is a private method that gets wrapped by other methods, e.g. spiSreq(), spiPoll(), etc.
To Write, set *bytes, numBytes.
To Read, set *bytes only. Don't need to set numBytes because Module will stop when no more bytes read.
@param bytes the data to be sent or received.
@param numBytes the number of bytes to be sent. This same buffer will be overwritten with the received data.
@note Modify this method for other hardware implementations.
@pre SPI port configured for writing
@pre Module has been initialized
@post bytes contains received data, if any
@see Stellaris Application Note spma002.pdf, "Adding 32kB of Serial SRAM to a Stellaris Microcontroller"
*/
void spiWrite(unsigned char *bytes, unsigned char numBytes)
{
    unsigned long ulReadData;
	//SPI_SS_SET();  //assert CS
	//while (SRDY_IS_HIGH()) ;
	while(numBytes--)    // Loop while there are more bytes left to be transferred.
    {
        SSIDataPut(SSI0_BASE, *bytes);              // Write the next byte to the SSI controller with a blocking put.
        SSIDataGet(SSI0_BASE, &ulReadData);         // Read into a long first
        *bytes++ = (unsigned char)ulReadData;       // ...and then convert it to a char
    }

    //while ( (~HWREG(SSI0_BASE + SSI_O_SR)) & SSI_SR_TFE) ;   //wait while Transmit FIFO is NOT empty
	//while ( (HWREG(SSI0_BASE + SSI_O_SR)) & SSI_SR_BSY) ;   //wait while busy   - experimental; works without this!
    //SPI_SS_CLEAR();
}

/**
 * Turn off interrupt sources that may interrupt us (SysTick and Ethernet) and then switch control
 * to the Boot Loader. This will never return!
 */
void halSwitchToBootloader()
{
    while(UARTCharsAvail(UART0_BASE))
    {
        UARTCharGet(UART0_BASE);
    }

    EthernetIntDisable(ETH_BASE, 0xFFFF);
    SysTickIntDisable();
    IntDisable(INT_UART0);
    UARTIntDisable(UART0_BASE, UART_INT_RX | UART_INT_RT);
    UARTIntClear(UART0_BASE, UART_INT_RX | UART_INT_RT);


    delayMs(100);
    // Call the boot loader so that it will listen for an update on the UART.
    (*((void (*)(void))(*(unsigned long *)0x2c)))();

    //
    // The boot loader should take control, so this should never be reached.
    // Just in case, loop forever.
    //
    while(1)
    {
    }

}

/**
 * @return 0 if no bootloader, or 1 if bootloader is indeed present.
 */
uint32_t halCheckForBootloader()
{
    unsigned long *pulApp;

#define BOOTLOADER_START_ADDRESS 0x0000

    //
    // See if the first location is 0xfffffffff or something that does not
    // look like a stack pointer, or if the second location is 0xffffffff or
    // something that does not look like a reset vector.
    //
    pulApp = (unsigned long *)BOOTLOADER_START_ADDRESS;
    if((pulApp[0] == 0xffffffff) || ((pulApp[0] & 0xfff00000) != 0x20000000) ||
       (pulApp[1] == 0xffffffff) || ((pulApp[1] & 0xfff00001) != 0x00000001))
    {
        return(0);  // No bootloader present
    } else {
    	return 1; 	// Bootloader is present
    }
}

uint32_t halCheckForApplication()
{
    unsigned long *pulApp;

#define APPLICATION_START_ADDRESS 0x1000		// This must match bootloader stuff!

    //
    // See if the first location is 0xfffffffff or something that does not
    // look like a stack pointer, or if the second location is 0xffffffff or
    // something that does not look like a reset vector.
    //
    pulApp = (unsigned long *)APPLICATION_START_ADDRESS;
    if((pulApp[0] == 0xffffffff) || ((pulApp[0] & 0xfff00000) != 0x20000000) ||
       (pulApp[1] == 0xffffffff) || ((pulApp[1] & 0xfff00001) != 0x00000001))
    {
        return(0);  // No bootloader present
    } else {
    	return 1; 	// Bootloader is present
    }
}
