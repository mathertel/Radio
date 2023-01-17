  ///
/// \file  TestRDA5807FP.ino
/// \brief An Arduino sketch to operate a RDA5807FP chip based radio using the Radio library.
///
/// \author Marcus Degenkolbe, http://www.degenkolbe.eu
/// \copyright Copyright (c) 2023 by Marcus Degenkolbe.\n
/// This work is licensed under a BSD style license. See http://www.mathertel.de/License.aspx
///
/// \details
/// This sketch implements a simple radio without any possibility to modify the settings after initializing the chip.\n
/// The radio chip is initialized and setup to a fixed band and frequency. These settings can be changed by modifying the 
/// FIX_BAND and FIX_STATION definitions. The chip sends the output data via I2S to a compatible I2S DAC.
///
/// Open the Serial console with 57600 baud to see the current radio information.
///
/// Wiring
/// ------ 
/// The RDA5807FP board/chip has to be connected by using the following connections:
/// | Arduino UNO pin    | Radio chip signal  | UDA1334A I2S DAC Board |
/// | -------------------| -------------------| -----------------------|
/// | 5V/VIN (red)       |                    | VIN                    |
/// | 3.3V (red)         | VCC                |                        |
/// | GND (black)        | GND                | GND                    |
/// | A5 or SCL (yellow) | SCLK               |                        |
/// | A4 or SDA (blue)   | SDIO               |                        |
/// |                    | GPIO1 (WS)         | WS                     |
/// |                    | GPIO2 (SD/DOUT)    | DIN                    |
/// |                    | GPIO3 (SCK/BCLK)   | BCLK                   |
/// The locations of the pins on the UNO board are written on the PCB.
/// The locations of the signals on the RDA5807FP side depend on the board you use.
///
/// More documentation and source code is available at http://www.mathertel.de/Arduino
///
/// ChangeLog:
/// ----------
/// * 12.01.2023 created.

#include <Arduino.h>
#include <Wire.h>
#include <radio.h>
#include <RDA5807FP.h>

// ----- Fixed settings here. -----

#define FIX_BAND     RADIO_BAND_FM   ///< The band that will be tuned by this sketch is FM.
#define FIX_STATION  8930            ///< The station that will be tuned by this sketch is 89.30 MHz.
#define FIX_VOLUME   4               ///< The volume that will be set by this sketch is level 4.

RDA5807FP radio;    // Create an instance of Class for RDA5807FP Chip

/// Setup a FM only radio configuration
/// with some debugging on the Serial port
void setup() {
  // open the Serial port
  Serial.begin(57600);
  Serial.println("Radio...");
  delay(200);

  // Initialize the Radio 
  radio.init();

  // configure I2S
  RDA5807FP_I2SConfig config;
  config.enabled = true;
  config.slave = false;
  config.data_signed = true;
  config.rate = RDA5807FP_I2S_WS_CNT::WS_STEP_48;

  // Enable information to the Serial port
  radio.debugEnable();

  // Set all radio setting to the fixed values.
  radio.setBandFrequency(FIX_BAND, FIX_STATION);
  radio.setVolume(FIX_VOLUME);
  radio.setMono(false);
  radio.setMute(false);
} // setup


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
} // loop

// End.

