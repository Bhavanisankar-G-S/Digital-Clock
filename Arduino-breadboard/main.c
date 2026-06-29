#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <LiquidCrystal.h>

// Define BCD pins for seven-segment display
#define A_PIN PB0
#define B_PIN PB1
#define C_PIN PB2
#define D_PIN PB3

// Define common pins for seven-segment display digits
#define COMMON_HH_1 PC0
#define COMMON_HH_2 PC1
#define COMMON_MM_1 PC2
#define COMMON_MM_2 PC3
#define COMMON_SS_1 PC4
#define COMMON_SS_2 PC5

// Define buttons
#define MENU_BUTTON PD2
#define TOGGLE_BUTTON PD3
#define SELECT_BUTTON PD4

// Global variables for clock
volatile int hours = 0, minutes = 0, seconds = 0;

// Stopwatch variables
volatile int stopwatchMinutes = 0, stopwatchSeconds = 0;
volatile uint8_t stopwatchRunning = 0;

// Timer variables
volatile int timerHours = 0, timerMinutes = 0, timerSeconds = 0;
volatile uint8_t timerRunning = 0;

// Time zone variables
int timezoneOffsets[] = {0, -5, -10, 3, 4, 9, -7, 1};
const char* timezoneNames[] = {"IST", "UTC-5", "UTC-10", "UTC+3", "UTC+4", "UTC+9", "UTC-7", "UTC+1"};
volatile uint8_t currentZone = 0;

// State control
volatile uint8_t istTimeSet = 0;
volatile uint8_t menuState = 0;  // Tracks the menu option: 0 for time zones, 1 for stopwatch, 2 for timer

// Helper functions for bitwise arithmetic
int add(int a, int b) {
    while (b != 0) {
        int carry = a & b;
        a = a ^ b;
        b = carry << 1;
    }
    return a;
}

int subtract(int a, int b) {
    return add(a, add(~b, 1));
}

int multiply(int a, int b) {
    int result = 0;
    while (b != 0) {
        if (b & 1) result = add(result, a);
        a = a << 1;
        b = b >> 1;
    }
    return result;
}

int divide(int dividend, int divisor) {
    int quotient = 0;
    while (dividend >= divisor) {
        int tempDivisor = divisor, multiple = 1;
        while (dividend >= (tempDivisor << 1)) {
            tempDivisor = tempDivisor << 1;
            multiple = multiple << 1;
        }
        dividend = subtract(dividend, tempDivisor);
        quotient = add(quotient, multiple);
    }
    return quotient;
}

int modulo(int dividend, int divisor) {
    return subtract(dividend, multiply(divide(dividend, divisor), divisor));
}

// Timer interrupt for clock tick
ISR(TIMER1_COMPA_vect) {
    if (istTimeSet) {
        seconds = add(seconds, 1);
        if (seconds >= 60) {
            seconds = 0;
            minutes = add(minutes, 1);
            if (minutes >= 60) {
                minutes = 0;
                hours = add(hours, 1);
                if (hours >= 24) {
                    hours = 0;
                }
            }
        }
    }

    if (stopwatchRunning) {
        stopwatchSeconds = add(stopwatchSeconds, 1);
        if (stopwatchSeconds >= 60) {
            stopwatchSeconds = 0;
            stopwatchMinutes = add(stopwatchMinutes, 1);
        }
    }

    if (timerRunning) {
        if (timerSeconds > 0) {
            timerSeconds = subtract(timerSeconds, 1);
        } else if (timerMinutes > 0) {
            timerMinutes = subtract(timerMinutes, 1);
            timerSeconds = 59;
        } else if (timerHours > 0) {
            timerHours = subtract(timerHours, 1);
            timerMinutes = 59;
            timerSeconds = 59;
        } else {
            timerRunning = 0;  // Timer finished
        }
    }
}

// Seven-segment display functions
void displayDigit(uint8_t commonPin, uint8_t digit) {
    PORTB = (digit & 0x0F);
    PORTC |= (1 << commonPin);
    _delay_ms(1);
    PORTC &= ~(1 << commonPin);
}

void displayClock() {
    int hh1 = divide(hours, 10);
    int hh2 = modulo(hours, 10);
    int mm1 = divide(minutes, 10);
    int mm2 = modulo(minutes, 10);
    int ss1 = divide(seconds, 10);
    int ss2 = modulo(seconds, 10);

    displayDigit(COMMON_HH_1, hh1);
    displayDigit(COMMON_HH_2, hh2);
    displayDigit(COMMON_MM_1, mm1);
    displayDigit(COMMON_MM_2, mm2);
    displayDigit(COMMON_SS_1, ss1);
    displayDigit(COMMON_SS_2, ss2);
}
void handleMenu() {
    // Menu state variables
    static uint8_t currentMenu = 0;    // Current menu index
    static uint8_t inSubmenu = 0;     // Tracks if inside a submenu

    // Menu options
    const char* menuOptions[] = {"Time Zones", "Stopwatch", "Timer"};
    const uint8_t totalMenus = 3;     // Total menu options available

    // If MENU button is pressed, return to the main menu
    if (!(PIND & (1 << MENU_BUTTON))) {  // Check MENU_BUTTON (Active-low logic)
        _delay_ms(200);  // Debounce delay
        if (inSubmenu) {
            inSubmenu = 0;  // Exit submenu
            currentMenu = 0;  // Reset menu selection to the first option
        }
    }

    // Main menu navigation
    if (!inSubmenu) {
        // If TOGGLE button is pressed, cycle through menu options
        if (!(PIND & (1 << TOGGLE_BUTTON))) {  // Check TOGGLE_BUTTON
            _delay_ms(200);  // Debounce delay
            currentMenu = (currentMenu + 1) % totalMenus;  // Cycle through menu options
        }

        // Display the current menu
        lcd_clear();  // Clear LCD display
        for (uint8_t i = 0; i < totalMenus; i++) {
            lcd_setCursor(0, i);  // Set cursor to line i
            if (i == currentMenu) {
                lcd_print("> ");  // Highlight the current menu option
            } else {
                lcd_print("  ");  // Indent other options
            }
            lcd_print(menuOptions[i]);  // Display menu option text
        }

        // If SELECT button is pressed, enter the selected submenu
        if (!(PIND & (1 << SELECT_BUTTON))) {  // Check SELECT_BUTTON
            _delay_ms(200);  // Debounce delay
            inSubmenu = 1;  // Enter submenu
            lcd_clear();  // Clear LCD display

            // Handle submenu options based on the current menu
            switch (currentMenu) {
                case 0:  // Time Zones
                    lcd_print("Time Zones");
                    _delay_ms(1000);
                    handleTimeZones();  // Call time zone handler
                    break;

                case 1:  // Stopwatch
                    lcd_print("Stopwatch");
                    _delay_ms(1000);
                    handleStopwatch();  // Call stopwatch handler
                    break;

                case 2:  // Timer
                    lcd_print("Timer");
                    _delay_ms(1000);
                    handleTimer();  // Call timer handler
                    break;

                default:
                    break;
            }
        }
    }
}

// Time Zones
void handleTimeZones() {
    lcd_clear();
    lcd_print("Select Time Zone");
    _delay_ms(1000);

    while (1) {
        lcd_clear();
        lcd_print(timezoneNames[currentZone]);
        lcd_setCursor(0, 1);
        lcd_print("Offset: ");
        lcd_print(timezoneOffsets[currentZone]);
        lcd_print("h");

        if (!(PIND & (1 << TOGGLE_BUTTON))) {  // Cycle through time zones
            _delay_ms(200);
            currentZone = add(currentZone, 1) % 8;
        }

        if (!(PIND & (1 << SELECT_BUTTON))) {  // Confirm selection
            _delay_ms(200);
            hours = add(hours, timezoneOffsets[currentZone]);
            if (hours >= 24) hours = subtract(hours, 24);
            if (hours < 0) hours = add(hours, 24);
            break;
        }
    }
}

// Stopwatch
void handleStopwatch() {
    lcd_clear();
    lcd_print("Stopwatch Started");
    _delay_ms(1000);

    while (1) {
        if (stopwatchRunning) {
            int mm1 = divide(stopwatchMinutes, 10);
            int mm2 = modulo(stopwatchMinutes, 10);
            int ss1 = divide(stopwatchSeconds, 10);
            int ss2 = modulo(stopwatchSeconds, 10);

            displayDigit(COMMON_MM_1, mm1);
            displayDigit(COMMON_MM_2, mm2);
            displayDigit(COMMON_SS_1, ss1);
            displayDigit(COMMON_SS_2, ss2);
        }

        if (!(PIND & (1 << SELECT_BUTTON))) {  // Toggle running state
            _delay_ms(200);
            stopwatchRunning = !stopwatchRunning;
        }

        if (!(PIND & (1 << MENU_BUTTON))) {  // Exit stopwatch
            _delay_ms(200);
            stopwatchRunning = 0;
            break;
        }
    }
}

// Timer
void handleTimer() {
    lcd_clear();
    lcd_print("Set Timer HH:MM:SS");
    _delay_ms(1000);

    timerHours = 0; timerMinutes = 0; timerSeconds = 0;

    while (1) {
        lcd_clear();
        lcd_print("Timer: ");
        lcd_setCursor(0, 1);
        lcd_print("HH:MM:SS ");
        lcd_print(timerHours); lcd_print(":");
        lcd_print(timerMinutes); lcd_print(":");
        lcd_print(timerSeconds);

        if (timerRunning) {
            int hh1 = divide(timerHours, 10);
            int hh2 = modulo(timerHours, 10);
            int mm1 = divide(timerMinutes, 10);
            int mm2 = modulo(timerMinutes, 10);
            int ss1 = divide(timerSeconds, 10);
            int ss2 = modulo(timerSeconds, 10);

            displayDigit(COMMON_HH_1, hh1);
            displayDigit(COMMON_HH_2, hh2);
            displayDigit(COMMON_MM_1, mm1);
            displayDigit(COMMON_MM_2, mm2);
            displayDigit(COMMON_SS_1, ss1);
            displayDigit(COMMON_SS_2, ss2);
        }

        if (!(PIND & (1 << MENU_BUTTON))) {  // Exit timer
            _delay_ms(200);
            timerRunning = 0;
            break;
        }
    }
}

// Setup
void setup() {
    DDRB = 0x0F;
    DDRC = 0x3F;
    DDRD &= ~(1 << MENU_BUTTON | 1 << TOGGLE_BUTTON | 1 << SELECT_BUTTON);
    PORTD |= (1 << MENU_BUTTON | 1 << TOGGLE_BUTTON | 1 << SELECT_BUTTON);

    TCCR1B |= (1 << WGM12);
    TCCR1B |= (1 << CS12 | 1 << CS10);
    OCR1A = 15624;
    TIMSK1 |= (1 << OCIE1A);

    sei();
}

// Main loop
int main(void) {
    // Call the setup function to configure hardware
    setup();

    while (1) {
        if (istTimeSet) {
            // Continuously display the clock on the seven-segment displays
            displayClock();
        }

        // Handle the main menu and navigation logic
        handleMenu();

        // Add other features to be run in the main loop as needed
    }
}
