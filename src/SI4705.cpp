///
/// \file SI4705.cpp
/// \brief Implementation for the radio library to control the SI4705 radio chip.
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
/// Many hints can be found in AN332: http://www.silabs.com/Support%20Documents/TechnicalDocs/AN332.pdf
///
/// ChangeLog see SI4705.h.

// Include the common radio library interface
#include <radio.h>

#include <SI4705.h>

// ----- Definitions for the Wire communication

#define SI4705_ADR 0x63  ///< The I2C address of SI4705 is 0x61 or 0x63

/// Uncomment this definition when not using the ELV radio board.
/// There is a special mute implementation.
#define ELVRADIO

// ----- Radio chip specific definitions including the registers

// Commands and Parameter definitions

#define	CMD_POWER_UP	         0x01  // Power up device and mode selection.
#define	CMD_POWER_UP_1_FUNC_FM   0x00
#define	CMD_POWER_UP_1_XOSCEN    0x10
#define	CMD_POWER_UP_1_PATCH     0x20
#define	CMD_POWER_UP_1_GPO2OEN   0x40
#define	CMD_POWER_UP_1_CTSIEN    0x80
#define	CMD_POWER_UP_2_ANALOGOUT 0x05

#define	CMD_GET_REV	        0x10	//	Returns revision information on the device.
#define	CMD_POWER_DOWN	    0x11	//	Power down device.

#define	CMD_SET_PROPERTY	0x12	//	Sets the value of a property.
#define	CMD_GET_PROPERTY	0x13	//	Retrieves a property�s value.
#define	CMD_GET_INT_STATUS	0x14	//	Reads interrupt status bits.
#define	CMD_GET_INT_STATUS_CTS	0x80	//	CTS flag in status


#define	CMD_PATCH_ARGS*	    0x15	//	Reserved command used for patch file downloads.
#define	CMD_PATCH_DATA*	    0x16	//	Reserved command used for patch file downloads.
#define	CMD_FM_TUNE_FREQ	0x20	//	Selects the FM tuning frequency.
#define	CMD_FM_SEEK_START	0x21	//	Begins searching for a valid frequency.
#define	CMD_FM_TUNE_STATUS	0x22	//	Queries the status of previous FM_TUNE_FREQ or FM_SEEK_START command.
#define	CMD_FM_RSQ_STATUS	0x23	//	Queries the status of the Received Signal Quality (RSQ) of the current channel
#define	CMD_FM_RDS_STATUS	0x24	//	Returns RDS information for current channel and reads an entry from RDS FIFO.
#define	CMD_FM_AGC_STATUS	0x27	//	Queries the current AGC settings All
#define	CMD_FM_AGC_OVERRIDE	0x28	//	Override AGC setting by disabling and forcing it to a fixed value

#define	CMD_GPIO_CTL	     0x80	//	Configures GPO1, 2, and 3 as output or Hi-Z.
#define	CMD_GPIO_CTL_GPO1OEN 0x02
#define	CMD_GPIO_CTL_GPO2OEN 0x04
#define	CMD_GPIO_CTL_GPO3OEN 0x08

#define	CMD_GPIO_SET           0x81   //  Sets GPO1, 2, and 3 output level (low or high).
#define	CMD_GPIO_SET_GPO1LEVEL 0x02
#define	CMD_GPIO_SET_GPO2LEVEL 0x04
#define	CMD_GPIO_SET_GPO3LEVEL 0x08

// Property and Parameter definitions

#define PROP_GPO_IEN          0x0001
#define PROP_GPO_IEN_STCIEN   0x01
#define PROP_GPO_IEN_RDSIEN   0x04

// Deemphasis time constant.
#define PROP_FM_DEEMPHASIS     0x1100
#define PROP_FM_DEEMPHASIS_50  0x01

// setup the antenna input pin
#define PROP_FM_ANTENNA_INPUT       0x1107
#define PROP_FM_ANTENNA_INPUT_FMI   0x00
#define PROP_FM_ANTENNA_INPUT_SHORT 0x01

// FM_MAX_TUNE_ERROR
// #define FM_MAX_TUNE_ERROR      0x1108

// #define FM_SOFT_MUTE_RATE            0x1300 // not in use any more
#define FM_SOFT_MUTE_SLOPE           0x1301
#define FM_SOFT_MUTE_MAX_ATTENUATION 0x1302
#define FM_SOFT_MUTE_SNR_THRESHOLD   0x1303
#define FM_SOFT_MUTE_RELEASE_RATE    0x1304
#define FM_SOFT_MUTE_ATTACK_RATE     0x1305

#define PROP_FM_SEEK_FREQ_SPACING   0x1402
#define FM_SEEK_TUNE_SNR_THRESHOLD  0x1403 
#define FM_SEEK_TUNE_RSSI_TRESHOLD  0x1404

#define PROP_RDS_INTERRUPT_SOURCE         0x1500
#define PROP_RDS_INTERRUPT_SOURCE_RDSRECV 0x01

#define PROP_RDS_INT_FIFO_COUNT 0x1501

#define PROP_RDS_CONFIG 0x1502

#define PROP_RX_VOLUME 0x4000

#define PROP_FM_BLEND_RSSI_STEREO_THRESHOLD 0x1800 
#define PROP_FM_BLEND_RSSI_MONO_THRESHOLD   0x1801

#define PROP_RX_HARD_MUTE       0x4001 
#define PROP_RX_HARD_MUTE_RIGHT 0x01 
#define PROP_RX_HARD_MUTE_LEFT  0x02 
#define PROP_RX_HARD_MUTE_BOTH  0x03 

// ----- implement

/// Initialize the extra variables in SI4705
SI4705::SI4705() {
  _realVolume = 0;
}

/// Initialize the library and the chip.
/// Set all internal variables to the standard values.
/// @return bool The return value is true when a SI4705 chip was found.
bool SI4705::init() {
  bool result = false; // no chip found yet.
  DEBUG_FUNC0("init");

  // Now that the unit is reset and I2C inteface mode, we need to begin I2C
  Wire.begin();

  // powering up is done by specifying the band etc. so it's implemented in setBand
  setBand(RADIO_BAND_FM);

  // set some common properties
  _setProperty(PROP_FM_ANTENNA_INPUT, PROP_FM_ANTENNA_INPUT_SHORT);

#if defined(ELVRADIO)
  // enable GPO1 output for mute function
  _sendCommand(2, CMD_GPIO_CTL, CMD_GPIO_CTL_GPO1OEN);
#endif

  // set volume to 0 and mute so no noise gets out here.
  _setProperty(PROP_RX_VOLUME, 0);

  // Set mute bits in the fm receiver
  setMute(true);
  setSoftMute(true);

  // adjust sensibility for scanning
  _setProperty(FM_SEEK_TUNE_SNR_THRESHOLD, 12);
  _setProperty(FM_SEEK_TUNE_RSSI_TRESHOLD, 42);

  _setProperty(PROP_GPO_IEN, PROP_GPO_IEN_STCIEN); //  | PROP_GPO_IEN_RDSIEN ????

  // RDS
  _setProperty(PROP_RDS_INTERRUPT_SOURCE, PROP_RDS_INTERRUPT_SOURCE_RDSRECV); // Set the CTS status bit after receiving RDS data.
  _setProperty(PROP_RDS_INT_FIFO_COUNT, 4);
  _setProperty(PROP_RDS_CONFIG, 0xFF01); // accept all correctable data and enable rds

  return(result);
} // init()


/// Switch all functions of the chip off by powering down.
/// @return void
void SI4705::term()
{
  _sendCommand(1, CMD_POWER_DOWN);
} // term


// ----- Audio output control -----

/// This function maps the newVolume value in the range 0..15 to the range 0..63 that is available in this chip.
/// @param newVolume The new volume level of audio output.
void SI4705::setVolume(uint8_t newVolume)
{
  setVolumeX(newVolume * 4);
} // setVolume()


/// This function sets the volume in the range 0..63.
/// @param newVolume The new volume level of audio output.
void SI4705::setVolumeX(uint8_t newVolume)
{
  if (newVolume > 63) newVolume = 63;
  _setProperty(PROP_RX_VOLUME, newVolume);
  _realVolume = newVolume;
  RADIO::setVolume(newVolume / 4);
} // setVolumeX()


/// Retrieve the current output volume in the range 0..63.
/// @return uint8_t actual volume.
uint8_t SI4705::getVolumeX() {
  return(_realVolume);
} // getVolumeX()


/// Control the mute mode of the radio chip
/// In mute mode no output will be produced by the radio chip.
/// @param switchOn The new state of the mute mode. True to switch on, false to switch off.
/// @return void
void SI4705::setMute(bool switchOn) {
  RADIO::setMute(switchOn);

  if (switchOn) {
    // Set mute bits in the fm receiver
    _setProperty(PROP_RX_HARD_MUTE, PROP_RX_HARD_MUTE_BOTH);

#if defined(ELVRADIO)
    // mute the ELV board by using GPO1
    _sendCommand(2, CMD_GPIO_SET, CMD_GPIO_SET_GPO1LEVEL);
#endif

  } else {
    // clear mute bits in the fm receiver
    _setProperty(PROP_RX_HARD_MUTE, 0x00);

#if defined(ELVRADIO)
    // unmute the ELV board by using GPO1
    _sendCommand(2, CMD_GPIO_SET, 0);
#endif
  } // if
} // setMute()


/// Control the softmute mode of the radio chip
/// If switched on the radio output is muted when no sender was found.
/// @param switchOn The new state of the softmute mode. True to switch on, false to switch off.
/// @return void
void SI4705::setSoftMute(bool switchOn) {
  RADIO::setSoftMute(switchOn);

  if (switchOn) {
    // to enable the softmute mode the attenuation is set to 0x10.
    _setProperty(FM_SOFT_MUTE_MAX_ATTENUATION, 0x14);
  } else {
    // to disable the softmute mode the attenuation is set to 0.  
    _setProperty(FM_SOFT_MUTE_MAX_ATTENUATION, 0x00);
  }
} // setSoftMute()


/// BassBoost is not supported by the SI4705 chip.
/// @param switchOn this functions ignores the switchOn parameter and always sets bassBoost to false.
/// @return void
void SI4705::setBassBoost(bool switchOn)
{
  RADIO::setBassBoost(false);
} // setBassBoost()


/// Control the mono mode of the radio chip
/// In mono mode the stereo decoding will be switched off completely  and the noise is typically reduced.
/// @param switchOn The new state of the mono mode. True to switch on, false to switch off.
/// @return void
void SI4705::setMono(bool switchOn)
{
  RADIO::setMono(switchOn);
  if (switchOn) {
    // disable automatic stereo feature
    _setProperty(PROP_FM_BLEND_RSSI_STEREO_THRESHOLD, 127);
    _setProperty(PROP_FM_BLEND_RSSI_MONO_THRESHOLD, 127);

  } else {
    // Automatic stereo feature on.
    _setProperty(PROP_FM_BLEND_RSSI_STEREO_THRESHOLD, 0x0031); // default = 49
    _setProperty(PROP_FM_BLEND_RSSI_MONO_THRESHOLD, 0x001E); // default = 30
  } // if
} // setMono


// ----- Band and frequency control methods -----

/// Start using the new band for receiving.
/// @param newBand The new band to be received.
/// @return void
void SI4705::setBand(RADIO_BAND newBand) {
  if (newBand == RADIO_BAND_FM) {
    // set band boundaries and steps 
    RADIO::setBand(newBand);

    // powering up in FM mode, analog outputs, crystal oscillator, GPO2 enabled for interrupts.
    _sendCommand(3, CMD_POWER_UP, (CMD_POWER_UP_1_XOSCEN | CMD_POWER_UP_1_GPO2OEN | CMD_POWER_UP_1_FUNC_FM), CMD_POWER_UP_2_ANALOGOUT);
    // delay 500 msec when using the crystal oscillator as mentioned in the note from the POWER_UP command.
    delay(500);
    _setProperty(PROP_FM_DEEMPHASIS, PROP_FM_DEEMPHASIS_50); // for Europe 50 deemphasis
    _setProperty(PROP_FM_SEEK_FREQ_SPACING, _freqSteps); // in 100kHz spacing

  } else {
    _sendCommand(1, CMD_POWER_DOWN);

  } // if
} // setBand()



/// Retrieve the real frequency from the chip after manual or automatic tuning.
/// @return RADIO_FREQ the current frequency.
RADIO_FREQ SI4705::getFrequency() {
  _readStatusData(CMD_FM_TUNE_STATUS, 0x03, tuneStatus, sizeof(tuneStatus));
  _freq = (tuneStatus[2] << 8) + tuneStatus[3];
  return (_freq);
}  // getFrequency


/// Start using the new frequency for receiving.\n
/// The new frequency is stored for later retrieval by the base class.\n
/// Because the chip may change the frequency automatically (when seeking)
/// the stored value might not be the current frequency.
/// @param newF The new frequency to be received.
/// @return void
void SI4705::setFrequency(RADIO_FREQ newF) {
  uint8_t status;

  RADIO::setFrequency(newF);
  _sendCommand(5, CMD_FM_TUNE_FREQ, 0, (newF >> 8) & 0xff, (newF)& 0xff, 0);

  // reset the RDSParser
  clearRDS();

  // loop until status ok.
  do {
    status = _readStatus();
  } while (!(status & CMD_GET_INT_STATUS_CTS));
} // setFrequency()


/// Start seek mode upwards.
void SI4705::seekUp(bool toNextSender) {
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
void SI4705::seekDown(bool toNextSender) {
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
uint8_t SI4705::_readStatus()
{
  uint8_t value;

  Wire.beginTransmission(SI4705_ADR);
  Wire.write(CMD_GET_INT_STATUS);
  Wire.endTransmission();

  Wire.requestFrom(SI4705_ADR, 1); // We want to read 1 byte only.
  value = Wire.read();

  return(value);
} // _readStatus()


/// Load status information from to the chip.
void SI4705::_readStatusData(uint8_t cmd, uint8_t param, uint8_t *values, uint8_t len)
{
  Wire.beginTransmission(SI4705_ADR);
  Wire.write(cmd);
  Wire.write(param);

  Wire.endTransmission();
  Wire.requestFrom(SI4705_ADR, (int)len); //We want to read some bytes.

  for (uint8_t n = 0; n < len; n++) {
    //Read in these bytes   
    values[n] = Wire.read();
  } // for
} // _readStatusData()


/// Return a filled RADIO_INFO with the status of the radio features of the chip.
void SI4705::getRadioInfo(RADIO_INFO *info) {
  RADIO::getRadioInfo(info);

  _readStatusData(CMD_FM_TUNE_STATUS, 0x01, tuneStatus, sizeof(tuneStatus));

  info->active = true;
  if (tuneStatus[1] & 0x01) info->tuned = true;

  _readStatusData(CMD_FM_RSQ_STATUS, 0x01, rsqStatus, sizeof(rsqStatus));
  if (rsqStatus[3] & 0x80) info->stereo = true;
  info->rssi = rsqStatus[4];
  info->snr = rsqStatus[5];

  _readStatusData(CMD_FM_RDS_STATUS, 0x05, rdsStatus.buffer, sizeof(rdsStatus));
  if (rdsStatus.resp2 & 0x01) info->rds = true;
} // getRadioInfo()


/// Return a filled AUIO_INFO with the actual audio settings.
void SI4705::getAudioInfo(AUDIO_INFO *info) {
  RADIO::getAudioInfo(info);
} // getAudioInfo()


/// Retrieve the next RDS data if available.
void SI4705::checkRDS()
{
  if (_sendRDS) {
    // fetch the interrupt status first
    uint8_t status = _readStatus();

    // fetch the current RDS data
    _readStatusData(CMD_FM_RDS_STATUS, 0x01, rdsStatus.buffer, sizeof(rdsStatus));

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


// ----- Debug functions -----

/// Send the current values of all registers to the Serial port.
void SI4705::debugStatus()
{
  RADIO::debugStatus();
  _readStatusData(CMD_FM_TUNE_STATUS, 0x03, tuneStatus, sizeof(tuneStatus));

  Serial.print("Tune-Status: ");
  Serial.print(tuneStatus[0], HEX); Serial.print(' ');
  Serial.print(tuneStatus[1], HEX); Serial.print(' ');

  Serial.print("TUNE:"); Serial.print((tuneStatus[2] << 8) + tuneStatus[3]); Serial.print(' ');
  // RSSI and SNR when tune is complete (not the actual one ?)
  Serial.print("RSSI:"); Serial.print(tuneStatus[4]); Serial.print(' ');
  Serial.print("SNR:");  Serial.print(tuneStatus[5]); Serial.print(' ');
  Serial.print("MULT:"); Serial.print(tuneStatus[6]); Serial.print(' ');
  Serial.print(tuneStatus[7]); Serial.print(' ');
  Serial.println();

  Serial.print("RSQ-Status: ");
  _readStatusData(CMD_FM_RSQ_STATUS, 0x01, rsqStatus, sizeof(rsqStatus));
  Serial.print(rsqStatus[0], HEX); Serial.print(' ');
  Serial.print(rsqStatus[1], HEX); Serial.print(' ');
  Serial.print(rsqStatus[2], HEX); Serial.print(' '); if (rsqStatus[2] & 0x08) Serial.print("SMUTE ");
  Serial.print(rsqStatus[3], HEX); Serial.print(' '); if (rsqStatus[3] & 0x80) Serial.print("STEREO ");
  // The current RSSI and SNR.
  Serial.print("RSSI:"); Serial.print(rsqStatus[4]); Serial.print(' ');
  Serial.print("SNR:");  Serial.print(rsqStatus[5]); Serial.print(' ');
  Serial.print(rsqStatus[7], HEX); Serial.print(' ');
  Serial.println();

  Serial.print("RDS-Status: ");
  _readStatusData(CMD_FM_RDS_STATUS, 0x01, rdsStatus.buffer, sizeof(rdsStatus));
  for (uint8_t n = 0; n < 12; n++) {
    Serial.print(rsqStatus[n], HEX); Serial.print(' ');
  } // for
  Serial.println();

  // AGC settings and status
  Serial.print("AGC-Status: ");
  _readStatusData(CMD_FM_AGC_STATUS, 0x01, agcStatus, sizeof(agcStatus));
  Serial.print(agcStatus[0], HEX); Serial.print(' ');
  Serial.print(agcStatus[1], HEX); Serial.print(' ');
  Serial.print(agcStatus[2], HEX); Serial.print(' ');
  Serial.println();

} // debugStatus


/// wait until the current seek and tune operation is over.
void SI4705::_waitEnd() {
  DEBUG_FUNC0("_waitEnd");
} // _waitEnd()


/// Send an array of bytes to the radio chip
void SI4705::_sendCommand(int cnt, int cmd, ...) {
  if (cnt > 8) {
    // see AN332: "Writing more than 8 bytes results in unpredictable device behavior."
    Serial.println("error: _sendCommand: too much parameters!");

  } else {
    Wire.beginTransmission(SI4705_ADR);
    Wire.write(cmd);

    va_list params;
    va_start(params, cmd);

    for (uint8_t i = 1; i < cnt; i++) {
      uint8_t c = va_arg(params, int);
      Wire.write(c);
    }
    Wire.endTransmission();
    va_end(params);

    // wait for Command being processed
    Wire.requestFrom(SI4705_ADR, 1); // We want to read the status byte.
    _status = Wire.read();
  } // if

} // _sendCommand()


/// Set a property in the radio chip
void SI4705::_setProperty(uint16_t prop, uint16_t value) {
  Wire.beginTransmission(SI4705_ADR);
  Wire.write(CMD_SET_PROPERTY);
  Wire.write(0);

  Wire.write(prop >> 8);
  Wire.write(prop & 0x00FF);
  Wire.write(value >> 8);
  Wire.write(value & 0x00FF);

  Wire.endTransmission();

  Wire.requestFrom(SI4705_ADR, 1); // We want to read the status byte.
  _status = Wire.read();
} // _setProperty()


// ----- internal functions -----

// The End.


