///
/// \file  TransmitSI4721.ino
/// \brief An Arduino sketch to operate a SI4721 chip in transmit mode using the Radio library.
///
/// \author N Poole, nickpoole.me
/// \author Matthias Hertel, http://www.mathertel.de
/// \copyright Copyright (c) 2014 by Matthias Hertel.\n
/// This work is licensed under a BSD 3-Clause License. See http://www.mathertel.de/License.aspx
///
/// \details
/// This sketch implements FM transmit mode with RDS broadcasting. \n
/// The radio chip is initialized and setup to a fixed band and frequency. These settings can be changed by modifying the
/// FIX_BAND and FIX_STATION definitions.
///
/// Open the Serial console with 115200 baud to see the current radio information.
///
/// Wiring
/// ------
/// The SI4703 board/chip has to be connected by using the following connections:
/// | Arduino UNO pin | Radio chip     |
/// | --------------- | ---------------|
/// | 3.3V            | VCC            |
/// | GND             | GND            |
/// | A5 or SCL       | SCLK           |
/// | A4 or SDA       | SDIO           |
/// |                 | RST (not used) |
/// The locations of the pins on the UNO board are written on the PCB.
/// The locations of the signals on the SI4721 side depend on the board you use.
///
/// More documentation and source code is available at http://www.mathertel.de/Arduino
///
/// ChangeLog:
/// ----------
/// * 05.12.2019 created.
/// * 20.09.2020 Integrated into radio library version 2.0.0

#include <Arduino.h>
#include <Wire.h>
#include <radio.h>
#include <si4721.h>

// ----- Fixed settings here. -----

#define FIX_BAND RADIO_BAND_FMTX ///< The band that will be tuned by this sketch is FM.
#define FIX_STATION 10410 ///< The station that will be tuned by this sketch is 104.10 MHz.
#define FIX_POWER 90 ///< The transmit output power that will be set by this sketch.

#define RDS_SERVICENAME "Testing" ///< The rds service name that will be set by this sketch is "Testing"
#define RDS_TEXTBUFFER "Hello World!" ///< The rds text buffer that will be set by this sketch is "Hello World!"

SI4721 transmitter; // Create an instance of Class for SI4721 Chip

/// Setup a FM only radio configuration
/// with some debugging on the Serial port
void setup()
{
  delay(3000);

  // open the Serial port
  Serial.begin(115200);
  Serial.println("Radio Initialize...");
  delay(200);

#ifdef ESP8266
  // For ESP8266 boards (like NodeMCU) the I2C GPIO pins in use
  // need to be specified.
  Wire.begin(D2, D1); // a common GPIO pin setting for I2C
#endif

  // see if a chip can be found
  if (transmitter._wireExists(&Wire, SI4721_ADR)) {
    Serial.print("Device found at address ");
    Serial.println(SI4721_ADR);
  } else {
    Serial.print("Device NOT found at address ");
    Serial.println(SI4721_ADR);
  }

  // Enable information to the Serial port
  transmitter.debugEnable(false);
  transmitter._wireDebug(false);

  // Initialize the Radio
  transmitter.init();
  // transmitter.setDeemphasis(75); // Un-comment this line in the USA to set correct deemphasis/preemphasis timing

  transmitter.setBandFrequency(FIX_BAND, FIX_STATION);
  transmitter.setTXPower(FIX_POWER);

  transmitter.beginRDS();
  transmitter.setRDSstation(RDS_SERVICENAME);
  transmitter.setRDSbuffer(RDS_TEXTBUFFER);
} // setup


/// show the current chip data every 3 seconds.
void loop()
{
  Serial.print("Transmitting on ");
  Serial.print(transmitter.getTuneStatus().frequency);
  Serial.println("kHz...");

  Serial.print("ASQ: ");
  Serial.println(transmitter.getASQ().asq);

  Serial.print("Audio In Level: ");
  Serial.println(transmitter.getASQ().audioInLevel);

  delay(3000);
} // loop

// End.