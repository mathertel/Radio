///
/// \file SI4705.h
/// \brief Library header file for the radio library to control the SI4705 radio chip.
///
/// \author Matthias Hertel, http://www.mathertel.de
/// \copyright Copyright (c) 2014 by Matthias Hertel.\n
/// This work is licensed under a BSD style license.\n
/// See http://www.mathertel.de/License.aspx
///
/// This library enables the use of the Radio Chip SI4705.
///
/// More documentation and source code is available at http://www.mathertel.de/Arduino
///
/// ChangeLog:
/// ----------
/// * 05.12.2014 created.
/// * 30.01.2015 working first version.
/// * 07.02.2015 cleanup
/// * 15.02.2015 RDS is working.
/// * 27.03.2015 scanning is working. No changes to default settings needed.
/// * 03.05.2015 softmute is working. 


#ifndef SI4705_h
#define SI4705_h

#include <Arduino.h>

// The wire library is used for the communication with the radio chip.
#include <Wire.h>

// Include the radio library that is extended by the SI4705 library.
#include <radio.h>

// ----- library definition -----

/// Library to control the SI4705 radio chip.
class SI4705 : public RADIO {
public:
  const uint8_t MAXVOLUME = 15;   ///< max volume level for radio implementations.
  const uint8_t MAXVOLUMEX = 63;  ///< max volume level for the SI4705 specific implementation.

  SI4705();

  bool   init();  ///< Initialize the library and the chip.
  void   term();  ///< Terminate all radio functions in the chip.

  // ----- Audio functions -----

  void    setVolume(uint8_t newVolume);   ///< Control the volume output of the radio chip in the range 0..15.

  void    setVolumeX(uint8_t newVolume);  ///< Control the volume output of the radio chip in the range 0..63.
  uint8_t getVolumeX();                   ///< Retrieve the current output volume in the range 0..63.

  void    setMute(bool switchOn);         ///< Control the mute mode of the radio chip.
  void    setSoftMute(bool switchOn);     ///< Control the softmute mode (mute on low signals) of the radio chip.

  // Overwrite audio functions that are not supported.
  void    setBassBoost(bool switchOn);    ///< regardless of the given parameter, the Bass Boost will never switch on.

  // ----- Radio receiver functions -----

  void    setMono(bool switchOn);         ///< Control the mono/stereo mode of the radio chip.

  void    setBand(RADIO_BAND newBand);    ///< Control the band of the radio chip.

  void    setFrequency(RADIO_FREQ newF);  ///< Control the frequency.
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

  uint8_t _realVolume; ///< The real volume set to the chip.

  // store the current status values
  uint8_t _status;        ///< the status after sending a command

  uint8_t tuneStatus[8];
  uint8_t rsqStatus[1 + 7];
  uint8_t rdsStatusx[1 + 12];
  uint8_t agcStatus[1 + 2];

  /// structure used to read status information from the SI4705 radio chip.
  union {
    // use structured access 
    struct {
      uint8_t  status;
      uint8_t  resp1;
      uint8_t  resp2;
      uint8_t  rdsFifoUsed;
      uint8_t  blockAH; uint8_t  blockAL;
      uint8_t  blockBH; uint8_t  blockBL;
      uint8_t  blockCH; uint8_t  blockCL;
      uint8_t  blockDH; uint8_t  blockDL;
      uint8_t  blockErrors;
    };
    // use the the byte while receiving and sending.
    uint8_t buffer[1 + 7];
  } tuneStatus2; // union RDSSTATUS


  /// structure used to read RDS information from the SI4705 radio chip.
  union {
    // use structured access 
    struct {
      uint8_t  status;
      uint8_t  resp1;
      uint8_t  resp2;
      uint8_t  rdsFifoUsed;
      uint8_t  blockAH; uint8_t  blockAL;
      uint8_t  blockBH; uint8_t  blockBL;
      uint8_t  blockCH; uint8_t  blockCL;
      uint8_t  blockDH; uint8_t  blockDL;
      uint8_t  blockErrors;
    };
    // use the the byte while receiving and sending.
    uint8_t buffer[1 + 12];
  } rdsStatus; // union RDSSTATUS


  // ----- low level communication to the chip using I2C bus

  /// send a command
  void _sendCommand(int cnt, int cmd, ...);

  /// set a property
  void _setProperty(uint16_t prop, uint16_t value);

  /// read the interrupt status.
  uint8_t _readStatus();

  /// read status information into a buffer
  void _readStatusData(uint8_t cmd, uint8_t param, uint8_t *values, uint8_t len);

  void _seek(bool seekUp = true);
  void _waitEnd();
};

#endif
