///
/// \file  TestSI4703.ino
/// \brief An Arduino sketch to operate a SI4703 chip based radio using the Radio library.
///
/// \author Matthias Hertel, http://www.mathertel.de
/// \copyright Copyright (c) 2014 by Matthias Hertel.\n
/// This work is licensed under a BSD style license.\n
/// See http://www.mathertel.de/License.aspx
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
/// The SI4703 board has to be connected by using the following connections:
/// | Arduino UNO pin    | ESP8266 | ESP32   | Radio chip signal |
/// | -------------------| ------- | ------- | ----------------- |
/// | 3.3V (red)         | 3v3     | 3v3     | VCC               |
/// | GND (black)        | GND     | GND     | GND               |
/// | A5 or SCL (yellow) | D1      |         | SCLK              |
/// | A4 or SDA (blue)   | D2      |         | SDIO              |
/// | D2 (white)         | D5      |         | RST               |
/// More documentation and source code is available at http://www.mathertel.de/Arduino
///
/// CHangeLog:
/// ----------
/// * 05.08.2014 created.
/// * 03.05.2015 corrections and documentation.
/// * 17.01.2023 use radio.setup to configure chip specific features.

#include <Arduino.h>
#include <Wire.h>
#include <radio.h>
#include <si4703.h>

// ----- Fixed settings here. -----

#define FIX_BAND RADIO_BAND_FM  ///< The band that will be tuned by this sketch is FM.
#define FIX_STATION 8930        ///< The station that will be tuned by this sketch is 89.30 MHz.
#define FIX_VOLUME 8            ///< The volume that will be set by this sketch is level 4.

SI4703 radio;  // Create an instance of Class for Si4703 Chip

/// Setup a FM only radio configuration
/// with some debugging on the Serial port
void setup() {
  delay(3000);

  // open the Serial port
  Serial.begin(115200);
  Serial.println("Radio::TestSI4703...");
  delay(200);

  // Enable information to the Serial port
  radio.debugEnable(true);
  radio._wireDebug(true);

#if defined(ARDUINO_ARCH_AVR)
  Wire.begin();  // a common pins for I2C = SDA:A4, SCL:A5
  radio.setup(RADIO_RESETPIN, 2);
  radio.setup(RADIO_SDAPIN, A4);

#elif defined(ESP8266)
  // For ESP8266 boards (like NodeMCU) the I2C GPIO pins in use
  // need to be specified.
  Wire.begin(D2, D1);  // a common GPIO pin setting for I2C
  radio.setup(RADIO_RESETPIN, D5);
  radio.setup(RADIO_SDAPIN, D2);

#elif defined(ESP32)
  Wire.begin();  // a common GPIO pin setting for I2C = SDA:21, SCL:22

#endif

  // Initialize the Radio
  radio.initWire(Wire);

  radio.debugEnable(true);
  radio._wireDebug(true);


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
