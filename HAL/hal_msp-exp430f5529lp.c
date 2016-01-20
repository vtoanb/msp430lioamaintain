/**
* @ingroup hal
* @{ 
* @file hal_launchpad.c
*
* @brief Hardware Abstraction Layer (HAL) for the TI MSP-EXP430F5529LP and Anaren Zigbee BoosterPack
*
* This file must be modified if changing hardware platforms. Based on hal_mdb2.c
*
* The Zigbee Module library & examples require several methods to be defined. 
* See hal_helper documentation. Also see hal_launchpad.h for macros that must be defined.
*
* @see hal_helper.c for utilities to assist when changing hardware platforms
*
Clocking:
SMCLK: 4MHz (see oscInit()), sources the following:
- UART - see halUartInit()
- SPI - see halSpiInitModule()
- PWM for RGB LEDs
ACLK: Sourced by XT1, 32.768KHz
- Timer - see initTimer()

* $Rev: 1911 $
* $Author: dsmith $
* $Date: 2013-09-23 13:53:16 -0700 (Mon, 23 Sep 2013) $
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

#include "hal_msp-exp430f5529lp.h"
#include "hal_version.h"
#include <stdint.h>

/** 
This is a function pointer for the Interrupt Service Routine called when a debug console character is received.
To use it, declare it with
<code> extern void (*debugConsoleIsr)(char);  </code> 
and then point it to a function you created, e.g.
<code> debugConsoleIsr = &handleDebugConsoleInterrupt;  </code>
and your function handleDebugConsoleInterrupt() will be called when a byte is received on the debug serial port.
*/
void (*debugConsoleIsr)(int8_t);

/** 
Function pointer for the ISR called when the button is pressed. 
Parameter is which button was pressed. */
void (*buttonIsr)(int8_t);

/** Function pointer for the ISR called when a timer generates an interrupt */
void (*timerIsr)(void);

/** Function pointer for the ISR called when a SRDY interrupt occurs */
void (*srdyIsr)(void);

/** Function pointer for the ISR called when a sysTick interrupt occurs */
void (*sysTickIsr)(void);


/** 
Flags to indicate when to wake up the processor. These are read in the various ISRs. 
If the flag is set then the processor will be woken up with HAL_WAKEUP() at the end of the ISR. 
This is required because HAL_WAKEUP() cannot be called anywhere except in an ISR. */
uint16_t wakeupFlags = 0;


/** Declaration so we can use this inside hal_launchpad.c for debugging */
int putchar(int c);


/** Debug console interrupt service routine, called when a byte is received on USCIB0. */
#pragma vector = USCI_A1_VECTOR
__interrupt void USCIA1RX_ISR(void)
{
#ifdef DEBUG_USCIA1RX_ISR
    putchar('@');
    while (1)
    {
        toggleLed(1);
        delayMs(100);
    }
#endif
    if (UCRXIFG & UCA1IFG)               //debug console character received
    {
        debugConsoleIsr(UCA1RXBUF);    //reading this register clears the interrupt flag
    }
}


/** Port 1 interrupt service routine, called when an interrupt-enabled pin on port 1 changes state. */
#pragma vector=PORT1_VECTOR
__interrupt void PORT1_ISR(void)
{
    if (P1IFG & BIT3)                   //Modify this based on which pin is connected to Button
    {
        buttonIsr(0);   // Button 0 was pressed
        if (wakeupFlags & WAKEUP_AFTER_BUTTON)    
            HAL_WAKEUP();
    }
    P1IFG = 0;                          // clear the interrupt
}


/** Port 2 interrupt service routine, called when an interrupt-enabled pin on port 2 changes state. */
#pragma vector=PORT2_VECTOR
__interrupt void PORT2_ISR(void)
{
    if (P2IFG & BIT2)                   //Modify this based on which pin is connected to SRDY
    {
        srdyIsr();
        if (wakeupFlags & WAKEUP_AFTER_SRDY)    
            HAL_WAKEUP();          
    }
    P2IFG = 0;                          // clear the interrupt
}

/**
Whether the selected button is pressed.
@param button which button - must be ANY_BUTTON or BUTTON_0 on this implementation.
@return >0 if the selected button is pressed, otherwise 0.
*/
uint8_t buttonIsPressed(uint8_t button)
{
    if ((button == ANY_BUTTON) || (button == BUTTON_0))
        return (~P1IN & BIT3);

    return 0;
}

/** Simple placeholder to point the function pointers to so that they don't cause mischief */
static void doNothing(int8_t a)
{
    volatile int8_t b = a;  //prevent this from getting optimized out
}

/**
 * Code from TI to set the core power level
 */
static void
SetVcoreUp( uint16_t level )
{
    // Open PMM registers for write
    PMMCTL0_H = PMMPW_H;
    // Set SVS/SVM high side new level
    SVSMHCTL = SVMHFP + SVMHE + SVSHFP + SVSHE + SVSHRVL0 * level + SVSMHRRL0 * level;
    // Set SVM low side to new level
    SVSMLCTL = SVMLFP + SVSLE + SVSLFP + SVMLE + SVSMLRRL0 * level;
    // Wait till SVM is settled
    while ((PMMIFG & SVSMLDLYIFG) == 0);
    // Clear already set flags
    PMMIFG &= ~(SVMLVLRIFG + SVMLIFG);
    // Set VCore to new level
    PMMCTL0_L = PMMCOREV0 * level;
    // Wait till new level reached
    if ((PMMIFG & SVMLIFG))
    while ((PMMIFG & SVMLVLRIFG) == 0);
    // Set SVS/SVM low side to new level
    SVSMLCTL = SVMLFP + SVSLE + SVSLFP + SVMLE + SVSLRVL0 * level + SVSMLRRL0 * level;
    // Lock PMM registers for write access
    PMMCTL0_H = 0x00;

    return;
}

/**
 * Initializes Oscillator: turns off WDT, configures MCLK to 8MHz using internal DCO and sets SMCLK to 4MHz
 * MSP-EXP430F5529 has 4MHz Crystal on XT2 and 32.768KHz Crystal on XT1
 */
void oscInit()
{
    WDTCTL = WDTPW + WDTHOLD; // Stop WDT

    // Increase Vcore setting to level3 to support fsystem=25MHz
    // NOTE: Change core voltage one level at a time..
    SetVcoreUp( 0x01 );
    SetVcoreUp( 0x02 );

    /* Make sure Crystal pins are seteup correctly */
    P5SEL |= (BIT5 | BIT4 | BIT3 | BIT2);


    /* Select DCO to operate between 2.5MHz and 54MHz
     * DCORSEL = 5
     * DISMOD  = 0 (Modulation Enabled)
     */
    UCSCTL1 = (DCORSEL_5);

    /* Setup FLL
     * FLLD = 0 (fDCOCLK/1)
     * FLLN = 243 (Multiplier bits) (32.768KHz * 243) = 7962.624MHz
     */
    UCSCTL2 = (FLLD__1 | 243);

    /* Use XT1CLK (32K XTAL) for reference FLL */
    UCSCTL3 = (SELREF__XT1CLK | FLLREFDIV__1);


    /* Configure XT2 for high frequency reference clock
     * Configure XT1 for low frequency reference clock
     * XT2DRIVE  = 00 (4MHz to 8MHz)
     * XT2BYPASS = 0  (Sourced Internally)
     * X2OFF     = 0  (XT2 is On)
     * XT1DRIVE  = 11 (Max drive, XT1 is in LowFrequency LF Mode)
     * XTS       = 0  (Low Frequency Mode, XCAP bits are enabled)
     * XT1BYPASS = 0  (XT1 is sourced internally)
     * XCAP      = 00 (2pF, 12pF already on board)
     * SMCLKOFF  = 0  (SMCLK is on)
     * XT1OFF    = 0  (XT1 is on)
     */
    UCSCTL6 = 0x00C0;

    /* Wait for XT1LF clock stability */
    UCSCTL7 &= XT1LFOFFG;
    while( (UCSCTL7 & XT1LFOFFG) != 0){ UCSCTL7 &= ~XT1LFOFFG; }

    /* Wait for XT2 clock stability */
    UCSCTL7 &= XT2OFFG;
    while( (UCSCTL7 & XT2OFFG) != 0){ UCSCTL7 &= ~XT2OFFG; }

    /* Wait for DCO clock stability */
    UCSCTL7 &= DCOFFG;
    while( (UCSCTL7 & DCOFFG) != 0){ UCSCTL7 &= ~DCOFFG; }

    /* Select the XT2 as the clock source for SMCLK, Select the
     * lowFreq XT1 as the source for ACLK
     * FLL/DCO as the source for MCLK
     */
    UCSCTL4 = (SELA__XT1CLK) | /* XT1 -> ACLK */
              (SELS__DCOCLK) | /* DCO -> SMCLK */
              (SELM__DCOCLK);  /* DCO -> MCLK */

    /* Select clock dividers for MCLK, SMCLK and ACLK */
    UCSCTL5 = (DIVA__1) | /* ACLK = 32768 Hz   */
              (DIVS__2) | /* SMCLK = ~4MHz MHz */
              (DIVM__1);  /* MCLK =  ~8MHz MHz */

    return;
}

/** Initializes Ports/Pins: sets direction, interrupts, pullup/pulldown resistors etc. */
void portInit()
{
    volatile uint8_t unusedMask;
    //
    //    Initialize Ports. Unused ports are set output low
    //
    /*PORT 1:
    1.0
    1.1
    1.2
    1.3
    1.4
    1.5
    1.6    MODULE Push Button S2
    1.7
    */
    unusedMask = (BIT7|BIT5|BIT4|BIT3|BIT2|BIT1|BIT0);
    P1DIR = unusedMask;
    P1OUT = (P1OUT & ~unusedMask);
    /* Switch is falling edge */
    P1IES |= BIT6;
    

    /*PORT 2
    2.0    MODULE LED6 (RGB-Blue)
    2.1
    2.2    MODULE RST
    2.3    MODULE I2C-CLK
    2.4
    2.5
    2.6    MODULE LED5 (RGB-Green)
    2.7    MODULE SPI-SSEL
    */
    unusedMask = (BIT5|BIT4|BIT1);
    P2DIR = unusedMask;
    P2OUT = (P2OUT & ~unusedMask);
    /* LEDs are output Initialized to OFF */
    P2DIR |= (BIT6|BIT0);
    P2OUT &= ~(BIT6|BIT0);
    /* MODULE RST: Hold the module in reset (output-low) until driver releases */
    P2DIR |= (BIT2);
    P2OUT &= ~(BIT2);
    /* MODULE I2C-CLK: I2C interface is bit banged. Keep clk output high until needed */
    P2DIR |= (BIT3);
    P2OUT |= (BIT3);
    /* MODULE SPI-SEL: Initialize such that the module is not selected */
    P2DIR |= (BIT7);
    P2OUT |= (BIT7);


    /*PORT 3
    3.0    MODULE SPI-MOSI
    3.1    MODULE SPI-MISO
    3.2    MODULE SPI-SCLK
    3.3    MODULE TXD (Not Used)
    3.4    MODULE RXD (Not Used)
    3.5
    3.6
    3.7
    */
    unusedMask = (BIT7|BIT6|BIT5|BIT4|BIT3);
    P3DIR = unusedMask;
    P3OUT = (P3OUT & ~unusedMask);
    /* MODULE SPI-MOSI/MISO/SCLK: Set to Peripheral */
    P3DIR |= (BIT2|BIT0);
    P3DIR &= ~(BIT1);
    P3SEL |= (BIT2|BIT1|BIT0);


    /*PORT 4
    4.0
    4.1    MODULE SRDY
    4.2    MODULE LED4 (RGB-Red)
    4.3
    4.4    UART-Rx
    4.5    UART-Tx
    4.6
    4.7
    */
    unusedMask = (BIT7|BIT6|BIT3|BIT0);
    P4DIR = unusedMask;
    P4OUT = (P4OUT & ~unusedMask);
    /* MODULE SRDY: Input IO Rising-Edge */
    P4DIR &= ~(BIT1);
    /* MODULE LED4: Output Low (off) initialized */
    P4DIR |= (BIT2);
    P4OUT &= ~(BIT2);
    /* Configure UART pins to be UART */
    P4DIR &= ~(BIT4);
    P4DIR |= (BIT5);
    P4SEL |= (BIT5|BIT4);


    /*PORT 5
    5.0
    5.1
    5.2    XT2IN   (4MHz)
    5.3    XT2OUT
    5.4    XIN     (32KHz)
    5.5    XOUT
    5.6
    5.7
    */
    unusedMask = (BIT7|BIT6|BIT1|BIT0);
    P5DIR = unusedMask;
    P5OUT = (P5OUT & ~unusedMask);
    /* Configure XT2 XT1 IO Pins to select the crystals */
    P5SEL |= (BIT5 | BIT4 | BIT3 | BIT2);


    /*PORT 6
    6.0
    6.1
    6.2
    6.3
    6.4
    6.5    MODULE LED1
    6.6    MODULE Current Sensor
    6.7
    */
    unusedMask = (BIT7|BIT4|BIT3|BIT2|BIT1|BIT0);
    P6DIR = unusedMask;
    P6OUT = (P6OUT & ~unusedMask);
    /* MODULE LED1: Output Low (off) initialized */
    P6DIR |= (BIT5);
    P6OUT &= ~(BIT5);
    /* MODULE Current Sensor: Input Analog Comparator B s*/
    P6DIR &= ~(BIT6);
    P6SEL |= (BIT6);


    /*PORT 7
    7.0
    7.1
    7.2
    7.3
    7.4    MODULE Test (Not Used)
    7.5
    7.6
    7.7
    */
    unusedMask = (BIT7|BIT6|BIT5|BIT4|BIT3|BIT2|BIT1|BIT0);
    P7DIR = unusedMask;
    P7OUT = (P7OUT & ~unusedMask);


    /*PORT 8
    8.0
    8.1    MODULE I2C-Data
    8.2
    8.3
    8.4
    8.5
    8.6
    8.7
    */
    unusedMask = (BIT7|BIT6|BIT5|BIT4|BIT3|BIT2|BIT0);
    P8DIR = unusedMask;
    P8OUT = (P8OUT & ~unusedMask);
    /* MODULE I2C-CLK: I2C interface is bit banged. Keep clk output high until needed */
    P8DIR |= (BIT1);
    P8OUT |= (BIT1);

    return;
}


/** 
Initialize UART debug console for I/O. Configures the USCI in the processor to use UART mode. 
@post UART may be used by putchar() etc. 
@note LaunchPad debugger can only handle 9600 baud, no faster
*/
void halUartInit()
{
    UCA1CTL1 = UCSWRST;                         // Stop USCIA0 state machine
    UCA1CTL0 = 0;
    UCA1CTL1 |= UCSSEL_2;                       // USCIA0 source from SMCLK
    UCA1BR0 = 26; UCA0BR1 = 0;                  // 4mHz smclk w/modulation for 9,600bps, table 15-5
    UCA1MCTL = UCBRS_0 +UCBRF_1 + UCOS16;       // Modulation UCBRSx=1, over sampling
    UCA1CTL1 &= ~UCSWRST;                       // **Initialize USCI state machine**
    UCA1IE |= UCRXIE;                            // Enable USCI_A0 RX interrupt
}

/** Display information about this driver firmware */
void displayVersion()
{
    int i = 0;
    printf("\r\n\r\n");
    for (i=0; i<8; i++)
        printf("-");
    printf(" Module Interface and Examples %s ", MODULE_INTERFACE_STRING);
    for (i=0; i<8; i++)
            printf("-");
    printf("\r\n");
    printf("%s", MODULE_VERSION_STRING);
}

/** 
Configures hardware for the particular hardware platform:
- Ports: sets direction, interrupts, pullup/pulldown resistors etc. 
- Holds radio in reset (active-low)

@post the board is configured for proper operation.
*/
void halInit()
{
    oscInit();                      // Initialize oscillators
    portInit();                     // Configure GPIO ports
    halUartInit();                  // Initialize UART
    SPI_SS_CLEAR();                 // Deselect Module
    //TACTL = MC_0;                   //  Stop Timer A:

    //Point the function pointers to doNothing() so that they don't trigger a restart
    buttonIsr = &doNothing;
    //Note: during development you may want to comment out the line below. 
    //That way you can restart the micro by pressing any key in the terminal window.
    debugConsoleIsr = &doNothing;


#ifdef TESTING_SPI_UART_VERSION
    halSpiInitModule();
#endif
    clearLeds();
    displayVersion();    
}

/** Send one byte via hardware UART. Required for printf() etc. in stdio.h */
int putchar(int c)
{	
    while (!(UCTXIFG & UCA1IFG));   // Wait for ready
    UCA1TXBUF = (uint8_t) (c & 0xFF);
    return c;
}

/**
Initializes the Serial Peripheral Interface (SPI) interface to the Zigbee Module (ZM).
@note Maximum module SPI clock speed is 4MHz. SPI port configured for clock polarity of 0, clock phase of 0, and MSB first.
@note On the MDB2 the MSP430 uses USCIB0 SPI port to communicate with the module.
@pre SPI pins configured correctly: 
- Clock, MOSI, MISO configured as SPI function
- Chip Select configured as an output
- SRDY configured as an input.
@post SPI port is configured for communications with the module.
@note this function is not required if using the UART to communicate with the module
*/
void halSpiInitModule()
{
    UCB0CTL1 |= UCSSEL_2 | UCSWRST;                 //serial clock source = SMCLK, hold SPI interface in reset
    UCB0CTL0 = UCCKPH | UCMSB | UCMST | UCSYNC;     //clock polarity = inactive is LOW (CPOL=0); Clock Phase = 0; MSB first; Master Mode; Synchronous Mode    
    UCB0BR0 = 2;  UCB0BR1 = 0;                      //SPI running at 2MHz (SMCLK / 2)
    UCB0CTL1 &= ~UCSWRST;                           //start USCI_B1 state machine  
}

/**
Sends a message over SPI to the Module.
The Module uses a "write-to-read" approach: to read data out, you must write data in.
This is a private function that gets wrapped by other methods, e.g. spiSreq(), spiAreq, etc.
To Write, set *bytes and numBytes. To Read, set *bytes only. Don't need to set numBytes because the 
Module will stop when no more bytes are received.
@param bytes the data to be sent or received.
@param numBytes the number of bytes to be sent. This same buffer will be overwritten with the received data.
@pre SPI port configured for the Module and Module has been initialized properly
@post bytes contains received data, if any
@see zm_phy_spi.c
@note this function is not required if using the UART to communicate with the module
*/
void spiWrite(uint8_t *bytes, uint8_t numBytes)
{
    while (numBytes--)
    {  
        UCB0TXBUF = *bytes;
        while (!(UCRXIFG & UCB0IFG));     //WAIT for a character to be received, if any
        *bytes++ = UCB0RXBUF;             //read bytes
    }
}

/** 
A fairly accurate blocking delay for waits in the millisecond range. Good for 1mSec to 1000mSec. 
@note At 1MHz, error of zero for 100mSec or 1000mSec. For 10mSec, error of 100uSec. At 1mSec, error is 20uSec.
@note At 8MHz, error of zero for 1000mSec.
@note Accuracy will depend on the clock source. MSP430F2xx internal DCO is typically +/-1%. 
For better timing accuracy, use a timer, or a crystal.
@pre TICKS_PER_MS is defined correctly for the clock setting used.
@param delay number of milliseconds to delay
*/
void delayMs(uint16_t delay)
{
    while (delay--)
    {
        __delay_cycles(TICKS_PER_MS);
    }
}

/** 
Turns ON the specified LED. 
@param led the LED to turn on
@post The specified LED is turned on. 
@return 0 if success, -1 if invalid LED specified
*/
int16_t setLed(uint8_t led)
{
    switch (led)
    {
    case 0:
        P1OUT |= BIT0;
        return 0;
    case 1:
        P4OUT &= ~BIT2;      // RGB Red
        return 0;
    case 2:
        P2OUT &= ~BIT6;     // RGB Green
        return 0;
    case 3:
        P2OUT &= ~BIT0;      // RGB Blue
        return 0;
    default:
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
    switch (led)
    {
    case 0:
        P1OUT &= ~BIT0;
        return 0;
    case 1:
        P4OUT |= BIT2;      // RGB Red
        return 0;
    case 2:
        P2OUT |= BIT6;     // RGB Green
        return 0;
    case 3:
        P2OUT |= BIT0;      // RGB Blue
        return 0;
    default:
        return -1;
    }
}

/** 
Turns OFF LEDs. 
@post LEDs are turned off. 
*/
void clearLeds()
{ 
    P1OUT &= ~BIT0;
    P2OUT |= (BIT0 | BIT6);
    P4OUT |= (BIT2);
}

/** 
Toggles the specified LED. 
@param led the LED to toggle
@post The specified LED is toggled. 
@return 0 if success, -1 if invalid LED specified
*/
int16_t toggleLed(uint8_t led)
{
    switch (led)
    {
    case 0:
        P1OUT ^= BIT0;
        return 0;
    case 1:
        P4OUT ^= BIT2;      // RGB Red
        return 0;
    case 2:
        P2OUT ^= BIT6;     // RGB Green
        return 0;
    case 3:
        P2OUT ^= BIT0;      // RGB Blue
        return 0;
    default:
        return -1;
    }
}

//
//
//          LAUNCHPAD PERIPHERALS
//          NOT REQUIRED FOR MODULE OPERATION
//
//

/** 
Reads the MSP430 supply voltage using the Analog to Digital Converter (ADC) and internal voltage reference. 
ADC clocked by ADC12OSC
@return Vcc supply voltage, in millivolts
*/
uint16_t getVcc3()
{
    ADC12CTL0 = ADC12REFON + ADC12REF2_5V + ADC12ON + ADC12SHT0_3;  // use internal ref, turn on 2.5V ref, set samp time = 64 cycles
    ADC12CTL1 = ADC12SHP;
    ADC12MCTL0 = ADC12SREF_1 + ADC12INCH_11;
    delayMs(1);                                     // Allow internal reference to stabilize
    ADC12CTL0 |= ADC12ENC + ADC12SC;                     // Enable conversions
    while (!(ADC12IFG0 & ADC12IFG));                // Conversion done?
    // Note: result is now ready in ADC12MEM

#define REF2_5V_VOLTAGE_MV    2500l
#define VCC_DIVIDED_BY_2        2
#define ADC_TO_MV_MULTIPLIER    (REF2_5V_VOLTAGE_MV * VCC_DIVIDED_BY_2)
    unsigned long temp = (*ADC12MEM * ADC_TO_MV_MULTIPLIER);        // Convert raw ADC value to millivolts
    return ((uint16_t) (temp / 4096l));
}

/** 
Reads the current sense amplifier output using the Analog to Digital Converter (ADC). 
Current sense resistor is 0.2 ohms, amplifier multiplies voltage drop by 50. Therefore, every mA of
current translates into 10mV measured by MSP430.
ADC clocked by ADC12OSC
@note Current sense input is on P6.6/A6 on MSP430F5529
@pre Current sense shunt is installed on the BoosterPack
@return Current in mA multiplied by 10 (e.g. return value of 713 = 71.3mA)
*/
uint16_t getCurrentSensor()
{
    ADC12CTL0 = ADC12REFON + ADC12REF2_5V + ADC12ON + ADC12SHT0_3;  // use internal ref, turn on 2.5V ref, set samp time = 64 cycles
    ADC12CTL1 = ADC12SHP;
    ADC12MCTL0 = ADC12SREF_1 + ADC12INCH_6;
    delayMs(1);                                     // Allow internal reference to stabilize
    ADC12CTL0 |= ADC12ENC + ADC12SC;                     // Enable conversions
    while (!(ADC12IFG0 & ADC12IFG));                // Conversion done?
    // Note: result is now ready in ADC12MEM
    uint32_t temp = (*ADC12MEM * 1500l);        // Convert raw ADC value to milliamperes
    return ((uint16_t) (temp / 4096l));
}

/** 
Configures all module interface signals as inputs to allow the module to be programmed.
Toggles LED0 quickly to indicate application is running. 
*/
void halSetAllPinsToInputs(void)
{
    WDTCTL = WDTPW + WDTHOLD; // Stop WDT
    P1DIR = 1; P1REN = 0; P1IE = 0; P1SEL = 0; 
    P2DIR = 0; P2REN = 0; P2IE = 0; P2SEL = 0; 
    P3DIR = 0; P3REN = 0; P3SEL = 0;
    P4DIR = 0; P4REN = 0; P4SEL = 0;
    P5DIR = 0; P5REN = 0; P5SEL = 0;
    P6DIR = 0; P6REN = 0; P6SEL = 0;
    P7DIR = 0; P7REN = 0; P7SEL = 0;
    P8DIR = 0; P8REN = 0; P8SEL = 0;
    for (;;)
    {
        toggleLed(0);
        delayMs(100);   
    }
}

/** Configures which events will wake up the processor if sleeping. */
void halSetWakeupFlags(uint16_t wakeupFlagsToSet)
{
    wakeupFlags |= wakeupFlagsToSet;  
}

/** No flags will wakeup the processor while it is sleeping. */
void halClearWakeupFlags(uint16_t wakeupFlagsToClear)
{
    wakeupFlags &= ~wakeupFlagsToClear;  
}

/** The longest delay allowed by the timer, in seconds. */
#define TIMER_MAX_SECONDS 4

/** 
Configures timer.
@pre ACLK sourced from XT1 (32.768KHz)
@param seconds period of the timer.
@note Cannot use initTimer() and RGB PWM functions at the same time since they both use the same timer module.
@note Prescalar is used to reduce source clock to 8.192KHZ which allows for a max timeout of 8s.
@note Maximum prescaling of Timer A is divide by 8. Even longer times can be obtained by prescaling ACLK 
if this doesn't affect other system peripherals.
@return 0 if success; -1 if illegal parameter
*/
int16_t initTimer(uint8_t seconds)
{  
    if ((seconds > TIMER_MAX_SECONDS) || (seconds == 0))
        return -1;

    wakeupFlags |= WAKEUP_AFTER_TIMER;
    TA1CCTL0 = CCIE;
    TA1CCR0 = (32768 / 4) * (seconds);
    TA1CTL = TASSEL_1 + ID_2 + MC_1;
    return 0;
}

/** Halts the timer. Leaves period unchanged. */
void stopTimer()
{
    TA1CTL = MC_0;
}

//
//  System Tick (sysTick)
//

/** 
Initializes the systick interval timer. Default Interval is approximately 8mSec and is not
adjustable because we are using the watchdog timer which has fixed timeout periods.
@note we are using the Watchdog Timer module to be a timer, not a classic watchdog. This is because 
we need the systick function at the same time as the PWM. The PWM uses all the normal timer modules
on the microcontroller so we have to use the WDT module instead. If you aren't using PWM then we
recommend just using initTimer() instead.
@note WDT sourced from SMCLK
*/
void initSysTick(void)
{
  WDTCTL = WDT_MDLY_32;                     // Set Watchdog Timer interval, source from SMCLK (4MHz), 8Msec
  SFRIE1 |= WDTIE;                             // Enable WDT interrupt
}

// Watchdog Timer interrupt service routine
#pragma vector=WDT_VECTOR
__interrupt void watchdog_timer(void)
{
  sysTickIsr();
}


/** Interrupt Service Routine called by Timer A1. May wakeup processor based on value of wakeupFlags. */
#pragma vector=  TIMER1_A0_VECTOR
__interrupt void Timer1_A0 (void)
{
    timerIsr();
    if (wakeupFlags & WAKEUP_AFTER_TIMER)    
    {
        HAL_WAKEUP();     
    }
}


/** 
Calibrate VLO.
 This function is stubbed out because the VLO is not used on the MSP430F5529. There is an onboard 32KHz
 Crystal that is used to drive the ACLK
*/
uint16_t calibrateVlo()
{
    return 32768;
}

/** Required for compatibility with processors (like Stellaris) that have a UART FIFO */
uint8_t halUartBusy()
{
	return (0);
}

//
//RGB LEDs:
// On the MSP-EXP430F5529 platform the RGB LEDs are not all connected to Timers that can act as
// PWM outputs so the RGB driver is stubbed out. Setting any intensity will turn the LED on full
// intensity. Setting 0 intensity will turn the LED off
//
#define OFF 0
#define ON  1
#define RGB_RED_CONTROL(_x)    {P4OUT = ((P4OUT & ~BIT2) | (BIT2 * (_x)));}
#define RGB_BLUE_CONTROL(_x)   {P2OUT = ((P2OUT & ~BIT0) | (BIT0 * (_x)));}
#define RGB_GREEN_CONTROL(_x)  {P2OUT = ((P2OUT & ~BIT6) | (BIT6 * (_x)));}

/**
Initializes the PWM engine used for the RGB LED. This allows the RGB LED to display many colors.
@post RGB LED may be used, with halRgbSetLeds().
@note Uses SMCLK and Timer modules TA1 and TA0.
*/
void halRgbLedPwmInit()
{
    halRgbSetLeds(0,0,0);                   // Initialization done, turn them off
}

/** 
Sets RGB LED color to the selected values. Adjusts intensities so that illuminance of each color is 
even. This way calling the function with (0xFF, 0xFF, 0xFF) will result in a fairly good white.
@param red the amount of red - 0 to 0xFF
@param blue the amount of blue - 0 to 0xFF
@param green the amount of green - 0 to 0xFF
@pre halRgbLedPwmInit() has been called to initialize the PWM engine.
@post RGB LED displays the selected colors.

*/
void halRgbSetLeds(uint8_t red, uint8_t blue, uint8_t green)
{
    if( red > 0 )  { RGB_RED_CONTROL(ON);   } else { RGB_RED_CONTROL(OFF);   }
    if( green > 0 ){ RGB_GREEN_CONTROL(ON); } else { RGB_GREEN_CONTROL(OFF); }
    if( blue > 0 ) { RGB_BLUE_CONTROL(ON);  } else { RGB_BLUE_CONTROL(OFF);  }
}

/** Simple test of RGB LED. */
void halRgbLedTest()
{
    int i;
    for (i=0; i< 0xFF; i++)
    {
        halRgbSetLeds(0xFF-i,0xFF,0xFF);
        delayMs(5);
    }
    for (i=0; i< 0xFF; i++)
    {
        halRgbSetLeds(i,0xFF,0xFF);
        delayMs(5);
    }
    
    for (i=0; i< 0xFF; i++)
    {
        halRgbSetLeds(0xFF,0xFF-i,0xFF);
        delayMs(5);
    }        
    for (i=0; i< 0xFF; i++)
    {
        halRgbSetLeds(0xFF,i,0xFF);
        delayMs(5);
    }
    
    for (i=0; i< 0xFF; i++)
    {
        halRgbSetLeds(0xFF,0xFF,0xFF-i);
        delayMs(5);
    }        
    for (i=0; i< 0xFF; i++)
    {
        halRgbSetLeds(0xFF,0xFF,i);
        delayMs(5);
    }
}

/* @} */

