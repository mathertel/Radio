///
/// \file SI4705.h
/// \brief Library header file for the radio library to control the SI4705 radio chip.
///
/// \author Matthias Hertel, http://www.mathertel.de
/// \copyright Copyright (c) 2014 by Matthias Hertel.\n
/// This work is licensed under a BSD style license.\n
/// See http://www.mathertel.de/License.aspx
///
/// This library enables the use of the Radio Chip SI4703.
///
/// More documentation and source code is available at http://www.mathertel.de/Arduino
///
/// History:
/// --------
/// * 05.12.2014 created.
/// *   .01.2015 working first version.


#ifndef SI4705_h
#define SI4705_h

#include <arduino.h>
#include <Wire.h>

#include <radio.h>

// ----- library definition -----


class SI4705 : public RADIO {
public:
  SI4705();

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

private:
  // ----- local variables

  // store the current status values
  uint8_t tuneStatus[8];
  uint8_t rsqStatus[8];
  uint8_t rdsStatus[12];

  // ----- low level communication to the chip using I2C bus

  /// send a command
  void _sendCommand(int cnt, int cmd, ...);

  /// set a property
  void _setProperty(uint16_t prop, uint16_t value);

  /// read status information into a buffer
  void _readStatus(uint8_t cmd, uint8_t param, uint8_t *values, uint8_t len);


  void     _write16(uint16_t val);        // Write 16 Bit Value on I2C-Bus
  uint16_t _read16(void);

  void _seek(bool seekUp = true);
  void _waitEnd();
};

#endif