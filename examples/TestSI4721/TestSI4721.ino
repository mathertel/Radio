///
/// \file  TestSI4721.ino
/// \brief An Arduino sketch to operate a SI4721 chip based radio using the Radio library.
///
/// \author N Poole, nickpoole.me
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
/// The SI4703 board/chip has to be connected by using the following connections:
/// | Arduino UNO pin | Radio chip signal  | 
/// | --------------- | -------------------| 
/// | 3.3V            | VCC                | 
/// | GND             | GND                | 
/// | A5 or SCL       | SCLK               | 
/// | A4 or SDA       | SDIO               | 
/// |                 | RST (not used)     | 
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
#include <RDSParser.h>

// ----- Fixed settings here. -----

#define FIX_BAND     RADIO_BAND_FM   ///< The band that will be tuned by this sketch is FM.
#define FIX_STATION  8930            ///< The station that will be tuned by this sketch is 89.30 MHz.
#define FIX_VOLUME   15               ///< The volume that will be set by this sketch is level 15.

SI4721 radio;    // Create an instance of Class for SI4705 Chip

RDSParser rds;	// get a RDS parser

/// Setup a FM only radio configuration
/// with some debugging on the Serial port
void setup() {
  // open the Serial port
  Serial.begin(57600);
  Serial.println("Radio Initialize...");
  delay(200);

  // Initialize the Radio 
  // radio.setDeemphasis(75); // Un-comment this line in the USA to set correct deemphasis timing
  radio.init();

  // Enable information to the Serial port
  radio.debugEnable();

  // Set all radio setting to the fixed values.
  radio.setBandFrequency(FIX_BAND, FIX_STATION);
  radio.setVolume(FIX_VOLUME);

  // Set RDS settings
  radio.attachReceiveRDS(RDS_process);
  rds.attachTextCallback(DisplayText);	// function to call when valid RDS Text data is received
  rds.attachServicenNameCallback(DisplayName);	// function to call when valid RDS Service Name data is received  
} // setup


/// show the current chip data every 3 seconds and check RDS for valid data.
void loop() {
  char s[12];
  radio.formatFrequency(s, sizeof(s));
  Serial.print("Station:"); 
  Serial.println(s);
  
  Serial.print("Radio:"); 
  radio.debugRadioInfo();
  
  Serial.print("Audio:"); 
  radio.debugAudioInfo();
  
  radio.checkRDS();

  delay(3000);
} // loop

/// when valid RDS Text is received, print it
void DisplayText(char *text)
{
  Serial.print("RDS Text:"); Serial.println(text);
}

/// when a valid RDS Service Name is received, print it
void DisplayName(char *name)
{
  Serial.print("RDS Name:"); Serial.println(name);
}

/// process incoming RDS data
void RDS_process(uint16_t block1, uint16_t block2, uint16_t block3, uint16_t block4) {
  rds.processData(block1, block2, block3, block4);
}

// End.
