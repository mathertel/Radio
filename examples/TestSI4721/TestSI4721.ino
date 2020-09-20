///
/// \file  TestSI4721.ino
/// \brief An Arduino sketch to operate a SI4705 chip based radio using the Radio library.
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
/// The SI4721 board/chip has to be connected by using the following connections:

/// | Arduino UNO pin | Radio chip signal  |
/// | --------------- | -------------------|
/// | 3.3V / 5V       | VCC                |
/// | GND             | GND                |
/// | A5 or SCL       | SCLK               |
/// | A4 or SDA       | SDIO               |
///
/// More documentation and source code is available at http://www.mathertel.de/Arduino
///
/// ChangeLog:
/// ----------
/// * 05.12.2019 created.

#include <Arduino.h>
#include <Wire.h>
#include <radio.h>

#include <si4721.h>

// ----- Fixed settings here. -----

#define FIX_BAND RADIO_BAND_FM ///< The band that will be tuned by this sketch is FM.
#define FIX_STATION 8930 ///< The station that will be tuned by this sketch is 89.30 MHz.
#define FIX_VOLUME 10 ///< The volume that will be set by this sketch is level 4.

SI4721 radio; // Create an instance of Class for SI4705 Chip

/// Setup a FM only radio configuration
/// with some debugging on the Serial port
void setup()
{
  delay(3000);

  // open the Serial port
  Serial.begin(115200);
  Serial.println("Radio...");
  delay(200);

#ifdef ESP8266
  // For ESP8266 boards (like NodeMCU) the I2C GPIO pins in use
  // need to be specified.
  Wire.begin(D2, D1); // a common GPIO pin setting for I2C
#endif

  // see if a chip can be found
  if (radio._wireExists(&Wire, SI4721_ADR)) {
    Serial.print("Device found at address ");
    Serial.println(SI4721_ADR);
  } else {
    Serial.print("Device NOT found at address ");
    Serial.println(SI4721_ADR);
  }

  // Enable debug information to the Serial port
  radio.debugEnable(false);
  radio._wireDebug(false);

  // Initialize the Radio
  radio.init(Wire, SI4721_ADR);

  // radio.setDeemphasis(75); // Un-comment this line in the USA

  // Set all radio setting to the fixed values.
  radio.setBandFrequency(FIX_BAND, FIX_STATION);
  radio.setVolume(FIX_VOLUME);
  radio.setMono(true);
  radio.setMute(false);
} // setup


/// show the current chip data every 3 seconds.
void loop()
{
  char s[12];
  radio.formatFrequency(s, sizeof(s));
  Serial.print("Station:");
  Serial.println(s);

  Serial.print("Radio:");
  radio.debugRadioInfo();

  Serial.print("Audio:");
  radio.debugAudioInfo();

  delay(3000);
} // loop

// End.
