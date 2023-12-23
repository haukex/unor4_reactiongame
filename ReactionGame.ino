/** Main Arduino Sketch.
 *
 * Copyright © 2023 by Hauke Dämpfling (haukex@zero-g.net). All Rights Reserved.
 */

/* ********** ********** ********** Globals ********** ********** ********** */

const String VERSION = String("$Id$").substring(5,13);

#define delayMilliseconds(x)   delay(x)

#define SERIAL_TIMEOUT_MS (10000)


/* ********** ********** ********** LCD Display ********** ********** ********** */
// https://github.com/johnrickman/LiquidCrystal_I2C
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,16,2); // address, width, height

void init_lcd() {
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0,0); // col, row
  lcd.print("Hello, World!");
  lcd.setCursor(0,1);
  lcd.print("This is a test.");
}


/* ********** ********** ********** EEPROM ********** ********** ********** */
#include <EEPROM.h>

//TODO: Read/write high score


/* ********** ********** ********** Buttons ********** ********** ********** */

const size_t BUTTONS = 8;
const pin_size_t LEDS[BUTTONS] = {D0, D1, D2, D3, D4, D5, D6, D7};
const pin_size_t BTNS[BUTTONS] = {D8, D9, A0, A1, A2, A3, A4, A5};
bool isSet[BUTTONS];

void init_buttons() {
  for(size_t i=0; i<BUTTONS; i++) {
    pinMode(LEDS[i], OUTPUT);
    digitalWrite(LEDS[i], LOW);
    pinMode(BTNS[i], INPUT_PULLUP);
  }
  check_buttons();
}

bool check_buttons() {
  bool anySet = false;
  for(size_t i=0; i<BUTTONS; i++) {
    //TODO: Debounce buttons if needed?
    isSet[i] = digitalRead(BTNS[i])==LOW;
    if (isSet[i]) anySet = true;
  }
  return anySet;
}

/* ********** ********** ********** Main Code ********** ********** ********** */

#define DEBUG_PIN D10

void setup() {
  Serial.begin(115200);
#ifdef DEBUG_PIN
  pinMode(DEBUG_PIN, OUTPUT);
  digitalWrite(DEBUG_PIN, LOW);
#endif
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  //while (!Serial);  // no, don't wait for USB-Serial connection to boot
  // instead, just wait a few seconds for PC to catch up if connected:
  delayMilliseconds(3000);
  Serial.setTimeout(SERIAL_TIMEOUT_MS);
  Serial.println(F("==========> Booting..."));
  Serial.print(F("Firmware version: "));
  Serial.println(VERSION);

  init_lcd(); // NOTE this hangs if LCD not connected
  init_buttons();

  Serial.println(F("========== ==========> Ready! <========== =========="));
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
  // Note to Self: millis() and micros() are unsigned long, which is uint32_t. millis overflow is 49.7 days, micros is 71.6 minutes.
#ifdef DEBUG_PIN
  // => Pulse the pin so we can measure update frequency:
  // Testing with this code shows about 46.4kHz (~21us) with some jitter.
  // Normal, fast button presses got down to 20-30ms, but with very quick presses
  // I was able to get down to 8-9ms with some bouncing.
  digitalWrite(DEBUG_PIN, HIGH);
  digitalWrite(DEBUG_PIN, LOW);
#endif
  check_buttons();
  for(size_t i=0; i<BUTTONS; i++)
    digitalWrite(LEDS[i], isSet[i] ? HIGH : LOW);
  //TODO: Implement actual game...
}
