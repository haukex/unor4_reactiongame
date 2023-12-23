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
#include <limits.h>

const String VERSION = String("$Id$").substring(5,21);

#define delayMilliseconds(x)   delay(x)

#define SERIAL_TIMEOUT_MS (10000)


/* ********** ********** ********** LCD Display ********** ********** ********** */
// https://github.com/johnrickman/LiquidCrystal_I2C
// seems to work fine despite the message
// "WARNING: library LiquidCrystal I2C claims to run on avr architecture(s) and may
// be incompatible with your current board which runs on renesas_uno architecture(s)."

#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,16,2); // address, width, height

void lcd_write(const char *one, const char *two) {
  lcd.clear();
  if (one!=NULL) {
    lcd.setCursor(0,0); // col, row
    lcd.print(one);
  }
  if (two!=NULL) {
    lcd.setCursor(0,1);
    lcd.print(two);
  }
}

void init_lcd() {
  lcd.init();
  lcd.backlight();
}


/* ********** ********** ********** EEPROM ********** ********** ********** */
#include <EEPROM.h>

//TODO Later: there should probably be a different high score per game length
const int EE_HISCORE_ADDR = 0;
unsigned long ee_high_score;

void init_hiscore() {
  EEPROM.get(EE_HISCORE_ADDR, ee_high_score);
  Serial.print(F("Loaded high score: "));
  Serial.println(ee_high_score);
}

void save_hiscore() {
  EEPROM.put(EE_HISCORE_ADDR, ee_high_score);
}


/* ********** ********** ********** Buttons ********** ********** ********** */

const size_t BUTTONS = 8;
const char BTN_NAMES[BUTTONS] =  { 'R', 'G', 'B', 'Y', 'A', 'C', 'D', 'E'};
const pin_size_t BTNS[BUTTONS] = { D10, D11,  A3,  A2,  A1,  A0,  D8,  D9};
const pin_size_t LEDS[BUTTONS] = {  D0,  D1,  D2,  D3,  D4,  D5,  D6,  D7};
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

void flash_leds(const size_t count, const unsigned long delay_ms) {
  for (size_t c=0; c<count; c++) {
    if (c) delayMilliseconds(delay_ms);
    for(size_t i=0; i<BUTTONS; i++) digitalWrite(LEDS[i], HIGH);
    delayMilliseconds(delay_ms);
    for(size_t i=0; i<BUTTONS; i++) digitalWrite(LEDS[i], LOW);
  }
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

size_t any_pressed() {
  for (size_t i=0; i<BUTTONS; i++)
    if (btn_cur_state[i]) return i+1;
  return 0;
}

const uint8_t MENUFLAG_R = 0b00000001;
const uint8_t MENUFLAG_G = 0b00000010;
const uint8_t MENUFLAG_B = 0b00000100;
const uint8_t MENUFLAG_Y = 0b00001000;

uint8_t do_menu(uint8_t choices) {
  // wait for buttons to be released if pressed
  while (any_pressed()) check_buttons();
  // turn on the choice LEDs
  if ( choices & MENUFLAG_R ) digitalWrite(LEDS[MENUBTN_R], HIGH);
  if ( choices & MENUFLAG_G ) digitalWrite(LEDS[MENUBTN_G], HIGH);
  if ( choices & MENUFLAG_B ) digitalWrite(LEDS[MENUBTN_B], HIGH);
  if ( choices & MENUFLAG_Y ) digitalWrite(LEDS[MENUBTN_Y], HIGH);
  // wait for a button to be pressed
  uint8_t chose = 0;
  while (true) {
    if (check_buttons()) {
      if ( (choices & MENUFLAG_R) && btn_cur_state[MENUBTN_R] ) { chose=MENUFLAG_R; break; }
      if ( (choices & MENUFLAG_G) && btn_cur_state[MENUBTN_G] ) { chose=MENUFLAG_G; break; }
      if ( (choices & MENUFLAG_B) && btn_cur_state[MENUBTN_B] ) { chose=MENUFLAG_B; break; }
      if ( (choices & MENUFLAG_Y) && btn_cur_state[MENUBTN_Y] ) { chose=MENUFLAG_Y; break; }
    }
  }
  // turn off all other LEDS
  digitalWrite(LEDS[MENUBTN_R], chose&MENUFLAG_R ? HIGH : LOW);
  digitalWrite(LEDS[MENUBTN_G], chose&MENUFLAG_G ? HIGH : LOW);
  digitalWrite(LEDS[MENUBTN_B], chose&MENUFLAG_B ? HIGH : LOW);
  digitalWrite(LEDS[MENUBTN_Y], chose&MENUFLAG_Y ? HIGH : LOW);
  // turn off the selected LED after button is released
  while (any_pressed()) check_buttons();
  digitalWrite(LEDS[MENUBTN_R], LOW);
  digitalWrite(LEDS[MENUBTN_G], LOW);
  digitalWrite(LEDS[MENUBTN_B], LOW);
  digitalWrite(LEDS[MENUBTN_Y], LOW);
  return chose;
}

/* ********** ********** ********** Speaker ********** ********** ********** */
// using a noname speaker driver IC that plays a sound when a pin is set high

#define AUDIO_TRIGGER_PIN D12

void init_audio() {
  digitalWrite(AUDIO_TRIGGER_PIN, LOW);
  pinMode(AUDIO_TRIGGER_PIN, OUTPUT);
}

void play_audio() {
  digitalWrite(AUDIO_TRIGGER_PIN, HIGH);
  delayMilliseconds(20);  // testing shows 10ms seems reliable, playing it safe here
  digitalWrite(AUDIO_TRIGGER_PIN, LOW);
}


/* ********** ********** ********** Initialization ********** ********** ********** */

void setup() {
  Serial.begin(115200);
  digitalWrite(LED_BUILTIN, HIGH);
  pinMode(LED_BUILTIN, OUTPUT);
  //while (!Serial);  // no, don't wait for USB-Serial connection to boot
  // instead, just wait a few seconds for PC to catch up if connected:
  // (BUT in this application we'll rarely be connected to a PC, so don't delay)
  //delayMilliseconds(3000);
  Serial.setTimeout(SERIAL_TIMEOUT_MS);
  Serial.println(F("==========> Booting..."));
  Serial.print(F("Firmware version: "));
  Serial.println(VERSION);

  init_lcd(); // NOTE this hangs if LCD not connected
  init_audio();
  init_buttons();
  init_hiscore();

  Serial.println(F("========== ==========> Ready! <========== =========="));
  lcd_write("Ready", NULL);
  digitalWrite(LED_BUILTIN, LOW);
  flash_leds(5, 100);
}


/* ********** ********** ********** Game Show Mode ********** ********** ********** */

[[noreturn]] void gameshow_buzzer() {
  Serial.println(F("===> Game Show Mode <==="));
  lcd_write(" Game Show Mode ", NULL);
  while (true) {
    //TODO: Exit by holding two buttons for >2s?
    while (any_pressed()) check_buttons();  // wait for all buttons to be released
    size_t which=0;
    while (!which) if (check_buttons()) which=any_pressed();  // wait for a button to be pressed
    // react to the button press
    which--;  // any_pressed returns the index+1
    digitalWrite(LEDS[which], HIGH);
    play_audio();
    Serial.print(F("Button "));
    Serial.println(BTN_NAMES[which]);
    char btnstr[] = " => Button _ <= ";
    btnstr[11] = BTN_NAMES[which];
    lcd_write(" Game Show Mode ", btnstr);
    delayMilliseconds(3000);
    digitalWrite(LEDS[which], LOW);
  }
}


/* ********** ********** ********** Reaction Game ********** ********** ********** */

const unsigned long GAME_MAX_DELAY_MS = 1000;  // should be 1000-2000
unsigned long cur_high_score = ULONG_MAX;
size_t prev_game_length = 0;

//TODO Later: consider a reaction game with only 4 buttons?

void reaction_game(const size_t game_length) {
  Serial.println(F("===> Reaction Game <==="));
  lcd_write(" Reaction Game  ", " in progress... ");
  if ( game_length>prev_game_length ) cur_high_score = ULONG_MAX;
  prev_game_length = game_length;
  flash_leds(1, GAME_MAX_DELAY_MS);
  randomSeed(micros());
  unsigned long score = 0;
  size_t prev_btn = BUTTONS;
  for (size_t game=0; game<game_length; game++) {
    while (any_pressed()) check_buttons();  // wait for all buttons to be released
    const size_t expect = random(BUTTONS);
    delayMilliseconds( expect==prev_btn ? random(500, GAME_MAX_DELAY_MS) : random(GAME_MAX_DELAY_MS) );
    size_t pressed=0;
    const unsigned long start_millis = millis();
    digitalWrite(LEDS[expect], HIGH);
    while (!pressed) if (check_buttons()) pressed=any_pressed();  // wait for a button to be pressed
    const unsigned long time_ms = millis() - start_millis;
    pressed--;  // any_pressed returns the index+1
    digitalWrite(LEDS[expect], LOW);
    if ( pressed==expect ) {
      score += time_ms;
      Serial.print(game+1);
      Serial.print(F(" of "));
      Serial.print(game_length);
      Serial.print(F(": "));
      Serial.print(time_ms);
      Serial.println("ms");
    }
    else {  // wrong button pressed, game failed
      score = ULONG_MAX;
      break;
    }
    prev_btn = expect;
  }

  Serial.print(F("Total score (lower is better): "));
  Serial.println(score);

  char dispscore[] = "                ";
  // unsigned long range is 0-4294967295 (10 chars)
  snprintf(dispscore, 17, "%10u ms", score);
  lcd_write( score < ee_high_score ? "!! HIGH SCORE !!" :
    ( score < cur_high_score ? "   Good Score!  " : "   Your score:  " ) , dispscore);

  if ( score < ee_high_score ) {
    ee_high_score = score;
    save_hiscore();
    play_audio();
    flash_leds(15, 50);
  }
  else if ( score < cur_high_score ) {
    cur_high_score = score;
    //TODO Later: For some reason the audio doesn't play the first time after boot...?
    // (it's not too bad because the user always gets a "Good Score!" message anyway...)
    play_audio();
    flash_leds(5, 150);
  }
  else {
    flash_leds(3, 250);
  }

  if ( score<ULONG_MAX )
    do_menu(MENUFLAG_R);
}


/* ********** ********** ********** Debug Loop ********** ********** ********** */

[[noreturn]] void debug_loop() {
  Serial.println(F("===> Debug Mode <==="));
  lcd_write("   Debug Mode   ", VERSION.c_str());
  while (true) {
    if ( check_buttons() ) {
#ifdef POLL_TIME_TEST
      Serial.print("Poll interv. ");
      Serial.print(cur_poll_interval_us);
      Serial.println("us");
#endif
      for(size_t i=0; i<BUTTONS; i++) {
        digitalWrite(LEDS[i], btn_cur_state[i] ? HIGH : LOW);
        if (btn_cur_state[i]) {
          Serial.print(F("Button "));
          Serial.println(BTN_NAMES[i]);
        }
      }
    }
  }
}


/* ********** ********** ********** Main Menu ********** ********** ********** */

void loop() {
  Serial.println(F("Showing Menu"));
  lcd_write("R: Reaction Game", "G: Game Show  Y>");
  uint8_t choice = do_menu(MENUFLAG_R|MENUFLAG_G|MENUFLAG_Y);
  if ( choice & MENUFLAG_R ) {  // "R: Reaction Game"
    lcd_write("R:Short/8   G:32", "B: Long/64    <Y");
    choice = do_menu(MENUFLAG_R|MENUFLAG_G|MENUFLAG_B|MENUFLAG_Y);
    if ( choice & MENUFLAG_R ) reaction_game(8);
    else if ( choice & MENUFLAG_G ) reaction_game(32);
    else if ( choice & MENUFLAG_B ) reaction_game(64);
  }
  else if ( choice & MENUFLAG_G )  // "G: Game Show"
    gameshow_buzzer();  // noreturn
  else if ( choice & MENUFLAG_Y ) {  // "Y>"
    lcd_write("R: High Score   ", "B: Debug      <Y");
    choice = do_menu(MENUFLAG_R|MENUFLAG_B|MENUFLAG_Y);
    if ( choice & MENUFLAG_R ) {  // "R: High Score"
      char dispscore[] = "                ";
      snprintf(dispscore, 17, "%u ms", ee_high_score);
      lcd_write(dispscore, "B: Clear      <Y");
      choice = do_menu(MENUFLAG_B|MENUFLAG_Y);
      if ( choice & MENUFLAG_B ) {  // "B: Clear"
        lcd_write("Are you sure?   ", "G: Yes     Y: No");
        choice = do_menu(MENUFLAG_G|MENUFLAG_Y);
        if ( choice & MENUFLAG_G ) {  // "G: Yes"
          ee_high_score = ULONG_MAX;
          save_hiscore();
        }
      }
    }
    else if ( choice & MENUFLAG_B )  // "B: Debug"
      debug_loop();  // noreturn
  }
}
