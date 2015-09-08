///
/// \file newchip.h
/// \brief Library header file for the radio library to control the newchip radio chip.
///
/// \author Matthias Hertel, http://www.mathertel.de
/// \copyright Copyright (c) 2014-2015 by Matthias Hertel.\n
/// This work is licensed under a BSD style license.\n
/// See http://www.mathertel.de/License.aspx
///
/// This library can be used as a starting point to implement a new radio chip for the radio library.
///
/// More documentation and source code is available at http://www.mathertel.de/Arduino
///
/// History:
/// --------
/// * 05.08.2014 created.


#ifndef newchip_h
#define newchip_h

#include <Arduino.h>
#include <Wire.h>

#include <radio.h>

// ----- library definition -----


/// Template library control a new radio chip.
class newchip : public RADIO {
  public:
    const uint8_t MAXVOLUME = 15;   ///< max volume level for radio implementations.
    newchip();
  
  bool   init();  // initialize library and the chip.
  void   term();  // terminate all radio functions.
  
  // Control of the audio features
  
  // Control the volume output of the radio chip
  void   setVolume(uint8_t newVolume); // set volume to 0..15

  // Control the bass boost function of the radio chip
  void   setBassBoost(bool switchOn);

  // Control mono/stereo mode of the radio chip
  void   setMono(bool switchOn); // Switch to mono mode.

  // Control the mute function of the radio chip
  void   setMute(bool switchOn); // Switch to mute mode.

  // Control of the core receiver

  // Control the frequency
  void setBand(RADIO_BAND newBand);

  void    setFrequency(RADIO_FREQ newF);
  RADIO_FREQ getFrequency(void);

  void seekUp(bool toNextSender = true);   // start seek mode upwards
  void seekDown(bool toNextSender = true); // start seek mode downwards
  
  void checkRDS(); // read RDS data from the current station and process when data available.
  
  void getRadioInfo(RADIO_INFO *info);
  void getAudioInfo(AUDIO_INFO *info);

  // ----- debug Helpers send information to Serial port
  
  void  debugScan();               // Scan all frequencies and report a status
  void  debugStatus();             // Report Info about actual Station

  // ----- read/write registers of the chip

  void  _readRegisters();  // read all status & data registers
  void  _saveRegisters();  // Save writable registers back to the chip

  private:
  // ----- local variables

  // store the current values of the 16 chip internal 16-bit registers
  // uint16_t registers[16];  

  // ----- low level communication to the chip using I2C bus

  void     _write16(uint16_t val);        // Write 16 Bit Value on I2C-Bus
  uint16_t _read16(void);
  
  void _seek(bool seekUp = true);
  void _waitEnd();
};

#endif
