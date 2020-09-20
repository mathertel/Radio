///
/// \file  PairTestSI4721.ino
/// \brief An Arduino sketch to operate two SI4721 chip based radios as a tx/rx pair using the Radio library.
///
/// \author N Poole, nickpoole.me
/// \author Matthias Hertel, http://www.mathertel.de
/// \copyright Copyright (c) 2014 by Matthias Hertel.\n
/// This work is licensed under a BSD style license. See http://www.mathertel.de/License.aspx
///
/// \details
/// This sketch is designed to test a transmitter and receiver pair of SI4721 devices on the same i2c bus. \n
/// The radio chips are initialized and setup to a fixed band and frequency. These settings can be changed by modifying the 
/// FIX_BAND and FIX_STATION definitions. 
///
/// Open the Serial console with 57600 baud to see the current radio information.
///
/// Wiring
/// ------ 
/// The SI4703 board/chip has to be connected by using the following connections:
/// | Arduino UNO pin | Radio chip 1 signal | Radio chip 2 signal | 
/// | --------------- | ------------------- | ------------------- | 
/// | 3.3V            | VCC                 | VCC                 | 
/// | GND             | GND                 | GND                 | 
/// | A5 or SCL       | SCLK                | SCLK                | 
/// | A4 or SDA       | SDIO                | SDIO                | 
/// |                 | RST (not used)      | RST (not used)      |
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

#define FIX_BAND     RADIO_BAND_FM   	///< The band that will be tuned by this sketch is FM.
#define FIX_STATION  8930            	///< The station that will be tuned by this sketch is 89.30 MHz.
#define FIX_VOLUME   15               	///< The volume that will be set by this sketch on the receiver is level 15.
#define RDS_SERVICENAME "Testing"		///< The rds service name that will be set by this sketch on the transmitter is "Testing"
#define RDS_TEXTBUFFER "Hello World!"	///< The rds text buffer that will be set by this sketch on the transmitter is "Hello World!"

SI4721 receiver, transmitter;    // Create two new instances of Class for SI4721 Chips

RDSParser rds; // get a RDS parser for the receiver

/// Setup two SI4721 radio devices on separate i2c 
/// addresses. Switch one to transmit mode and tune 
/// them to the same station.
void setup() {
  // open the Serial port
  Serial.begin(57600);
  Serial.println("Radios Initialize...");
  delay(200);

  // Initialize transmitter with SI4721's alternate i2c address (0x11) and enter transmit mode
  // transmitter.setDeemphasis(75); // Un-comment this line in the USA to set correct deemphasis/preemphasis timing
  transmitter.init(Wire, 0x11); // NOTE: Target board must have alternate address set in hardware
  transmitter.setModeTransmit();
  transmitter.setTXpower(115); // Output power of the device in dBµV (valid range is 88 to 115)
  transmitter.setBandFrequency(FIX_BAND, FIX_STATION);
  transmitter.beginRDS();
  transmitter.setRDSstation(RDS_SERVICENAME);
  transmitter.setRDSbuffer(RDS_TEXTBUFFER);  

  // Initialize receiver with default i2c address
  // Radio initializes chips in receive mode so there's no need to call setModeReceive()
  // receiver.setDeemphasis(75); // Un-comment this line in the USA to set correct deemphasis/preemphasis timing
  receiver.init();
  receiver.setBandFrequency(FIX_BAND, FIX_STATION);
  receiver.attachReceiveRDS(RDS_process);
  rds.attachTextCallback(DisplayText);	// function to call when valid RDS Text data is received
  rds.attachServicenNameCallback(DisplayName);	// function to call when valid RDS Service Name data is received  
  
} // setup


/// show the current chip data every 3 seconds. 
/// Check for valid RDS data every 500ms.
void loop() {

  for(uint8_t i = 0; i < 6; i++){ 
	receiver.checkRDS();
	delay(500);
  }

  char s[12];
  receiver.formatFrequency(s, sizeof(s));
  Serial.print("Receiving on "); 
  Serial.println(s);

  Serial.print("Transmitting on ");
  Serial.print(transmitter.getTuneStatus().frequency);
  Serial.println("kHz...");
  
  Serial.print("ASQ: ");
  Serial.println(transmitter.getASQ().asq);

  Serial.print("Audio In Level: ");
  Serial.println(transmitter.getASQ().audioInLevel);

}
 // loop

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