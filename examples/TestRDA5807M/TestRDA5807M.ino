///
/// \file  TestRDA5807M.ino
/// \brief An Arduino sketch to operate a SI4705 chip based radio using the Radio library.
///
/// \author Matthias Hertel, http://www.mathertel.de
/// \copyright Copyright (c) 2014 by Matthias Hertel.\n
/// This work is licensed under a BSD style license. See http://www.mathertel.de/License.aspx
///
/// \details
/// This sketch implements a "as simple as possible" radio without any possibility to modify the settings after initializing the chip.\n
/// The radio chip is initialized and setup to a fixed band and frequency. These settings can be changed by modifying the
/// FIX_BAND and FIX_STATION definitions.
///
/// Open the Serial console with 57600 baud to see the current radio information.
///
/// Wiring
/// ------
/// The RDA5807M board/chip has to be connected by using the following connections:
///
/// | Signal       | Arduino UNO | ESP8266 | ESP32  | Radio chip signal |
/// | ------------ | ------------| ------- | ------ | ----------------- |
/// | VCC (red)    | 3.3V        | 3v3     | 3v3    | VCC               |
/// | GND (black)  | GND         | GND     | GND    | GND               |
/// | SCL (yellow) | A5 / SCL    | D1      | 22     | SCLK              |
/// | SDA (blue)   | A4 / SDA    | D2      | 21     | SDIO              |
///
/// The locations of the signals on the RDA5807M side depend on the board you use.
/// More documentation is available at http://www.mathertel.de/Arduino
/// Source Code is available on https://github.com/mathertel/Radio
///
/// ChangeLog:
/// ----------
/// * 05.12.2014 created.
/// * 19.05.2015 extended.

#include <Arduino.h>
#include <Wire.h>

#include <radio.h>
#include <RDA5807M.h>

// ----- Fixed settings here. -----

#define FIX_BAND RADIO_BAND_FM  ///< The band that will be tuned by this sketch is FM.
#define FIX_STATION 8930        ///< The station that will be tuned by this sketch is 89.30 MHz.
#define FIX_VOLUME 10           ///< The volume that will be set by this sketch is level 4.

RDA5807M radio;  // Create an instance of Class for RDA5807M Chip

/// Setup a FM only radio configuration
/// with some debugging on the Serial port
void setup() {
  delay(3000);
  // open the Serial port
  Serial.begin(115200);
  Serial.println("RDA5807M Radio...");
  delay(200);

  // Enable information to the Serial port
  radio.debugEnable(true);
  radio._wireDebug(false);

  // Set FM Options for Europe
  radio.setup(RADIO_FMSPACING, RADIO_FMSPACING_100);   // for EUROPE
  radio.setup(RADIO_DEEMPHASIS, RADIO_DEEMPHASIS_50);  // for EUROPE

  // Initialize the Radio
  if (!radio.initWire(Wire)) {
    Serial.println("no radio chip found.");
    delay(4000);
    ESP.restart();
  };

  // Set all radio setting to the fixed values.
  radio.setBandFrequency(FIX_BAND, FIX_STATION);
  radio.setVolume(FIX_VOLUME);
  radio.setMono(false);
  radio.setMute(false);
}  // setup


/// show the current chip data every 3 seconds.
void loop() {
  char s[12];
  radio.formatFrequency(s, sizeof(s));
  Serial.print("Station:");
  Serial.println(s);

  Serial.print("Radio:");
  radio.debugRadioInfo();

  Serial.print("Audio:");
  radio.debugAudioInfo();

  delay(3000);
}  // loop

// End.
