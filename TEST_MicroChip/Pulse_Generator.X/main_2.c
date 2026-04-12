/* 
 * File:   main_2.c
 * Author: Collins_m
 *
 * Created on July 5, 2025, 10:22 AM
 */

#include <xc.h>
#include <string.h>
#include <stdio.h>

// Define oscillator frequency for delay macros
#define _XTAL_FREQ 40000000UL // 20 MHz crystal with 4x PLL = 40 MHz Fosc

// PIC18F4580 Configuration Bit Settings
// CONFIG1H
#pragma config OSC = HSPLL     // HS oscillator with PLL enabled (4x PLL for 20 MHz crystal)
#pragma config FCMEN = OFF     // Fail-Safe Clock Monitor disabled
#pragma config IESO = OFF      // Internal/External Oscillator Switchover disabled

// CONFIG2L
#pragma config PWRT = OFF      // Power-up Timer disabled
#pragma config BOREN = OFF     // Brown-out Reset disabled
#pragma config BORV = 3        // Brown-out Reset Voltage set to 2.1V (unused)

// CONFIG2H
#pragma config WDT = OFF       // Watchdog Timer disabled
#pragma config WDTPS = 32768   // WDT Postscaler 1:32768 (unused)

// CONFIG3H
#pragma config PBADEN = OFF    // PORTB<4:0> pins are digital on Reset (for keypad)
#pragma config LPT1OSC = OFF   // Low-Power Timer1 Oscillator disabled
#pragma config MCLRE = ON      // MCLR pin enabled; RE3 input pin disabled

// CONFIG4L
#pragma config STVREN = ON     // Stack Full/Underflow Reset enabled
#pragma config LVP = OFF       // Single-Supply ICSP disabled
#pragma config BBSIZ = 1024    // Boot Block Size 1K words (2K bytes)
#pragma config XINST = OFF     // Extended Instruction Set disabled

// CONFIG5L
#pragma config CP0 = OFF       // Block 0 not code-protected
#pragma config CP1 = OFF       // Block 1 not code-protected
#pragma config CP2 = OFF       // Block 2 not code-protected
#pragma config CP3 = OFF       // Block 3 not code-protected

// CONFIG5H
#pragma config CPB = OFF       // Boot Block not code-protected
#pragma config CPD = OFF       // Data EEPROM not code-protected

// CONFIG6L
#pragma config WRT0 = OFF      // Block 0 not write-protected
#pragma config WRT1 = OFF      // Block 1 not write-protected
#pragma config WRT2 = OFF      // Block 2 not write-protected
#pragma config WRT3 = OFF      // Block 3 not write-protected

// CONFIG6H
#pragma config WRTC = OFF      // Configuration Registers not write-protected
#pragma config WRTB = OFF      // Boot Block not write-protected
#pragma config WRTD = OFF      // Data EEPROM not write-protected

// CONFIG7L
#pragma config EBTR0 = OFF     // Block 0 not protected from table reads
#pragma config EBTR1 = OFF     // Block 1 not protected from table reads
#pragma config EBTR2 = OFF     // Block 2 not protected from table reads
#pragma config EBTR3 = OFF     // Block 3 not protected from table reads

// CONFIG7H
#pragma config EBTRB = OFF     // Boot Block not protected from table reads

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
void __interrupt() isr(void) {
    if (PIR1bits.TMR2IF) {  // Check Timer2 interrupt flag
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
        PIR1bits.TMR2IF = 0;  // Clear Timer2 interrupt flag
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
    OSCCON = 0x00;  // Default to OSC config (HSPLL set in config bits)
    
    // Port Configuration
    TRISC = 0x00;    // All PORTC as output (LCD control)
    TRISD = 0x00;    // All PORTD as output (Pulses and LED)
    TRISB = 0x0F;    // RB0-RB3 as input (keypad rows), RB4-RB7 as output (columns)
    TRISA = 0x01;    // RA0 as input
    
    // Analog Configuration
    ADCON1 = 0x0F;   // All PORTA and PORTB as digital
    CMCON = 0x07;    // Comparators off for digital I/O
    
    // Timer2 Configuration (1us interrupt at 40MHz)
    T2CON = 0x00;    // Timer2 off, prescaler 1:1, postscaler 1:1
    PR2 = 39;        // Period = (PR2+1)*4/Fosc = (39+1)*4/40MHz = 1us
    PIE1bits.TMR2IE = 1; // Enable Timer2 interrupt
    INTCONbits.GIE = 1;  // Enable global interrupts
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
    char keys[4][4] = {
        {'1', '2', '3', 'A'},
        {'4', '5', '6', 'B'},
        {'7', '8', '9', 'C'},
        {'*', '0', '#', 'D'}
    };
    
    // Check if any key is pressed
    COL1 = COL2 = COL3 = COL4 = 1;
    if (ROW1 && ROW2 && ROW3 && ROW4) return 0; // No key pressed
    
    // Scan each column
    for (char col = 0; col < 4; col++) {
        // Activate one column at a time
        COL1 = COL2 = COL3 = COL4 = 1;
        if (col == 0) COL1 = 0;
        else if (col == 1) COL2 = 0;
        else if (col == 2) COL3 = 0;
        else COL4 = 0;
        
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