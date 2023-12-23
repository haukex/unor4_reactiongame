/** Main Arduino Sketch.
 *
 * Dev Notes:
 * - millis() and micros() are unsigned long, which is uint32_t. millis overflow is 49.7 days, micros is 71.6 minutes.
 * - Normal, fast button presses got down to 20-30ms, but with very quick presses I was able to get down to 8-9ms with some bouncing.
 * - Testing with POLL_TIME_TEST showed a polling interval of around 20us, which is fast enough.
 *
 * Copyright © 2023 by Hauke Dämpfling (haukex@zero-g.net). All Rights Reserved.
 */

/* ********** ********** ********** Globals ********** ********** ********** */

const String VERSION = String("$Id$").substring(5,13);

#define delayMilliseconds(x)   delay(x)

#define SERIAL_TIMEOUT_MS (10000)


/* ********** ********** ********** LCD Display ********** ********** ********** */
// https://github.com/johnrickman/LiquidCrystal_I2C
// seems to work fine despite the message
// "WARNING: library LiquidCrystal I2C claims to run on avr architecture(s) and may
// be incompatible with your current board which runs on renesas_uno architecture(s)."

#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,16,2); // address, width, height

void init_lcd() {
  lcd.init();
  lcd.backlight();

  // the following code takes ~48ms (measured)
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
const char BTN_NAMES[BUTTONS] =  {'R', 'G', 'B', 'Y', 'A', 'C', 'D', 'E'};
const pin_size_t BTNS[BUTTONS] = { A5,  A4,  A3,  A2,  A1,  A0,  D8,  D9};
const pin_size_t LEDS[BUTTONS] = { D0,  D1,  D2,  D3,  D4,  D5,  D6,  D7};
const unsigned long BTN_IGNORE_TIME_MS = 100;  // basic debouncing
const size_t MENUBTN_R = 0;
const size_t MENUBTN_G = 1;
const size_t MENUBTN_B = 2;
const size_t MENUBTN_Y = 3;

unsigned long btn_ignore_until_millis[BUTTONS];
bool btn_prev_state[BUTTONS];
bool btn_cur_state[BUTTONS];

#undef POLL_TIME_TEST
#ifdef POLL_TIME_TEST
unsigned long prev_poll_time_us;
unsigned long cur_poll_interval_us;
#endif

void init_buttons() {
  for(size_t i=0; i<BUTTONS; i++) {
    pinMode(LEDS[i], OUTPUT);
    digitalWrite(LEDS[i], LOW);
    pinMode(BTNS[i], INPUT_PULLUP);
    btn_ignore_until_millis[i] = 0;
  }
  check_buttons();  // initialize variables
}

bool check_buttons() {
#ifdef POLL_TIME_TEST
  const unsigned long now_us = micros();
  cur_poll_interval_us = now_us - prev_poll_time_us;
  prev_poll_time_us = now_us;
#endif
  bool any_changes = false;
  const unsigned long now_ms = millis();
  for(size_t i=0; i<BUTTONS; i++) {
    if ( now_ms > btn_ignore_until_millis[i] ) {
      btn_cur_state[i] = digitalRead(BTNS[i])==LOW;
      if ( btn_cur_state[i] != btn_prev_state[i] ) {
        any_changes = true;
        btn_ignore_until_millis[i] = now_ms + BTN_IGNORE_TIME_MS;
      }
      btn_prev_state[i] = btn_cur_state[i];
    }
  }
  return any_changes;
}


/* ********** ********** ********** Speaker ********** ********** ********** */
// using a noname speaker driver IC that plays a sound when a pin is set high

#define AUDIO_TRIGGER_PIN D10

void init_audio() {
  digitalWrite(AUDIO_TRIGGER_PIN, LOW);
  pinMode(AUDIO_TRIGGER_PIN, OUTPUT);
}

void play_audio() {
  digitalWrite(AUDIO_TRIGGER_PIN, HIGH);
  delayMilliseconds(20);  // testing shows 10ms seems reliable, playing it safe here
  digitalWrite(AUDIO_TRIGGER_PIN, LOW);
}


/* ********** ********** ********** Main Code ********** ********** ********** */

void setup() {
  Serial.begin(115200);
  digitalWrite(LED_BUILTIN, HIGH);
  pinMode(LED_BUILTIN, OUTPUT);
  //while (!Serial);  // no, don't wait for USB-Serial connection to boot
  // instead, just wait a few seconds for PC to catch up if connected:
  //TODO: remove delay for prod
  delayMilliseconds(3000);
  Serial.setTimeout(SERIAL_TIMEOUT_MS);
  Serial.println(F("==========> Booting..."));
  Serial.print(F("Firmware version: "));
  Serial.println(VERSION);

  init_lcd(); // NOTE this hangs if LCD not connected
  init_audio();
  init_buttons();

  Serial.println(F("========== ==========> Ready! <========== =========="));
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
  if ( check_buttons() ) {
#ifdef POLL_TIME_TEST
    Serial.print("Poll interv. ");
    Serial.print(cur_poll_interval_us);
    Serial.println("us");
#endif
    for(size_t i=0; i<BUTTONS; i++) {
      digitalWrite(LEDS[i], btn_cur_state[i] ? HIGH : LOW);
      if (btn_cur_state[i]) {
        Serial.print("Button ");
        Serial.println(BTN_NAMES[i]);
      }
    }
  }
  //TODO: Implement actual game...
}
