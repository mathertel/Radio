///
/// \file RDA5807M.h
/// \brief Library header file for the radio library to control the RDA5807M radio chip.
///
/// \author Matthias Hertel, http://www.mathertel.de
/// \copyright Copyright (c) 2014-2015 by Matthias Hertel.\n
/// This work is licensed under a BSD style license.\n
/// See http://www.mathertel.de/License.aspx
/// 
/// \details
/// This library enables the use of the Radio Chip RDA5807M from http://www.rdamicro.com/ that supports FM radio bands and RDS data.
///
/// More documentation and source code is available at http://www.mathertel.de/Arduino
///
/// History:
/// --------
/// * 12.05.2014 creation of the RDA5807M library.
/// * 28.06.2014 running simple radio
/// * 08.07.2014 RDS data receive function can be registered.

// multi-Band enabled

// - - - - -
// help from: http://arduino.vom-kuhberg.de/index.php
//   http://projects.qi-hardware.com/index.php/p/qi-kernel/source/tree/144e9c2530f863e32a3538b06c63484401bbe314/drivers/media/radio/radio-rda5807.c


#ifndef RDA5807M_h
#define RDA5807M_h

#include <Arduino.h>
#include <Wire.h>
#include <radio.h>

// ----- library definition -----

/// Library to control the RDA5807M radio chip.
class RDA5807M : public RADIO {
  public:
    // ----- RDA5807M specific implementations -----
    const uint8_t MAXVOLUME = 15;   ///< max volume level for radio implementations.

  RDA5807M();
  
  bool   init();
  void   term();
  
  // ----- Audio features -----
  
  void   setVolume(uint8_t newVolume);
  void   setBassBoost(bool switchOn);
  void   setMono(bool switchOn);
  void   setMute(bool switchOn);
  void   setSoftMute(bool switchOn);    ///< Set the soft mute mode (mute on low signals) on or off.

  // ----- Receiver features -----
  void   setBand(RADIO_BAND newBand);
  void   setFrequency(RADIO_FREQ newF);
  RADIO_FREQ getFrequency(void);

  void    seekUp(bool toNextSender = true);   // start seek mode upwards
  void    seekDown(bool toNextSender = true); // start seek mode downwards
  
  // ----- Supporting RDS for RADIO_BAND_FM and RADIO_BAND_FMWORLD

  void    checkRDS();

  // ----- combined status functions -----

  virtual void getRadioInfo(RADIO_INFO *info); ///< Retrieve some information about the current radio function of the chip.

  // ----- Supporting RDS for RADIO_BAND_FM and RADIO_BAND_FMWORLD

  // ----- debug Helpers send information to Serial port
  
  void    debugScan();               // Scan all frequencies and report a status
  void    debugStatus();             // DebugInfo about actual chip data available

  private:
  // ----- local variables
  uint16_t registers[16];  // memory representation of the registers

  // ----- low level communication to the chip using I2C bus

  void     _readRegisters(); // read all status & data registers
  void     _saveRegisters();     // Save writable registers back to the chip
  void     _saveRegister(byte regNr); // Save one register back to the chip
  
  void     _write16(uint16_t val);        // Write 16 Bit Value on I2C-Bus
  uint16_t _read16(void);
};

#endif
