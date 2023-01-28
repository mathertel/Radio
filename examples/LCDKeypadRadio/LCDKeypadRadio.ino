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
/// Open the Serial console with 57600 baud to see current radio information and change various settings.
///
/// Wiring
/// ------
/// The necessary wiring of the various chips are described in the Testxxx example sketches.
/// The boards have to be connected by using the following connections:
///
/// Arduino port | SI4703 signal | RDA5807M signal
/// :----------: | :-----------: | :-------------:
///          GND | GND           | GND
///         3.3V | VCC           | VCC
///           A5 | SCLK          | SCLK
///           A4 | SDIO          | SDIO
///           D2 | RST           | -
///
///
/// More documentation and source code is available at http://www.mathertel.de/Arduino
///
/// History:
/// --------
/// * 05.08.2014 created.
/// * 04.10.2014 working.
/// * 22.03.2015 Copying to LCDKeypadRadio.
/// * 23.01.2023 more robust, nicer LCD messages and cleanup.

#include <LiquidCrystal.h>

#include <Wire.h>

#include <radio.h>
#include <rda5807M.h>
#include <si4703.h>
#include <si4705.h>
#include <SI47xx.h>
#include <tea5767.h>

#include <RDSParser.h>

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



// Define some stations available at your locations here:
// 89.30 MHz as 8930
// Be sure to enter a strong station in preset[0];

RADIO_FREQ preset[] = {
  8930,  // * hr3
  9270,
  9310,
  9670,
  10250
};

unsigned int presetIndex = 0;  ///< Start at Station with index=5

/// The radio object has to be defined by using the class corresponding to the used chip.
/// by uncommenting the right radio object definition.

// RADIO radio;       ///< Create an instance of a non functional radio.
// RDA5807M radio;    ///< Create an instance of a RDA5807 chip radio
// SI4703   radio;    ///< Create an instance of a SI4703 chip radio.
// SI4705   radio;    ///< Create an instance of a SI4705 chip radio.
SI47xx radio;  ///< Create an instance of a SI4721 chip radio.
// TEA5767  radio;    ///< Create an instance of a TEA5767 chip radio.


/// get a RDS parser
RDSParser rds;

unsigned long nextFreqTime = 0;
unsigned long nextClearTime = 0;

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
///              [Vol-]
///
KEYSTATE getLCDKeypadKey() {
  static unsigned long lastChange = 0;
  static KEYSTATE lastKey = KEYSTATE_NONE;

  unsigned long now = millis();
  KEYSTATE newKey;

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
    lastChange = now;
    lastKey = newKey;
    return (KEYSTATE_NONE);

  } else if (lastChange == 0) {
    // nothing new.
    return (KEYSTATE_NONE);
  }
  if (now > lastChange + 50) {
    // now its not a bouncing button any more.
    lastChange = 0;  // don't report twice
    return (newKey);

  } else {
    return (KEYSTATE_NONE);

  }  // if
}  // getLCDKeypadKey()


/// Setup a FM only radio configuration with I/O for commands and debugging on the Serial port.
void setup() {
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

  // Initialize the Radio
  bool found = radio.initWire(Wire);

  if (!found) {
    Serial.println("No Radio Chip found.");
    Serial.println("Press reset...");

    lcd.setCursor(0, 0);
    lcd.print("No Radio Chip");
    lcd.setCursor(0, 1);
    lcd.print("Press reset...");

    while (1) {};
  }

  // Enable information to the Serial port
  radio.debugEnable();

  radio.setBandFrequency(RADIO_BAND_FM, preset[presetIndex]);  // first preset.

  // delay(100);

  radio.setMono(false);
  radio.setMute(false);
  radio.setVolume(radio.getMaxVolume());

  Serial.write('>');

  // setup the information chain for RDS data.
  radio.attachReceiveRDS(RDS_process);
  rds.attachServiceNameCallback(DisplayServiceName);
  rds.attachTimeCallback(DisplayTime);
}  // Setup


/// Constantly check for keypad  and trigger command execution.
void loop() {
  unsigned long now = millis();

  // some internal static values for parsing the input
  static RADIO_FREQ lastFrequency = 0;
  RADIO_FREQ f = 0;

  KEYSTATE k = getLCDKeypadKey();

  if (k == KEYSTATE_RIGHT) {
    radio.seekUp(true);

  } else if (k == KEYSTATE_UP) {
    // increase volume
    int v = radio.getVolume();
    if (v < radio.getMaxVolume()) radio.setVolume(++v);
    lcd.setCursor(0, 1);
    lcd.print("vol:");
    lcd.print(v);
    nextClearTime = now + 2000;

  } else if (k == KEYSTATE_DOWN) {
    // decrease volume
    int v = radio.getVolume();
    if (v > 0) radio.setVolume(--v);
    lcd.setCursor(0, 1);
    lcd.print("vol:");
    lcd.print(v);
    nextClearTime = now + 2000;

  } else if (k == KEYSTATE_LEFT) {
    radio.seekDown(true);

  } else if (k == KEYSTATE_SELECT) {
    // 10110
    if (presetIndex < (sizeof(preset) / sizeof(RADIO_FREQ)) - 1) {
      presetIndex++;
    } else {
      presetIndex = 0;
    }
    radio.setFrequency(preset[presetIndex]);

  } else {
    //
  }

  // check for RDS data
  radio.checkRDS();

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
