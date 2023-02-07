/// \file LCDKeypadRadio.ino
/// \brief Radio implementation using the Serial communication.
///
/// \author Matthias Hertel, http://www.mathertel.de
/// \copyright Copyright (c) 2014 by Matthias Hertel.\n
/// This work is licensed under a BSD style license.\n
/// See http://www.mathertel.de/License.aspx
///
/// \details
/// This is a full function radio implementation that uses a LCD display to show the current station information.\n
/// It can be used with various chips after adjusting the radio object definition.\n
/// Open the Serial console with 115200 baud to see current radio information and change various settings.
///
/// Wiring
/// ------
/// The necessary wiring of the various chips are described in the Testxxx example sketches.
/// The boards have to be connected e. g. by using the following connections:
///
/// Arduino port | SI4703 signal | RDA5807M signal
/// :----------: | :-----------: | :-------------:
///          GND | GND           | GND
///         3.3V | VCC           | VCC
///           A5 | SCLK          | SCLK
///           A4 | SDIO          | SDIO
///           D2 | RST           | -
///
/// More documentation is available at http://www.mathertel.de/Arduino
/// Source Code is available on https://github.com/mathertel/Radio
///
/// History:
/// --------
/// * 05.08.2014 created.
/// * 04.10.2014 working.
/// * 22.03.2015 Copying to LCDKeypadRadio.
/// * 23.01.2023 more robust, nicer LCD messages and cleanup.
/// * 30.01.2023 avoid using analogRead() too often.

#include <LiquidCrystal.h>

#include <Wire.h>

#include <radio.h>
#include <RDA5807M.h>
#include <SI4703.h>
#include <SI4705.h>
#include <SI47xx.h>
#include <TEA5767.h>

#include <RDSParser.h>

// Define some stations available at your locations here:
// 89.30 MHz as 8930

RADIO_FREQ preset[] = {
  8930,   // Sender:<  hr3   >
  9060,   // Sender:<  hr1   >
  9310,   //
  9340,   // Sender:<BAYERN 3>
  9440,   // Sender:<  hr1   >
  9530,   // Sender:< YOU FM >
  9670,   // Sender:<  hr2   >
  9870,   // Sender:<  Dlf   >
  10020,  // Sender:< planet >
  10140,  // Sender:<RADIOBOB>
  10160,  // Sender:<  hr4   >
  10300   // Sender:<ANTENNE >
};

uint16_t presetIndex = 0;  ///< Start at Station with index = 1


// ===== SI4703 specific pin wiring =====
// #define ENABLE_SI4703

#ifdef ENABLE_SI4703
#if defined(ARDUINO_ARCH_AVR)
#define RESET_PIN 2
#define MODE_PIN A4  // same as SDA

#elif defined(ESP8266)
#define RESET_PIN D5
#define MODE_PIN D2  // same as SDA

#elif defined(ESP32)
#define RESET_PIN 4
#define MODE_PIN 21  // same as SDA

#endif
#endif


// The keys available on the keypad
enum KEYSTATE {
  KEYSTATE_NONE,
  KEYSTATE_SELECT,
  KEYSTATE_LEFT,
  KEYSTATE_UP,
  KEYSTATE_DOWN,
  KEYSTATE_RIGHT
} __attribute__((packed));


// ----- forwards -----
// You need this because the function is not returning a simple value.
KEYSTATE getLCDKeypadKey();

/// The radio object has to be defined by using the class corresponding to the used chip.
/// by uncommenting the right radio object definition.

// RADIO radio;     ///< Create an instance of a non functional radio.
RDA5807M radio;  ///< Create an instance of a RDA5807 chip radio
// SI4703   radio;  ///< Create an instance of a SI4703 chip radio.
// SI4705   radio;  ///< Create an instance of a SI4705 chip radio.
// SI47xx radio;    ///< Create an instance of a SI4721 chip radio.
// TEA5767  radio;  ///< Create an instance of a TEA5767 chip radio.


/// get a RDS parser
RDSParser rds;

unsigned long nextFreqTime = 0;
unsigned long nextKeyTime = 0;
unsigned long nextClearTime = 0;

bool doReportKey = false;
KEYSTATE lastKey = KEYSTATE_NONE;


// - - - - - - - - - - - - - - - - - - - - - - - - - -


// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);


/// This function will be when a new frequency is received.
/// Update the Frequency on the LCD display.
void DisplayFrequency() {
  char s[12];
  radio.formatFrequency(s, sizeof(s));
  lcd.setCursor(0, 0);
  lcd.print(s);
}  // DisplayFrequency()


/// This function will be called by the RDS module when a new ServiceName is available.
/// Update the LCD to display the ServiceName in row 1 chars 0 to 7.
void DisplayServiceName(const char *name) {
  size_t len = strlen(name);
  lcd.setCursor(0, 1);
  lcd.print(name);
  while (len < 8) {
    lcd.print(' ');
    len++;
  }  // while
}  // DisplayServiceName()


/// This function will be called by the RDS module when a rds time message was received.
/// Update the LCD to display the time in right upper corner.
void DisplayTime(uint8_t hour, uint8_t minute) {
  lcd.setCursor(11, 0);
  if (hour < 10) lcd.print('0');
  lcd.print(hour);
  lcd.print(':');
  if (minute < 10) lcd.print('0');
  lcd.println(minute);
}  // DisplayTime()


// - - - - - - - - - - - - - - - - - - - - - - - - - -


void RDS_process(uint16_t block1, uint16_t block2, uint16_t block3, uint16_t block4) {
  rds.processData(block1, block2, block3, block4);
}


/// This function determines the current pressed key but
/// * doesn't return short key down situations that are key signal bouncing.
/// * returns a specific key down only once.
///
/// [Next] [seek--] [Vol+] [seek++] [RST]
///                 [Vol-]
///
KEYSTATE getLCDKeypadKey() {
  KEYSTATE newKey = KEYSTATE_NONE;

  // Get current key state
  int v = analogRead(A0);  // the buttons are read from the analog0 pin

  if (v < 100) {
    newKey = KEYSTATE_RIGHT;
  } else if (v < 200) {
    newKey = KEYSTATE_UP;
  } else if (v < 400) {
    newKey = KEYSTATE_DOWN;
  } else if (v < 600) {
    newKey = KEYSTATE_LEFT;
  } else if (v < 800) {
    newKey = KEYSTATE_SELECT;
  } else {
    newKey = KEYSTATE_NONE;
  }

  if (newKey != lastKey) {
    // a new situation - just remember but do not return anything pressed.
    doReportKey = true;
    lastKey = newKey;
    return (KEYSTATE_NONE);

  } else if ((newKey == lastKey) && (doReportKey)) {
    doReportKey = false;  // don't report twice
    return (newKey);
  }
  return (KEYSTATE_NONE);
}  // getLCDKeypadKey()


void displayKeyValue(const char *key, int value) {
  lcd.setCursor(0, 1);
  lcd.print(key);
  lcd.print(": ");
  lcd.print(value);
  lcd.print("  ");
}  // displayKeyValue


/// Setup a FM only radio configuration with I/O for commands and debugging on the Serial port.
void setup() {
  delay(1000);
  // open the Serial port
  Serial.begin(115200);
  Serial.println("LCDKeypadRadio...");

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("LCDKeypadRadio");
  delay(2000);
  lcd.clear();

#if defined(RESET_PIN)
  // This is required for SI4703 chips:
  radio.setup(RADIO_RESETPIN, RESET_PIN);
  radio.setup(RADIO_MODEPIN, MODE_PIN);
#endif

  // Initialize the Radio
  bool found = radio.initWire(Wire);

  if (!found) {
    Serial.println("No Radio Chip found.");
    Serial.println("Press reset...");

    lcd.setCursor(0, 0);
    lcd.print("No Radio Chip");
    lcd.setCursor(0, 1);
    lcd.print("Press reset...");

    while (1) {};  // forever
  }

  // Enable information to the Serial port
  radio.debugEnable();

  radio.setBandFrequency(RADIO_BAND_FM, preset[presetIndex]);  // first preset.

  radio.setMono(false);
  radio.setMute(false);
  radio.setVolume(radio.getMaxVolume());

  // setup the information chain for RDS data.
  radio.attachReceiveRDS(RDS_process);
  rds.attachServiceNameCallback(DisplayServiceName);
  rds.attachTimeCallback(DisplayTime);
}  // Setup


/// Constantly check for keypad  and trigger command execution.
void loop() {
  unsigned long now = millis();

  // check for RDS data
  radio.checkRDS();

  // some internal static values for parsing the input
  static RADIO_FREQ lastFrequency = 0;
  RADIO_FREQ f = 0;

  // detect any key press on LCD Keypad Module
  if (now > nextKeyTime) {
    nextKeyTime = now + 100;

    KEYSTATE k = getLCDKeypadKey();

    if (k == KEYSTATE_RIGHT) {
      radio.seekUp(true);

    } else if (k == KEYSTATE_UP) {
      // increase volume
      int v = radio.getVolume();
      if (v < radio.getMaxVolume()) radio.setVolume(++v);
      displayKeyValue("vol", v);
      nextClearTime = now + 2000;

    } else if (k == KEYSTATE_DOWN) {
      // decrease volume
      int v = radio.getVolume();
      if (v > 0) radio.setVolume(--v);
      displayKeyValue("vol", v);
      nextClearTime = now + 2000;

    } else if (k == KEYSTATE_LEFT) {
      radio.seekDown(true);

    } else if (k == KEYSTATE_SELECT) {
      if (presetIndex < (sizeof(preset) / sizeof(RADIO_FREQ)) - 1) {
        presetIndex++;
      } else {
        presetIndex = 0;
      }

      radio.setFrequency(preset[presetIndex]);
      nextClearTime = now;
    }
  }  // if

  // update the display from time to time
  if (now > nextFreqTime) {
    f = radio.getFrequency();
    if (f != lastFrequency) {
      // print current tuned frequency
      DisplayFrequency();
      lastFrequency = f;
    }  // if
    nextFreqTime = now + 400;
  }  // if

  // clear 2. line of display
  if ((nextClearTime) && (now > nextClearTime)) {
    lcd.setCursor(0, 1);
    lcd.print("                ");
    nextClearTime = 0;
  }  // if
}  // loop

// End.
