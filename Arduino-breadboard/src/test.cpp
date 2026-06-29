#include <Arduino.h>

// BCD Pins (for the 7-seg driver's binary input)
#define A 2
#define B 3
#define C 4
#define D 5

// Display Common Pins for each digit
#define H1 6
#define H2 7
#define M1 8
#define M2 9
#define S1 10
#define S2 11

// Time-set Buttons (Analog inputs as digital)
#define SET_HOUR A1
#define SET_MIN A2
#define SET_SEC A0

// Global BCD digits for the clock (each digit stored separately)
volatile uint8_t h1 = 0;  // tens of hours
volatile uint8_t h2 = 0;  // ones of hours
volatile uint8_t m1 = 0;  // tens of minutes
volatile uint8_t m2 = 0;  // ones of minutes
volatile uint8_t s1 = 0;  // tens of seconds
volatile uint8_t s2 = 0;  // ones of seconds

// Multiplexing control variables
uint8_t current_digit = 0;
unsigned long last_mux = 0;
const uint8_t mux_interval = 1; // 1ms per digit

// Timekeeping control
unsigned long last_second = 0;

// Button debouncing
unsigned long last_button_check = 0;
const uint16_t debounce_interval = 200;

/*
  bcdIncrement()
  Increments a BCD digit by one using bit‐wise operations.
  If the digit equals max, it rolls over to 0.
*/
uint8_t bcdIncrement(uint8_t bcd, uint8_t max) {
  if (bcd == max) {
    return 0;
  }
  uint8_t d0 = bcd & 0x01;
  uint8_t d1 = (bcd >> 1) & 0x01;
  uint8_t d2 = (bcd >> 2) & 0x01;
  uint8_t d3 = (bcd >> 3) & 0x01;
  
  uint8_t n0 = !d0;
  uint8_t c0 = d0;
  uint8_t n1 = d1 ^ c0;
  uint8_t c1 = d1 & c0;
  uint8_t n2 = d2 ^ c1;
  uint8_t c2 = d2 & c1;
  uint8_t n3 = d3 ^ c2;
  
  uint8_t result = (n3 << 3) | (n2 << 2) | (n1 << 1) | n0;
  return result;
}

/*
  updateSeconds()
  Increments seconds digits using BCD logic.
*/
void updateSeconds() {
  if (s2 == 9) {
    s2 = bcdIncrement(s2, 9); // becomes 0
    s1 = bcdIncrement(s1, 5);
  } else {
    s2 = bcdIncrement(s2, 9);
  }
}

/*
  updateMinutes()
  Increments minutes digits using BCD logic.
*/
void updateMinutes() {
  if (m2 == 9) {
    m2 = bcdIncrement(m2, 9); // becomes 0
    m1 = bcdIncrement(m1, 5);
  } else {
    m2 = bcdIncrement(m2, 9);
  }
}

/*
  updateHours()
  Increments the hours digits in 24-hour format using corrected logic.
*/
void updateHours() {
  if (h1 == 2) {
    if (h2 == 3) {
      // Wrap from 23 to 00.
      h1 = 0;
      h2 = 0;
    } else {
      h2 = bcdIncrement(h2, 3);
    }
  } else {
    if (h2 == 9) {
      h2 = bcdIncrement(h2, 9); // becomes 0
      // Increment tens digit properly:
      if (h1 == 0) {
        // For hours 09 -> 10, h1 goes 0 -> 1.
        h1 = bcdIncrement(h1, 1);
      } else if (h1 == 1) {
        // For hours 19 -> 20, h1 should go 1 -> 2.
        h1 = bcdIncrement(h1, 2);
      }
    } else {
      h2 = bcdIncrement(h2, 9);
    }
  }
}

/*
  updateTime()
  Checks every second and increments the time accordingly.
*/
void updateTime() {
  if (millis() - last_second >= 1000) {
    last_second += 1000;
    updateSeconds();
    if (s1 == 0 && s2 == 0) {
      updateMinutes();
      if (m1 == 0 && m2 == 0) {
        updateHours();
      }
    }
  }
}

/*
  set_time()
  Adjusts time based on button presses.
*/
void set_time() {
  if (millis() - last_button_check > debounce_interval) {
    // Hours button
    if (digitalRead(SET_HOUR) == LOW) {
      if (h1 == 2) {
        if (h2 == 3) {
          h1 = 0;
          h2 = 0;
        } else {
          h2 = bcdIncrement(h2, 3);
        }
      } else {
        if (h2 == 9) {
          h2 = bcdIncrement(h2, 9);
          if (h1 == 0) {
            h1 = bcdIncrement(h1, 1);
          } else if (h1 == 1) {
            h1 = bcdIncrement(h1, 2);
          }
        } else {
          h2 = bcdIncrement(h2, 9);
        }
      }
    }
    // Minutes button
    if (digitalRead(SET_MIN) == LOW) {
      if (m2 == 9) {
        m2 = bcdIncrement(m2, 9);
        m1 = bcdIncrement(m1, 5);
      } else {
        m2 = bcdIncrement(m2, 9);
      }
    }
    // Seconds button
    if (digitalRead(SET_SEC) == LOW) {
      if (s2 == 9) {
        s2 = bcdIncrement(s2, 9);
        s1 = bcdIncrement(s1, 5);
      } else {
        s2 = bcdIncrement(s2, 9);
      }
    }
    last_button_check = millis();
  }
}

/*
  displayDigit()
  Sends a 4-bit BCD digit to the output pins and activates the common pin.
*/
void displayDigit(uint8_t commonPin, uint8_t digit) {
  digitalWrite(A, LOW);
  digitalWrite(B, LOW);
  digitalWrite(C, LOW);
  digitalWrite(D, LOW);

  digitalWrite(A, digit & 0x01);
  digitalWrite(B, (digit >> 1) & 0x01);
  digitalWrite(C, (digit >> 2) & 0x01);
  digitalWrite(D, (digit >> 3) & 0x01);

  digitalWrite(commonPin, HIGH);
}

/*
  multiplexDisplay()
  Cycles through the 6 digits (hours, minutes, seconds) quickly.
*/
void multiplexDisplay() {
  if (millis() - last_mux >= mux_interval) {
    digitalWrite(H1, LOW);
    digitalWrite(H2, LOW);
    digitalWrite(M1, LOW);
    digitalWrite(M2, LOW);
    digitalWrite(S1, LOW);
    digitalWrite(S2, LOW);

    const uint8_t digits[] = { h1, h2, m1, m2, s1, s2 };
    const uint8_t pins[] = { H1, H2, M1, M2, S1, S2 };

    displayDigit(pins[current_digit], digits[current_digit]);
    current_digit = (current_digit + 1) % 6;
    last_mux = millis();
  }
}

void setup() {
  pinMode(A, OUTPUT);
  pinMode(B, OUTPUT);
  pinMode(C, OUTPUT);
  pinMode(D, OUTPUT);

  uint8_t commonPins[] = { H1, H2, M1, M2, S1, S2 };
  for (uint8_t i = 0; i < 6; i++) {
    pinMode(commonPins[i], OUTPUT);
    digitalWrite(commonPins[i], LOW);
  }

  pinMode(SET_HOUR, INPUT_PULLUP);
  pinMode(SET_MIN, INPUT_PULLUP);
  pinMode(SET_SEC, INPUT_PULLUP);
}

void loop() {
  set_time();
  updateTime();
  multiplexDisplay();
}

