/* 
 * File:   main.c
 * Author: Collins_m
 *
 * Created on July 5, 2025, 10:22 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <xc.h>
#include <string.h>

// Define oscillator frequency for delay macros
#define _XTAL_FREQ 64000000UL

// PIC18F46Q84 Configuration Bit Settings
#pragma config FEXTOSC = OFF     // External oscillator not enabled
#pragma config RSTOSC = HFINTOSC_64MHZ // Reset oscillator: HFINTOSC with HFFRQ=64MHz
#pragma config CLKOUTEN = OFF    // Clock Out Enable bit disabled
#pragma config MVECEN = OFF      // Multi-vector enable bit disabled
#pragma config IVT1WAY = ON      // IVTLOCK bit can be cleared and set only once
#pragma config CSWEN = ON        // Clock Switch Enable bit enabled
#pragma config FCMEN = ON        // Fail-Safe Clock Monitor Enable bit enabled

// CONFIG3L
#pragma config MCLRE = EXTMCLR   // MCLR pin enabled, RE3 input disabled
#pragma config PWRTS = PWRT_OFF // Power-up timer selection bits (PWRT is disabled)
#pragma config LPBOREN = OFF     // Low-Power BOR disabled
#pragma config BOREN = SBORDIS   // Brown-out Reset enabled, SBOREN disabled

// CONFIG3H
#pragma config BORV = VBOR_1P9  // Brown-out Reset Voltage Selection bits (Brown-out Reset Voltage (VBOR) set to 1.9V)
#pragma config ZCD = OFF         // Zero-cross detect disabled
#pragma config PPS1WAY = ON      // PPSLOCK bit can be cleared and set only once
#pragma config STVREN = ON       // Stack full/underflow will cause Reset
#pragma config XINST = OFF       // Extended Instruction Set disabled
#pragma config DEBUG = OFF

// CONFIG4L
#pragma config WDTCPS = WDTCPS_31// WDT Period Select bits (Divider ratio 1:65536)
#pragma config WDTE = OFF        // WDT disabled

// CONFIG4H
#pragma config WDTCWS = WDTCWS_7 // WDT Window Select bits (Window always open)
#pragma config WDTCCS = SC       // WDT input clock selector (Software Control)
// Pin Definitions
#define LCD_RS LATCbits.LATC0
#define LCD_EN LATCbits.LATC1
#define LCD_D0 LATCbits.LATC2
#define LCD_D1 LATCbits.LATC3
#define LCD_D2 LATCbits.LATC4
#define LCD_D3 LATCbits.LATC5
#define LCD_D4 LATCbits.LATC6
#define LCD_D5 LATCbits.LATC7
#define LCD_D6 LATDbits.LATD0
#define LCD_D7 LATDbits.LATD1
#define LED LATDbits.LATD2
#define PULSE1 LATDbits.LATD3
#define PULSE2 LATDbits.LATD4

// Keypad Definitions
#define ROW1 PORTBbits.RB0
#define ROW2 PORTBbits.RB1
#define ROW3 PORTBbits.RB2
#define ROW4 PORTBbits.RB3
#define COL1 LATBbits.LATB4
#define COL2 LATBbits.LATB5
#define COL3 LATBbits.LATB6
#define COL4 LATBbits.LATB7
#define COL5 LATBbits.LATB0

// Global Variables
unsigned long onPulseTime = 100;     // Default ON time (100us)
unsigned long offPulseTime = 100;    // Default OFF time (100us)
unsigned long timerDuration = 1000;  // Default timer duration (1000 cycles)
unsigned char activePulse = 0;       // Pulse generator state
unsigned long countCurrent = 0;      // Current cycle count
char displayBuffer[32];              // LCD display buffer

// Function Prototypes
void systemInit(void);
void lcdInit(void);
void lcdWriteCmd(unsigned char);
void lcdWriteData(unsigned char);
void lcdPrint(const char*);
char keypadScan(void);
void updateDisplay(void);
void handleKeypress(char key);

// Interrupt Service Routine
// Interrupt Service Routine
void __interrupt() isr(void) {
    if (PIR2 & (1 << 1)) {  // Check Timer2 interrupt flag
        if (activePulse) {
            static unsigned char state = 0;
            static unsigned long counter = 0;
        
            counter++;
            if (state == 0 && counter >= onPulseTime) {
                PULSE1 = 0;
                PULSE2 = 1;
                state = 1;
                counter = 0;
            } else if (state == 1 && counter >= offPulseTime) {
                PULSE1 = 1;
                PULSE2 = 0;
                state = 0;
                counter = 0;
            }
            
            if (countCurrent > 0) {
                countCurrent--;
                if (countCurrent == 0) {
                    activePulse = 0;
                    PULSE1 = 0;
                    PULSE2 = 0;
                    LED = 0;
                    T2CONbits.TMR2ON = 0;
                }
            }
        }
        PIR2 &= ~(1 << 1);  // Clear Timer2 interrupt flag
    }
}



// Main Function
void main(void) {
    systemInit();
    lcdInit();
    updateDisplay();
    
    while(1) {
        char key = keypadScan();
        if (key) {
            handleKeypress(key);
            updateDisplay();
            __delay_ms(100); // Debounce delay
        }
    }
}

// System Initialization
void systemInit(void) {
    // Clock Configuration
    OSCCON1 = 0x60;  // HFINTOSC with divider 1 (64MHz)
    
    // Port Configuration
    TRISC = 0x00;    // All PORTC as output (LCD control)
    TRISD = 0x00;    // All PORTD as output (Pulses and LED)
    TRISB = 0x0F;    // RB0-RB3 as input (keypad rows), RB4-RB7 as output (columns)
    TRISA = 0x01;    // RA0 as input
    
    // Analog Configuration
    ANSELB = 0x00;   // All PORTB as digital
    ANSELA = 0x00;   // All PORTA as digital
    
    // Timer2 Configuration (1us interrupt at 64MHz)
    T2CON = 0x00;    // Timer2 off, prescaler 1:1, postscaler 1:1
    PR2 = 63;        // Period register (64 counts = 1us at 64MHz)
    PIE2 |= (1 << 1); // Enable Timer2 interrupt
    INTCON0bits.GIE = 1; // Enable global interrupts
    INTCON0bits.IPEN = 1; //Enable priority levels
}

// LCD Initialization
void lcdInit(void) {
    __delay_ms(50);  // Power-on delay
    
    // Initialization sequence
    lcdWriteCmd(0x30);
    __delay_ms(5);
    lcdWriteCmd(0x30);
    __delay_us(100);
    lcdWriteCmd(0x30);
    
    // Function set: 8-bit, 2 lines, 5x8 dots
    lcdWriteCmd(0x38);
    
    // Display control: Display off
    lcdWriteCmd(0x08);
    
    // Clear display
    lcdWriteCmd(0x01);
    
    // Entry mode set: Increment, no shift
    lcdWriteCmd(0x06);
    
    // Display control: Display on, cursor off, blink off
    lcdWriteCmd(0x0C);
}

// Write command to LCD
void lcdWriteCmd(unsigned char cmd) {
    LCD_RS = 0; // Command mode
    LCD_D0 = (cmd >> 0) & 0x01;
    LCD_D1 = (cmd >> 1) & 0x01;
    LCD_D2 = (cmd >> 2) & 0x01;
    LCD_D3 = (cmd >> 3) & 0x01;
    LCD_D4 = (cmd >> 4) & 0x01;
    LCD_D5 = (cmd >> 5) & 0x01;
    LCD_D6 = (cmd >> 6) & 0x01;
    LCD_D7 = (cmd >> 7) & 0x01;
    
    LCD_EN = 1;
    __delay_us(1);
    LCD_EN = 0;
    __delay_us(100);
}

// Write data to LCD
void lcdWriteData(unsigned char data) {
    LCD_RS = 1; // Data mode
    LCD_D0 = (data >> 0) & 0x01;
    LCD_D1 = (data >> 1) & 0x01;
    LCD_D2 = (data >> 2) & 0x01;
    LCD_D3 = (data >> 3) & 0x01;
    LCD_D4 = (data >> 4) & 0x01;
    LCD_D5 = (data >> 5) & 0x01;
    LCD_D6 = (data >> 6) & 0x01;
    LCD_D7 = (data >> 7) & 0x01;
    
    LCD_EN = 1;
    __delay_us(1);
    LCD_EN = 0;
    __delay_us(100);
}

// Print string to LCD
void lcdPrint(const char* str) {
    while(*str) {
        lcdWriteData(*str++);
    }
}

// Keypad Scanning Function
char keypadScan(void) {
    char keys[4][5] = {
        {'1', '2', '3', 'A', 'B'},
        {'4', '5', '6', 'C', 'D'},
        {'7', '8', '9', 'E', 'F'},
        {'*', '0', '#', 'G', 'H'}
    };
    
    // Check if any key is pressed
    COL1 = COL2 = COL3 = COL4 = COL5 = 1;
    if (ROW1 && ROW2 && ROW3 && ROW4) return 0; // No key pressed
    
    // Scan each column
    for (char col = 0; col < 5; col++) {
        // Activate one column at a time
        COL1 = COL2 = COL3 = COL4 = COL5 = 1;
        if (col == 0) COL1 = 0;
        else if (col == 1) COL2 = 0;
        else if (col == 2) COL3 = 0;
        else if (col == 3) COL4 = 0;
        else COL5 = 0;
        
        __delay_us(10); // Settling time
        
        // Check each row
        for (char row = 0; row < 4; row++) {
            if ((row == 0 && !ROW1) || (row == 1 && !ROW2) ||
                (row == 2 && !ROW3) || (row == 3 && !ROW4)) {
                char result = keys[row][col];
                while (!(ROW1 && ROW2 && ROW3 && ROW4)); // Wait for key release
                return result;
            }
        }
    }
    return 0;
}

// Handle Keypress Events
void handleKeypress(char key) {
    static unsigned long input_value = 0;
    static char mode = 0; // 0: on_time, 1: off_time, 2: timer
    
    if (key == 'A') { // ON/OFF toggle
        if (!activePulse) {
            activePulse = 1;
            countCurrent = timerDuration;
            LED = 1;
            T2CONbits.TMR2ON = 1;
        } else {
            activePulse = 0;
            LED = 0;
            PULSE1 = 0;
            PULSE2 = 0;
            T2CONbits.TMR2ON = 0;
        }
    } 
    else if (key == 'B') { // Select ON time
        mode = 0;
        input_value = onPulseTime;
    }
    else if (key == 'C') { // Select OFF time
        mode = 1;
        input_value = offPulseTime;
    }
    else if (key == 'D') { // Select timer duration
        mode = 2;
        input_value = timerDuration;
    }
    else if (key >= '0' && key <= '9') { // Numeric input
        input_value = input_value * 10 + (key - '0');
    }
    else if (key == '#') { // Enter key
        if (input_value >= 100 && input_value <= 999900) {
            if (mode == 0) onPulseTime = input_value;
            else if (mode == 1) offPulseTime = input_value;
            else timerDuration = input_value;
        }
        input_value = 0;
    }
}

// Update LCD Display
void updateDisplay(void) {
    char unit_on[3], unit_off[3];
    unsigned long on_time = onPulseTime;
    unsigned long off_time = offPulseTime;
    
    // Convert ON time to appropriate units
    if (on_time >= 1000000) {
        on_time /= 1000000;
        strcpy(unit_on, "s");
    } else if (on_time >= 1000) {
        on_time /= 1000;
        strcpy(unit_on, "ms");
    } else {
        strcpy(unit_on, "us");
    }
    
    // Convert OFF time to appropriate units
    if (off_time >= 1000000) {
        off_time /= 1000000;
        strcpy(unit_off, "s");
    } else if (off_time >= 1000) {
        off_time /= 1000;
        strcpy(unit_off, "ms");
    } else {
        strcpy(unit_off, "us");
    }
    
    // Format display string
    sprintf(displayBuffer, "ON:%lu%s OFF:%lu%s", 
            on_time, unit_on, off_time, unit_off);
    
    // Update LCD
    lcdWriteCmd(0x01); // Clear display
    lcdPrint(displayBuffer);
}