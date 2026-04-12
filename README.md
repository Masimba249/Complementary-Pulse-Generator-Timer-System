# Complementary Pulse Generator & Timer System

##  Project Overview
This project involves the development of an embedded system designed to generate highly configurable complementary pulses. Based on a PIC18F microcontroller, the system features a dedicated keypad for user input and a graphic LCD for real-time parameter monitoring. 

---

##  Hardware Specifications

The system is built using the following core hardware components:
* [cite_start]**Microcontroller**: PIC18F46Q84-I/P[cite: 2].
* [cite_start]**Display**: 12864B Graphic Blue Color Backlight LCD Display Module[cite: 4].
* [cite_start]**Input Device**: A custom keypad consisting of 21 switches[cite: 6].
* [cite_start]**Status Indicator**: An LED that reflects the operational state of the pulses[cite: 15].

##  Software & Tools
* [cite_start]**Development Environment**: MPLAB IDE[cite: 3].
* [cite_start]**Programming/Debugging Tool**: Pickit-3[cite: 3].

---

##  Functional Requirements & Features

The system is programmed to fulfill the following operational criteria:

### Pulse Generation
* [cite_start]The system requires the generation of two complimentary pulses[cite: 8].
* [cite_start]The "on time" and "off time" of the pulses can be adjusted directly from the keypad[cite: 8].
* [cite_start]The allowable range for the on time and off time values spans from 100us to 9999s[cite: 10].
* [cite_start]The user can enter any value within this specified range with a precise 1us resolution[cite: 10].

### User Interface & Display
* [cite_start]The display shows the exact data entered from the keypad, specifically displaying the pulse on time and off time in us/ms/s[cite: 9].
* [cite_start]The keypad includes an ON/OFF button specifically used to turn on the pulses[cite: 13].

### Timer Integration & Automation
* [cite_start]The system provides an adjustable timer[cite: 11].
* [cite_start]The timer's count range operates from 100us to 9999s[cite: 11].
* [cite_start]Users can input any timer value within this range with a 1us resolution[cite: 12].
* [cite_start]The timer starts automatically as soon as the ON/OFF button is pressed to turn on the pulses[cite: 14].
* [cite_start]The pulses will turn off automatically once the timer finishes its counting sequence[cite: 14].
* [cite_start]A status LED is configured to turn ON when the pulses are active and will turn OFF when the pulses are inactive[cite: 15].