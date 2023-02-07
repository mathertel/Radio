///
/// \file  TestSI4721.ino
/// \brief An Arduino sketch to operate a SI4720 and SI4721 chip based radio using the Radio library.
///
/// \author N Poole, nickpoole.me
/// \author Matthias Hertel, http://www.mathertel.de
/// \copyright Copyright (c) 2014 by Matthias Hertel.\n
/// This work is licensed under a BSD 3-Clause License. See http://www.mathertel.de/License.aspx
///
/// \details
/// This sketch implements a "as simple as possible" radio without any possibility to modify the settings after initializing the chip.\n
/// The radio chip is initialized and setup to a fixed band and frequency. These settings can be changed by modifying the
/// FIX_BAND and FIX_STATION definitions.
///
/// Open the Serial console with 115200 baud to see the current radio information.
///
/// Wiring
/// ------
/// The SI4720 / SI4721 board/chip has to be connected by using the following connections:

/// | Arduino UNO pin | ESP8266 | ESP32 | Radio chip signal  |
/// | --------------- | ------- | ----- | -------------------|
/// | 3.3V / 5V       | Vin     | V5    | VCC                |
/// | GND             | GND     | GND   | GND                |
/// | A5 or SCL       | D1      | IO22  | SCLK               |
/// | A4 or SDA       | D2      | IO21  | SDIO               |
///
/// More documentation is available at http://www.mathertel.de/Arduino
/// Source Code is available on https://github.com/mathertel/Radio
///
/// ChangeLog:
/// ----------
/// * 05.12.2019 created.
/// * 18.05.2022 property oriented interface adapted.
/// * 15.01.2023 cleanup compiler warnings.

#include <Arduino.h>
#include <Wire.h>

#include <radio.h>
#include <SI47xx.h>

// ----- Fixed settings here. -----

#define FIX_BAND RADIO_BAND_FM  ///< The band that will be tuned by this sketch is FM.
#define FIX_STATION 8930        ///< The station that will be tuned by this sketch is 89.30 MHz.
#define FIX_VOLUME 10           ///< The volume that will be set by this sketch is level 4.

SI47xx radio;  // Create an instance of Class for SI47xx Chip

/// Setup a FM only radio configuration
/// with some debugging on the Serial port
void setup() {
  delay(3000);

  // open the Serial port
  Serial.begin(115200);
  Serial.println("SI47xxRadio...");
  delay(200);

#if defined(ARDUINO_ARCH_AVR)

#elif defined(ESP8266)
  // For ESP8266 boards (like NodeMCU) the I2C GPIO pins in use
  // need to be specified.
  Wire.begin(D2, D1);  // a common GPIO pin setting for I2C

#elif defined(ESP32)
  Wire.begin();  // a common GPIO pin setting for I2C = SDA:21, SCL:22

#endif

  // see if a chip can be found
  if (radio._wireExists(&Wire, 0x11)) {
    Serial.println("Device found at address 0x11");
  } else if (radio._wireExists(&Wire, 0x61)) {
    Serial.println("Device found at address 0x61");
  } else if (radio._wireExists(&Wire, 0x63)) {
    Serial.println("Device found at address 0x63");
  } else {
    Serial.println("Device NOT found at any address ");
  }

  Wire.begin();  // a common GPIO pin setting for I2C = SDA:21, SCL:22

  // Enable debug information to the Serial port
  radio.debugEnable(true);
  radio._wireDebug(false);

  // Set FM Options for Europe
  radio.setup(RADIO_FMSPACING, RADIO_FMSPACING_100);   // for EUROPE
  radio.setup(RADIO_DEEMPHASIS, RADIO_DEEMPHASIS_50);  // for EUROPE

  // Initialize the Radio
  radio.initWire(Wire);

  // radio.setDeemphasis(75); // Un-comment this line for USA

  // Set all radio setting to the fixed values.
  radio.setBandFrequency(FIX_BAND, FIX_STATION);
  radio.setVolume(FIX_VOLUME);
  radio.setMono(true);
  radio.setMute(false);

  radio.setup(RADIO_ANTENNA, RADIO_ANTENNA_OPT1);
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
