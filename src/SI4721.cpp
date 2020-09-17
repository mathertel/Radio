///
/// \file SI4721.cpp
/// \brief Implementation for the radio library to control the SI4721 radio chip.
///
/// \author N Poole, nickpoole.me
/// \author Matthias Hertel, http://www.mathertel.de
/// \copyright Copyright (c) 2014 by Matthias Hertel.\n
/// This work is licensed under a BSD style license.\n
/// See http://www.mathertel.de/License.aspx
///
/// This library enables the use of the Radio Chip SI4721.
///
/// More documentation and source code is available at http://www.mathertel.de/Arduino
///
/// Many hints can be found in AN332: http://www.silabs.com/Support%20Documents/TechnicalDocs/AN332.pdf
///
/// ChangeLog see SI4721.h.

#include <Arduino.h>
#include <Wire.h> // The chip is controlled via the standard Arduiino Wire library and the IIC/I2C bus.

#include <radio.h> // Include the common radio library interface

// Include the chip specific radio library interface
#include <SI4721.h>

// ----- Radio chip specific definitions including the registers

// Commands and Parameter definitions

#define CMD_POWER_UP 0x01 // Power up device and mode selection.
#define CMD_POWER_UP_1_FUNC_FM 0x00
#define CMD_POWER_UP_1_XOSCEN 0x10
#define CMD_POWER_UP_1_PATCH 0x20
#define CMD_POWER_UP_1_GPO2OEN 0x40
#define CMD_POWER_UP_1_CTSIEN 0x80
#define CMD_POWER_UP_2_ANALOGOUT 0x05

#define CMD_GET_REV 0x10 //	Returns revision information on the device.
#define CMD_POWER_DOWN 0x11 //	Power down device.

#define CMD_SET_PROPERTY 0x12 //	Sets the value of a property.
#define CMD_GET_PROPERTY 0x13 //	Retrieves a property’s value.
#define CMD_GET_INT_STATUS 0x14 //	Reads interrupt status bits.
#define CMD_GET_INT_STATUS_CTS 0x80 //	CTS flag in status


#define CMD_PATCH_ARGS *0x15 //	Reserved command used for patch file downloads.
#define CMD_PATCH_DATA *0x16 //	Reserved command used for patch file downloads.
#define CMD_FM_TUNE_FREQ 0x20 //	Selects the FM tuning frequency.
#define CMD_FM_SEEK_START 0x21 //	Begins searching for a valid frequency.
#define CMD_FM_TUNE_STATUS 0x22 //	Queries the status of previous FM_TUNE_FREQ or FM_SEEK_START command.
#define CMD_FM_RSQ_STATUS 0x23 //	Queries the status of the Received Signal Quality (RSQ) of the current channel
#define CMD_FM_RDS_STATUS 0x24 //	Returns RDS information for current channel and reads an entry from RDS FIFO.
#define CMD_FM_AGC_STATUS 0x27 //	Queries the current AGC settings All
#define CMD_FM_AGC_OVERRIDE 0x28 //	Override AGC setting by disabling and forcing it to a fixed value

#define CMD_TX_TUNE_FREQ 0x30
#define CMD_TX_TUNE_POWER 0x31
#define CMD_TX_TUNE_MEASURE 0x32
#define CMD_TX_TUNE_STATUS 0x33
#define CMD_TX_ASQ_STATUS 0x34
#define CMD_TX_RDS_BUFF 0x35
#define CMD_TX_RDS_PS 0x36

#define CMD_GPIO_CTL 0x80 //	Configures GPO1, 2, and 3 as output or Hi-Z.
#define CMD_GPIO_CTL_GPO1OEN 0x02
#define CMD_GPIO_CTL_GPO2OEN 0x04
#define CMD_GPIO_CTL_GPO3OEN 0x08

#define CMD_GPIO_SET 0x81 //  Sets GPO1, 2, and 3 output level (low or high).
#define CMD_GPIO_SET_GPO1LEVEL 0x02
#define CMD_GPIO_SET_GPO2LEVEL 0x04
#define CMD_GPIO_SET_GPO3LEVEL 0x08

#define CMD_GPO_CTL 0x80
#define CMD_GPO_SET 0x81

// Property and Parameter definitions

#define PROP_GPO_IEN 0x0001
#define PROP_GPO_IEN_STCIEN 0x01
#define PROP_GPO_IEN_RDSIEN 0x04

// Deemphasis time constant.
#define PROP_FM_DEEMPHASIS 0x1100
#define PROP_FM_DEEMPHASIS_50 0x01
#define PROP_FM_DEEMPHASIS_75 0x02

// stereo blend
#define PROP_FM_BLEND_STEREO_THRESHOLD 0x1105

// setup the antenna input pin
#define PROP_FM_ANTENNA_INPUT 0x1107
#define PROP_FM_ANTENNA_INPUT_FMI 0x00
#define PROP_FM_ANTENNA_INPUT_SHORT 0x01

// FM_MAX_TUNE_ERROR
// #define FM_MAX_TUNE_ERROR      0x1108

// #define FM_SOFT_MUTE_RATE            0x1300 // not in use any more
#define FM_SOFT_MUTE_SLOPE 0x1301
#define FM_SOFT_MUTE_MAX_ATTENUATION 0x1302
#define FM_SOFT_MUTE_SNR_THRESHOLD 0x1303
#define FM_SOFT_MUTE_RELEASE_RATE 0x1304
#define FM_SOFT_MUTE_ATTACK_RATE 0x1305

#define PROP_FM_SEEK_FREQ_SPACING 0x1402
#define FM_SEEK_TUNE_SNR_THRESHOLD 0x1403
#define FM_SEEK_TUNE_RSSI_TRESHOLD 0x1404

#define PROP_RDS_INTERRUPT_SOURCE 0x1500
#define PROP_RDS_INTERRUPT_SOURCE_RDSRECV 0x01

#define PROP_RDS_INT_FIFO_COUNT 0x1501

#define PROP_RDS_CONFIG 0x1502

#define PROP_RX_VOLUME 0x4000

#define PROP_FM_BLEND_RSSI_STEREO_THRESHOLD 0x1800
#define PROP_FM_BLEND_RSSI_MONO_THRESHOLD 0x1801

#define PROP_RX_HARD_MUTE 0x4001
#define PROP_RX_HARD_MUTE_RIGHT 0x01
#define PROP_RX_HARD_MUTE_LEFT 0x02
#define PROP_RX_HARD_MUTE_BOTH 0x03

// Transmit Parameters
#define PROP_DIGITAL_INPUT_FORMAT 0x0101
#define PROP_DIGITAL_INPUT_SAMPLE_RATE 0x0103
#define PROP_REFCLK_FREQ 0x0201
#define PROP_REFCLK_PRESCALE 0x0202
#define PROP_TX_COMPONENT_ENABLE 0x2100
#define PROP_TX_AUDIO_DEVIATION 0x2101
#define PROP_TX_PILOT_DEVIATION 0x2102
#define PROP_TX_RDS_DEVIATION 0x2103
#define PROP_TX_LINE_LEVEL_INPUT_LEVEL 0x2104
#define PROP_TX_LINE_INPUT_MUTE 0x2105
#define PROP_TX_PILOT_FREQUENCY 0x2107
#define PROP_TX_ACOMP_ENABLE 0x2200
#define PROP_TX_ACOMP_THRESHOLD 0x2201
#define PROP_TX_ATTACK_TIME 0x2202
#define PROP_TX_RELEASE_TIME 0x2203
#define PROP_TX_ACOMP_GAIN 0x2204
#define PROP_TX_LIMITER_RELEASE_TIME 0x2205
#define PROP_TX_ASQ_INTERRUPT_SOURCE 0x2300
#define PROP_TX_ASQ_LEVEL_LOW 0x2301
#define PROP_TX_ASQ_DURATION_LOW 0x2302
#define PROP_TX_AQS_LEVEL_HIGH 0x2303
#define PROP_TX_AQS_DURATION_HIGH 0x2304
#define PROP_TX_RDS_INTERRUPT_SOURCE 0x2C00
#define PROP_TX_RDS_PI 0x2C01
#define PROP_TX_RDS_PS_MIX 0x2C02
#define PROP_TX_RDS_PS_MISC 0x2C03
#define PROP_TX_RDS_PS_REPEAT_COUNT 0x2C04
#define PROP_TX_RDS_MESSAGE_COUNT 0x2C05
#define PROP_TX_RDS_PS_AF 0x2C06
#define PROP_TX_RDS_FIFO_SIZE 0x2C07

// Preemphasis time constant.
#define PROP_TX_PREEMPHASIS 0x2106
#define PROP_TX_PREEMPHASIS_50 0x01
#define PROP_TX_PREEMPHASIS_75 0x00

/// Initialize the extra variables in SI4721
SI4721::SI4721()
{
  _realVolume = 0;
}

/// Initialize the library and the chip.
/// Set all internal variables to the standard values.
/// @return bool The return value is true when a SI4721 chip was found.
bool SI4721::init(TwoWire &wirePort, uint8_t deviceAddress)
{
  bool result; // chip found ?.
  DEBUG_FUNC0("init");

  RADIO::init();

  // Now that the unit is reset and I2C interface mode, we need to begin I2C
  _i2cPort = &wirePort;
  _i2cPort->begin();
  _i2caddr = deviceAddress;

  // see if a chip can be found
  result = RADIO::_wireExists(_i2cPort, deviceAddress);


  // Power down the device
  _sendCommand(1, CMD_POWER_DOWN);

  // powering up is done by specifying the band etc. so it's implemented in setBand

  return (result);
} // init()


/// Switch all functions of the chip off by powering down.
/// @return void
void SI4721::term()
{
  _sendCommand(1, CMD_POWER_DOWN);
} // term


// ----- Audio output control -----

/// This function maps the newVolume value in the range 0..15 to the range 0..63 that is available in this chip.
/// @param newVolume The new volume level of audio output.
void SI4721::setVolume(uint8_t newVolume)
{
  DEBUG_FUNC1("setVolume", newVolume);
  setVolumeX(newVolume * 4);
} // setVolume()


/// This function sets the volume in the range 0..63.
/// @param newVolume The new volume level of audio output.
void SI4721::setVolumeX(uint8_t newVolume)
{
  if (newVolume > 63)
    newVolume = 63;
  _setProperty(PROP_RX_VOLUME, newVolume);
  _realVolume = newVolume;
  RADIO::setVolume(newVolume / 4);
} // setVolumeX()


/// Retrieve the current output volume in the range 0..63.
/// @return uint8_t actual volume.
uint8_t SI4721::getVolumeX()
{
  return (_realVolume);
} // getVolumeX()


/// Control the mute mode of the radio chip
/// In mute mode no output will be produced by the radio chip.
/// @param switchOn The new state of the mute mode. True to switch on, false to switch off.
/// @return void
void SI4721::setMute(bool switchOn)
{
  RADIO::setMute(switchOn);

  if (switchOn) {
    // Set mute bits in the fm receiver
    _setProperty(PROP_RX_HARD_MUTE, PROP_RX_HARD_MUTE_BOTH);

  } else {
    // clear mute bits in the fm receiver
    _setProperty(PROP_RX_HARD_MUTE, 0x00);
  } // if
} // setMute()


/// Control the FM deemphasis of the radio chip
/// This number is also used as the pre-emphasis in transmit mode
/// @param uS The new deemphasis value in µS
/// @return void
void SI4721::setDeemphasis(uint8_t uS)
{
  _fmDeemphasis = uS;
}

/// Control the softmute mode of the radio chip
/// If switched on the radio output is muted when no sender was found.
/// @param switchOn The new state of the softmute mode. True to switch on, false to switch off.
/// @return void
void SI4721::setSoftMute(bool switchOn)
{
  DEBUG_FUNC1("setSoftMute", switchOn);

  RADIO::setSoftMute(switchOn);

  if (switchOn) {
    // to enable the softmute mode the attenuation is set to 0x10.
    _setProperty(FM_SOFT_MUTE_MAX_ATTENUATION, 0x14);
  } else {
    // to disable the softmute mode the attenuation is set to 0.
    _setProperty(FM_SOFT_MUTE_MAX_ATTENUATION, 0x00);
  }
} // setSoftMute()


/// BassBoost is not supported by the SI4721 chip.
/// @param switchOn this functions ignores the switchOn parameter and always sets bassBoost to false.
/// @return void
void SI4721::setBassBoost(bool switchOn)
{
  DEBUG_FUNC1("setBassBoost", switchOn);
  DEBUG_STR("not supported.");
  RADIO::setBassBoost(false);
} // setBassBoost()


/// Control the mono mode of the radio chip
/// In mono mode the stereo decoding will be switched off completely  and the noise is typically reduced.
/// @param switchOn The new state of the mono mode. True to switch on, false to switch off.
/// @return void
void SI4721::setMono(bool switchOn)
{
  DEBUG_FUNC1("setMono", switchOn);
  RADIO::setMono(switchOn);
  if (switchOn) {
    // disable automatic stereo feature
    _setProperty(PROP_FM_BLEND_STEREO_THRESHOLD, 127);
    // not available in SI4721:
    // _setProperty(PROP_FM_BLEND_RSSI_STEREO_THRESHOLD, 127);
    // _setProperty(PROP_FM_BLEND_RSSI_MONO_THRESHOLD, 127);

  } else {
    // Automatic stereo feature on.
    _setProperty(PROP_FM_BLEND_STEREO_THRESHOLD, 49); // default = 49 dBμV
  } // if
} // setMono


// ----- Band and frequency control methods -----

/**
 * Start using the new band for receiving or transmitting.
 * @param newBand The new band to be enabled.
 * @return void
 */
void SI4721::setBand(RADIO_BAND newBand)
{
  DEBUG_FUNC1("setBand", newBand);

  if (newBand == RADIO_BAND_FM) {
    // set band boundaries and steps
    RADIO::setBand(newBand);

    // Power down the device
    _sendCommand(1, CMD_POWER_DOWN);

    // Give the device some time to power down before restart
    delay(500);

    // Power up in receive mode without patch
    _sendCommand(3, CMD_POWER_UP, (CMD_POWER_UP_1_XOSCEN | CMD_POWER_UP_1_FUNC_FM), CMD_POWER_UP_2_ANALOGOUT);

    // delay 500 msec when using the crystal oscillator as mentioned in the note from the POWER_UP command.
    delay(500);
    _setProperty(PROP_REFCLK_FREQ, 32768); // crystal is 32.768
    _setProperty(PROP_FM_DEEMPHASIS, _fmDeemphasis == 75 ? PROP_FM_DEEMPHASIS_75 : PROP_FM_DEEMPHASIS_50); // Europe 50μS / USA 75μS deemphasis
    _setProperty(PROP_FM_SEEK_FREQ_SPACING, _freqSteps); // in 100kHz spacing
    _setProperty(PROP_FM_ANTENNA_INPUT, PROP_FM_ANTENNA_INPUT_SHORT); // sets antenna input
    setFrequency(_freqLow);

    setMono(true);
    setSoftMute(true);
    setVolume(0);
    setMute(false);

    // adjust sensibility for scanning
    _setProperty(FM_SEEK_TUNE_SNR_THRESHOLD, 12);
    _setProperty(FM_SEEK_TUNE_RSSI_TRESHOLD, 42);

    // _setProperty(PROP_GPO_IEN, 0); // no interrupts
    _setProperty(PROP_GPO_IEN, PROP_GPO_IEN_STCIEN | PROP_GPO_IEN_RDSIEN); //  | PROP_GPO_IEN_RDSIEN ????

  } else if (newBand == RADIO_BAND_FMTX) {
    // Power down the device
    _sendCommand(1, CMD_POWER_DOWN);

    // Give the device some time to power down before restart
    delay(500);

    // Power up in transmit mode
    _sendCommand(3, CMD_POWER_UP, 0x12, 0x50);
    // delay 500 msec when using the crystal oscillator as mentioned in the note from the POWER_UP command.
    delay(500);
    _setProperty(PROP_REFCLK_FREQ, 32768); // crystal is 32.768
    _setProperty(PROP_TX_PREEMPHASIS, _fmDeemphasis == 75 ? PROP_TX_PREEMPHASIS_75 : PROP_TX_PREEMPHASIS_50); // uses the RX deemphasis as the TX preemphasis
    _setProperty(PROP_TX_ACOMP_GAIN, 10); // sets max gain
    _setProperty(PROP_TX_ACOMP_ENABLE, 0x0); // turns on limiter and AGC

  } else {
    _sendCommand(1, CMD_POWER_DOWN);

  } // if
} // setBand()


/// Retrieve the real frequency from the chip after manual or automatic tuning.
/// @return RADIO_FREQ the current frequency.
RADIO_FREQ SI4721::getFrequency()
{

  if (_band == RADIO_BAND_FMTX) {
    _readStatusData(CMD_TX_TUNE_STATUS, 0x01, tuneStatus, sizeof(tuneStatus));
  } else {
    _readStatusData(CMD_FM_TUNE_STATUS, 0x03, tuneStatus, sizeof(tuneStatus));
  }

  _freq = (tuneStatus[2] << 8) + tuneStatus[3];
  return (_freq);
} // getFrequency


/// If the device is currently in receive mode then \n
/// start using the new frequency for receiving.\n
/// If the device is in transmit mode then set the transmit frequency. \n
/// The new frequency is stored for later retrieval by the base class.\n
/// Because the chip may change the frequency automatically (when seeking)
/// the stored value might not be the current frequency.
/// @param newF The new frequency to be received/transmitted.
/// @return void
void SI4721::setFrequency(RADIO_FREQ newF)
{
  uint8_t status;

  RADIO::setFrequency(newF);

  if (_band == RADIO_BAND_FMTX) {
    _sendCommand(4, CMD_TX_TUNE_FREQ, 0, (newF >> 8) & 0xff, (newF)&0xff);
  } else {
    _sendCommand(5, CMD_FM_TUNE_FREQ, 0, (newF >> 8) & 0xff, (newF)&0xff, 0);
  }

  // reset the RDSParser
  clearRDS();

  // loop until status ok.
  do {
    status = _readStatus();
  } while (!(status & CMD_GET_INT_STATUS_CTS));
} // setFrequency()


/// Start seek mode upwards.
void SI4721::seekUp(bool toNextSender)
{
  uint8_t status;

  if (!toNextSender) {
    RADIO_FREQ newF = getFrequency() + _freqSteps;
    setFrequency(newF);

  } else {
    _setProperty(FM_SEEK_TUNE_SNR_THRESHOLD, 12);
    _setProperty(FM_SEEK_TUNE_RSSI_TRESHOLD, 42);
    // start tuning
    _sendCommand(2, CMD_FM_SEEK_START, 0x0C);

    // reset the RDSParser
    clearRDS();

    // loop until status ok.
    do {
      status = _readStatus();
    } while (!(status & CMD_GET_INT_STATUS_CTS));
  } // if
} // seekUp()


/// Start seek mode downwards.
void SI4721::seekDown(bool toNextSender)
{
  uint8_t status;
  if (!toNextSender) {
    RADIO_FREQ newF = getFrequency() - _freqSteps;
    setFrequency(newF);

  } else {
    // start tuning
    _sendCommand(2, CMD_FM_SEEK_START, 0x04);

    // reset the RDSParser
    clearRDS();

    // loop until status ok.
    do {
      status = _readStatus();
    } while (!(status & CMD_GET_INT_STATUS_CTS));
  } // if
} // seekDown()


/// Load the status information from to the chip.
uint8_t SI4721::_readStatus()
{
  uint8_t data[1];
  _wireRead(_i2cPort, _i2caddr, CMD_GET_INT_STATUS, data, 1);
  return (data[0]);
} // _readStatus()


/// Load status information from to the chip.
void SI4721::_readStatusData(uint8_t cmd, uint8_t param, uint8_t *values, uint8_t len)
{
  _i2cPort->beginTransmission(_i2caddr);
  _i2cPort->write(cmd);
  _i2cPort->write(param);

  _i2cPort->endTransmission();
  _i2cPort->requestFrom(_i2caddr, (int)len); //We want to read some bytes.

  for (uint8_t n = 0; n < len; n++) {
    //Read in these bytes
    values[n] = _i2cPort->read();
  } // for
} // _readStatusData()


/// Return a filled RADIO_INFO with the status of the radio features of the chip.
void SI4721::getRadioInfo(RADIO_INFO *info)
{
  RADIO::getRadioInfo(info);

  _readStatusData(CMD_FM_TUNE_STATUS, 0x01, tuneStatus, sizeof(tuneStatus));

  info->active = true;
  if (tuneStatus[1] & 0x01)
    info->tuned = true;

  _readStatusData(CMD_FM_RSQ_STATUS, 0x01, rsqStatus, sizeof(rsqStatus));
  if (rsqStatus[3] & 0x80)
    info->stereo = true;
  info->rssi = rsqStatus[4];
  info->snr = rsqStatus[5];

  _readStatusData(CMD_FM_RDS_STATUS, 0x05, rdsStatus.buffer, sizeof(rdsStatus));
  if (rdsStatus.resp2 & 0x01)
    info->rds = true;
} // getRadioInfo()


/// Return a filled AUIO_INFO with the actual audio settings.
void SI4721::getAudioInfo(AUDIO_INFO *info)
{
  RADIO::getAudioInfo(info);
} // getAudioInfo()


// initialize RDS mode
void SI4721::attachReceiveRDS(receiveRDSFunction newFunction)
{
  DEBUG_FUNC0("attachReceiveRDS");

  // enable RDS
  _setProperty(PROP_RDS_INTERRUPT_SOURCE, PROP_RDS_INTERRUPT_SOURCE_RDSRECV); // Set the CTS status bit after receiving RDS data.
  _setProperty(PROP_RDS_INT_FIFO_COUNT, 4);
  _setProperty(PROP_RDS_CONFIG, 0xFF01); // accept all correctable data and enable rds

  RADIO::attachReceiveRDS(newFunction);
}

/// Retrieve the next RDS data if available.
void SI4721::checkRDS()
{
  if (_sendRDS) {
    // fetch the interrupt status first
    // uint8_t status = _readStatus();

    // fetch the current RDS data
    // _wireRead(_i2cPort, _i2caddr, )
    _readStatusData(CMD_FM_RDS_STATUS, 0x01, rdsStatus.buffer, sizeof(rdsStatus));

    // Serial.print("Status:");
    // Serial.println(rdsStatus.status, HEX);

    if ((rdsStatus.resp2 = 0x01) && (rdsStatus.rdsFifoUsed) && (rdsStatus.blockErrors == 0)) {
      // RDS is in sync, it's a complete entry and no errors

#define RDSBLOCKWORD(h, l) (h << 8 | l)

      _sendRDS(RDSBLOCKWORD(rdsStatus.blockAH, rdsStatus.blockAL),
               RDSBLOCKWORD(rdsStatus.blockBH, rdsStatus.blockBL),
               RDSBLOCKWORD(rdsStatus.blockCH, rdsStatus.blockCL),
               RDSBLOCKWORD(rdsStatus.blockDH, rdsStatus.blockDL));
    } // if
  } // if _sendRDS
} // checkRDS()

// ----- Transmitter functions -----

/// Set the output power of the device.
/// @param pwr Output power of the device in dBµV (valid range is 88 to 115)
/// @return void
void SI4721::setTXpower(uint8_t pwr)
{
  _sendCommand(5, CMD_TX_TUNE_POWER, 0, 0, pwr, 0);
}

/// Begin Broadcasting RDS and optionally set Program ID. \n
/// The Program ID should be a unique 4 character hexadecimal \n
/// code that identifies your station. By default, the Radio library \n
/// will use 0xBEEF as your Program ID.
/// @param programID Optional 4 character hexadecimal ID
/// @return void
void SI4721::beginRDS(uint16_t programID)
{

  _setProperty(PROP_TX_AUDIO_DEVIATION, 6625); // 66.25KHz (default is 68.25)
  _setProperty(PROP_TX_RDS_DEVIATION, 200); // 2KHz (default)
  _setProperty(PROP_TX_RDS_INTERRUPT_SOURCE, 0x0001); // RDS IRQ
  _setProperty(PROP_TX_RDS_PI, programID); // program identifier
  _setProperty(PROP_TX_RDS_PS_MIX, 0x03); // 50% mix (default)
  _setProperty(PROP_TX_RDS_PS_MISC, 0x1808); // RDSD0 & RDSMS (default)
  _setProperty(PROP_TX_RDS_PS_REPEAT_COUNT, 3); // 3 repeats (default)
  _setProperty(PROP_TX_RDS_MESSAGE_COUNT, 1);
  _setProperty(PROP_TX_RDS_PS_AF, 0xE0E0); // no AF
  _setProperty(PROP_TX_RDS_FIFO_SIZE, 0);
  _setProperty(PROP_TX_COMPONENT_ENABLE, 0x0007);
}

/// Set the RDS station name string. \n
/// Your Station Name (Programme Service Name) is a static, \n
/// 8 character display that represents your call letters or \n
/// station identity name.
/// @param *s string containing your 8 character name
/// @return void
void SI4721::setRDSstation(char *s)
{
  uint8_t i, len = strlen(s);
  uint8_t slots = (len + 3) / 4;

  for (uint8_t i = 0; i < slots; i++) {
    uint8_t psChar[4] = {' ', ' ', ' ', ' '};
    memcpy(psChar, s, min(4, (int)strlen(s))); // copy from index of string s to the minimum of 4 or the length of s
    s += 4; // advance index of s by 4
    _sendCommand(6, CMD_TX_RDS_PS, i, psChar[0], psChar[1], psChar[2], psChar[3], 0);
  }
}

/// Load new data into RDS Radio Text Buffer.
/// @param *s string containing arbitrary text to be transmitted as RDS Radio Text
/// @return void
void SI4721::setRDSbuffer(char *s)
{
  uint8_t i, len = strlen(s);
  uint8_t slots = (len + 3) / 4;
  char slot[5];

  for (uint8_t i = 0; i < slots; i++) {
    uint8_t rdsBuff[4] = {' ', ' ', ' ', ' '};
    memcpy(rdsBuff, s, min(4, (int)strlen(s)));
    s += 4;
    _sendCommand(8, CMD_TX_RDS_BUFF, i == 0 ? 0x06 : 0x04, 0x20, i, rdsBuff[0], rdsBuff[1], rdsBuff[2], rdsBuff[3], 0);
  }

  _setProperty(PROP_TX_COMPONENT_ENABLE, 0x0007); // stereo, pilot+rds
}

/// Get TX Status and Audio Input Metrics
/// @param void
/// @return ASQ_STATUS struct containing asq and audioInLevel values
ASQ_STATUS SI4721::getASQ()
{
  _sendCommand(2, CMD_TX_ASQ_STATUS, 0x1);

  _i2cPort->requestFrom((uint8_t)_i2caddr, (uint8_t)5);

  ASQ_STATUS result;

  uint8_t response[5];
  for (uint8_t i = 0; i < 5; i++) {
    response[i] = _i2cPort->read();
  }

  result.asq = response[1];
  result.audioInLevel = response[4];

  return result;
}

/// Get TX Tuning Status
/// @param void
/// @return TX_STATUS struct containing frequency, dBuV, antennaCap, and noiseLevel values
TX_STATUS SI4721::getTuneStatus()
{
  _sendCommand(2, CMD_TX_TUNE_STATUS, 0x1);

  _i2cPort->requestFrom((uint8_t)_i2caddr, (uint8_t)8);

  TX_STATUS result;

  uint8_t response[8];
  for (uint8_t i = 0; i < 8; i++) {
    response[i] = _i2cPort->read();
  }

  result.frequency = response[2];
  result.frequency <<= 8;
  result.frequency |= response[3];
  result.dBuV = response[5];
  result.antennaCap = response[6];
  result.noiseLevel = response[7];

  return result;
}

// ----- Debug functions -----

/// Send the current values of all registers to the Serial port.
void SI4721::debugStatus()
{
  RADIO::debugStatus();
  _readStatusData(CMD_FM_TUNE_STATUS, 0x03, tuneStatus, sizeof(tuneStatus));

  Serial.print("Tune-Status: ");
  Serial.print(tuneStatus[0], HEX);
  Serial.print(' ');
  Serial.print(tuneStatus[1], HEX);
  Serial.print(' ');

  Serial.print("TUNE:");
  Serial.print((tuneStatus[2] << 8) + tuneStatus[3]);
  Serial.print(' ');
  // RSSI and SNR when tune is complete (not the actual one ?)
  Serial.print("RSSI:");
  Serial.print(tuneStatus[4]);
  Serial.print(' ');
  Serial.print("SNR:");
  Serial.print(tuneStatus[5]);
  Serial.print(' ');
  Serial.print("MULT:");
  Serial.print(tuneStatus[6]);
  Serial.print(' ');
  Serial.print(tuneStatus[7]);
  Serial.print(' ');
  Serial.println();

  Serial.print("RSQ-Status: ");
  _readStatusData(CMD_FM_RSQ_STATUS, 0x01, rsqStatus, sizeof(rsqStatus));
  Serial.print(rsqStatus[0], HEX);
  Serial.print(' ');
  Serial.print(rsqStatus[1], HEX);
  Serial.print(' ');
  Serial.print(rsqStatus[2], HEX);
  Serial.print(' ');
  if (rsqStatus[2] & 0x08)
    Serial.print("SMUTE ");
  Serial.print(rsqStatus[3], HEX);
  Serial.print(' ');
  if (rsqStatus[3] & 0x80)
    Serial.print("STEREO ");
  // The current RSSI and SNR.
  Serial.print("RSSI:");
  Serial.print(rsqStatus[4]);
  Serial.print(' ');
  Serial.print("SNR:");
  Serial.print(rsqStatus[5]);
  Serial.print(' ');
  Serial.print(rsqStatus[7], HEX);
  Serial.print(' ');
  Serial.println();

  Serial.print("RDS-Status: ");
  _readStatusData(CMD_FM_RDS_STATUS, 0x01, rdsStatus.buffer, sizeof(rdsStatus));
  for (uint8_t n = 0; n < 12; n++) {
    Serial.print(rsqStatus[n], HEX);
    Serial.print(' ');
  } // for
  Serial.println();

  // AGC settings and status
  Serial.print("AGC-Status: ");
  _readStatusData(CMD_FM_AGC_STATUS, 0x01, agcStatus, sizeof(agcStatus));
  Serial.print(agcStatus[0], HEX);
  Serial.print(' ');
  Serial.print(agcStatus[1], HEX);
  Serial.print(' ');
  Serial.print(agcStatus[2], HEX);
  Serial.print(' ');
  Serial.println();

} // debugStatus


/// wait until the current seek and tune operation is over.
void SI4721::_waitEnd()
{
  DEBUG_FUNC0("_waitEnd");
} // _waitEnd()


/// Send an array of bytes to the radio chip
void SI4721::_sendCommand(int cnt, int cmd, ...)
{
  if (_debugRegisters) {
    Serial.print("CMD(");
    _printHex2(cmd);
    Serial.println(")");
  }
  if (cnt > 8) {
    // see AN332: "Writing more than 8 bytes results in unpredictable device behavior."
    Serial.println("error: _sendCommand: too much parameters!");

  } else {
    _i2cPort->beginTransmission(_i2caddr);
    _i2cPort->write(cmd);

    va_list params;
    va_start(params, cmd);

    for (uint8_t i = 1; i < cnt; i++) {
      uint8_t c = va_arg(params, int);
      if (_debugRegisters)
        _printHex2(c);
      _i2cPort->write(c);
    }
    _i2cPort->endTransmission();
    va_end(params);

    // wait for Command being processed
    _i2cPort->requestFrom(_i2caddr, 1); // We want to read the status byte.
    _status = _i2cPort->read();
  } // if

  if (_debugRegisters)
    Serial.println();
} // _sendCommand()


/// Set a property in the radio chip
void SI4721::_setProperty(uint16_t prop, uint16_t value)
{
  if (_debugRegisters) {
    // Serial.printf("PROP(%04x): %04x\n", prop, value);
  }
  _i2cPort->beginTransmission(_i2caddr);
  _i2cPort->write(CMD_SET_PROPERTY);
  _i2cPort->write(0);

  _i2cPort->write(prop >> 8);
  _i2cPort->write(prop & 0x00FF);
  _i2cPort->write(value >> 8);
  _i2cPort->write(value & 0x00FF);

  _i2cPort->endTransmission();

  _i2cPort->requestFrom(_i2caddr, 1); // We want to read the status byte.
  _status = _i2cPort->read();
} // _setProperty()


// ----- internal functions -----

// The End.
