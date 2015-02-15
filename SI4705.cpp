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
/// History:
/// --------
/// * 05.12.2014 created.
/// * 30.01.2015 working first version.
/// * 07.02.2015 cleanup
/// * 15.02.2015 RDS is working.

#include <Arduino.h>
#include <Wire.h>     // The chip is controlled via the standard Arduiino Wire library and the IIC/I2C bus.

#include <RADIO.h>    // Include the common radio library interface
#include <SI4705.h>

// ----- Definitions for the Wire communication

#define SI4705_ADR 0x63  ///< The I2C address of SI4705 is 0x61 or 0x63

/// Uncomment this definition when not using the ELV radio board.
/// There is a special mute implementation.
#define ELVRADIO


// ----- Radio chip specific definitions including the registers

// Commands & parameter Constants

#define	CMD_POWER_UP	    0x01	//	Power up device and mode selection.
#define	CMD_POWER_UP_1_FUNC_FM 0x00
#define	CMD_POWER_UP_1_XOSCEN  0x10
#define	CMD_POWER_UP_2_GPO2OEN 0x40
#define	CMD_POWER_UP_2_ANALOGOUT 0x05

#define	CMD_GET_REV	        0x10	//	Returns revision information on the device.
#define	CMD_POWER_DOWN	    0x11	//	Power down device.

#define	CMD_SET_PROPERTY	0x12	//	Sets the value of a property.
#define	CMD_GET_PROPERTY	0x13	//	Retrieves a property’s value.
#define	CMD_GET_INT_STATUS	0x14	//	Reads interrupt status bits.
#define	CMD_PATCH_ARGS*	    0x15	//	Reserved command used for patch file downloads.
#define	CMD_PATCH_DATA*	    0x16	//	Reserved command used for patch file downloads.
#define	CMD_FM_TUNE_FREQ	0x20	//	Selects the FM tuning frequency.
#define	CMD_FM_SEEK_START	0x21	//	Begins searching for a valid frequency.
#define	CMD_FM_TUNE_STATUS	0x22	//	Queries the status of previous FM_TUNE_FREQ or FM_SEEK_START command.
#define	CMD_FM_RSQ_STATUS	0x23	//	Queries the status of the Received Signal Quality (RSQ) of the current channel
#define	CMD_FM_RDS_STATUS	0x24	//	Returns RDS information for current channel and reads an entry from RDS FIFO.
#define	CMD_FM_AGC_STATUS	0x27	//	Queries the current AGC settings All
#define	CMD_FM_AGC_OVERRIDE	0x28	//	Override AGC setting by disabling and forcing it to a fixed value

#define	CMD_GPIO_CTL	    0x80	//	Configures GPO1, 2, and 3 as output or Hi-Z.
#define	CMD_GPIO_SET	    0x81	//	Sets GPO1, 2, and 3 output level (low or high).

#define PROP_GPO_IEN        0x0001
#define PROP_GPO_IEN_STCIEN   0x01
#define PROP_GPO_IEN_RDSIEN   0x04

#define PROP_FM_DEEMPHASIS   0x1100
#define PROP_FM_DEEMPHASIS_50  0x01

// setup the antenna input pin
#define PROP_FM_ANTENNA_INPUT 0x1107
#define PROP_FM_ANTENNA_INPUT_FMI   0x00
#define PROP_FM_ANTENNA_INPUT_SHORT 0x01

#define PROP_FM_SEEK_FREQ_SPACING 0x1402

// rds properties
#define PROP_RDS_INTERRUPT_SOURCE 0x1500
#define PROP_RDS_INTERRUPT_SOURCE_RDSRECV 0x01

#define PROP_RDS_INT_FIFO_COUNT 0x1501

#define PROP_RDS_CONFIG 0x1502


#define PROP_RX_VOLUME 0x4000

#define PROP_X0 0x1500
#define PROP_X1 0x1501
#define PROP_X2 0x1502

// #define PROP_FM_BLEND_STEREO_THRESHOLD      0x1105
// #define PROP_FM_BLEND_MONO_THRESHOLD      0x1106


#define PROP_FM_BLEND_RSSI_STEREO_THRESHOLD 0x1800 
#define PROP_FM_BLEND_RSSI_MONO_THRESHOLD 0x1801

// FM_MAX_TUNE_ERROR
// #define FM_MAX_TUNE_ERROR      0x1108

#define PROP_RX_HARD_MUTE 0x4001 

//Write : 20 0 22 2E 0
//Write : 22 3

//Write : 12 0 0 1 0 5  xx
//Write : 12 0 0 1 0 1  Property 0x0001. GPO_IEN: STCIEN

//Write : 12 0 15 0 0 1
//Write : 12 0 15 1 0 4
//Write : 12 0 15 2 EF 1
//Write : 20 0 22 E2 0
//Write : 12 0 40 0 0 20
//Write : 81 0
//Write : 80 2

// ----- implement

// initialize the extra variables in SI4703
SI4705::SI4705() {
}

// initialize all internals.
bool SI4705::init() {
  bool result = false; // no chip found yet.
  DEBUG_FUNC0("init");

  Wire.begin(); //Now that the unit is reset and I2C inteface mode, we need to begin I2C

  // power up in FM mode and analog outputs.
  _sendCommand(3, CMD_POWER_UP, CMD_POWER_UP_1_XOSCEN | CMD_POWER_UP_2_GPO2OEN | CMD_POWER_UP_1_FUNC_FM, CMD_POWER_UP_2_ANALOGOUT); // 
  // delay(200);

  // set some common properties
  _setProperty(PROP_FM_ANTENNA_INPUT, PROP_FM_ANTENNA_INPUT_SHORT);
  _setProperty(PROP_FM_DEEMPHASIS, PROP_FM_DEEMPHASIS_50); // for Europe 50 deemphasis
  _setProperty(PROP_FM_SEEK_FREQ_SPACING, 10); // in 100kHz spacing

  // set volume to 0 and mute so no noise gets out here.
  _setProperty(PROP_RX_VOLUME, 0);

  // Set mute bits in the fm receiver
  _setProperty(PROP_RX_HARD_MUTE, 0x03);

#if defined(ELVRADIO)
  // mute the ELV board by using GPO1
  _sendCommand(2, CMD_GPIO_CTL, 0x02);
  _sendCommand(2, CMD_GPIO_SET, 0x02);
#endif

  _setProperty(PROP_GPO_IEN, PROP_GPO_IEN_STCIEN); //  | PROP_GPO_IEN_RDSIEN ????


  // _sendCommand(2, 0x22, 0x03);

  /// RDS
  //_setProperty(0x1500, 0x0001);
  //_setProperty(0x1501, 0x0004);
  //_setProperty(0x1502, 0xEF01);

  _setProperty(PROP_RDS_INTERRUPT_SOURCE, PROP_RDS_INTERRUPT_SOURCE_RDSRECV); // Set the CTS status bit after receiving RDS data.
  _setProperty(PROP_RDS_INT_FIFO_COUNT, 4);
  _setProperty(PROP_RDS_CONFIG, 0xFF01); // accept all correctable data and enable rds

  return(result);
} // init()


// switch the power off
void SI4705::term()
{
  DEBUG_FUNC0("term");
  _sendCommand(1, CMD_POWER_DOWN);
} // term


// ----- Volume control -----

/// set the volume in the range 0..15. This is mapped to the internal range 0..64
void SI4705::setVolume(uint8_t newVolume)
{
  DEBUG_FUNC1("setVolume", newVolume);
  if (newVolume > 15) newVolume = 15;

  RADIO::setVolume(newVolume);
  _setProperty(PROP_RX_VOLUME, newVolume * 4);
} // setVolume()


/// BassBoost is not supported by the SI4705 chip.
void SI4705::setBassBoost(bool switchOn)
{
  DEBUG_FUNC1("setBassBoost", switchOn);
  RADIO::setBassBoost(false);
} // setBassBoost()


// Mono / Stereo
void SI4705::setMono(bool switchOn)
{
  DEBUG_FUNC1("setMono", switchOn);
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


// Switch mute mode.
void SI4705::setMute(bool switchOn) {
  DEBUG_FUNC1("setMute", switchOn);
  RADIO::setMute(switchOn);

  if (switchOn) {
    // Set mute bits in the fm receiver
    _setProperty(PROP_RX_HARD_MUTE, 0x03);

#if defined(ELVRADIO)
    // mute the ELV board by using GPO1
    _sendCommand(2, CMD_GPIO_CTL, 0x02);
    _sendCommand(2, CMD_GPIO_SET, 0x02);
#endif

  } else {
    // clear mute bits in the fm receiver
    _setProperty(PROP_RX_HARD_MUTE, 0x00);

#if defined(ELVRADIO)
    // unmute the ELV board by using GPO1
    _sendCommand(2, CMD_GPIO_CTL, 0x02);
    _sendCommand(2, CMD_GPIO_SET, 0x00);
#endif
  } // if
} // setMute()


// ----- Band and frequency control methods -----

// tune to new band.
void SI4705::setBand(RADIO_BAND newBand) {
  DEBUG_FUNC1("setBand", newBand);
  if (newBand == RADIO_BAND_FM) {
    RADIO::setBand(newBand);
    _sendCommand(3, CMD_POWER_UP, CMD_POWER_UP_1_XOSCEN | CMD_POWER_UP_2_GPO2OEN | CMD_POWER_UP_1_FUNC_FM, CMD_POWER_UP_2_ANALOGOUT);
    delay(200);
    _setProperty(PROP_FM_DEEMPHASIS, PROP_FM_DEEMPHASIS_50); // for Europe 50 deemphasis

    _freqSteps = 10;
    _setProperty(PROP_FM_SEEK_FREQ_SPACING, _freqSteps); // in 100kHz spacing

  } else {
    _sendCommand(1, CMD_POWER_DOWN);

  } // if



} // setBand()


/**
 * @brief Retrieve the real frequency from the chip after automatic tuning.
 * @return RADIO_FREQ the current frequency.
 */
RADIO_FREQ SI4705::getFrequency() {
  DEBUG_FUNC0("getFrequency");
  _readStatusData(CMD_FM_TUNE_STATUS, 0x03, tuneStatus, sizeof(tuneStatus));
  _freq = (tuneStatus[2] << 8) + tuneStatus[3];

  return (_freq);
}  // getFrequency


/**
 * @brief Change the frequency in the chip.
 * @param newF
 * @return void
 */
void SI4705::setFrequency(RADIO_FREQ newF) {
  DEBUG_FUNC1("setFrequency", newF);
  _sendCommand(5, CMD_FM_TUNE_FREQ, 0, (newF >> 8) & 0xff, (newF)& 0xff, 0);
} // setFrequency()


// start seek mode upwards
void SI4705::seekUp(bool toNextSender) {
  DEBUG_FUNC0("seekUp");
  _seek(true);
} // seekUp()


// start seek mode downwards
void SI4705::seekDown(bool toNextSender) {
  _seek(false);
} // seekDown()



// Load status information from to the chip
// only 8 byte status requests are supported. 
uint8_t SI4705::_readStatus()
{
  uint8_t value;

  Wire.beginTransmission(SI4705_ADR);
  Wire.write(CMD_GET_INT_STATUS);
  Wire.endTransmission();

  Wire.requestFrom(SI4705_ADR, 1); // We want to read 1 byte only.
  value = Wire.read();

  // DEBUG_VALX("Status", value);
  return(value);
} // _readStatus()


// Load status information from to the chip
// only 8 byte status requests are supported. 
void SI4705::_readStatusData(uint8_t cmd, uint8_t param, uint8_t *values, uint8_t len)
{
  //DEBUG_FUNC0("_readStatusData");

  Wire.beginTransmission(SI4705_ADR);
  Wire.write(cmd);
  Wire.write(param);

  Wire.endTransmission();

  // Serial.print("_read(");
  // long startTime = millis();
  Wire.requestFrom(SI4705_ADR, (int)len); //We want to read some bytes.

  //if (millis() - startTime > timeout) {
  //  return -1;
  //}

  for (uint8_t n = 0; n < len; n++) {
    //Read in these bytes   
    values[n] = Wire.read();
  } // for

  //// clean buffer
  //while (Wire.available() > 0)
  //{
  //  Wire.read();
  //}

} // _readStatusData()


// write a register value using 2 bytes into the Wire.
void SI4705::_write16(uint16_t val)
{
  Wire.write(val >> 8); Wire.write(val & 0xFF);
} // _write16


// read a register value using 2 bytes in a row
uint16_t SI4705::_read16(void)
{
  uint8_t hiByte = Wire.read();
  uint8_t loByte = Wire.read();
  return((hiByte << 8) + loByte);
} // _read16


void SI4705::getRadioInfo(RADIO_INFO *info) {
  DEBUG_FUNC0("getRadioInfo");

  RADIO::getRadioInfo(info);

  _readStatusData(CMD_FM_TUNE_STATUS, 0x01, tuneStatus, sizeof(tuneStatus));

  info->active = true;
  info->rssi = tuneStatus[4];
  // TODO: bool rds;
  if (tuneStatus[1] & 0x01) info->tuned = true;
  // TODO: bool mono;
  // TODO: bool stereo;

} // getRadioInfo()


void SI4705::getAudioInfo(AUDIO_INFO *info) {
  RADIO::getAudioInfo(info);
} // getAudioInfo()


void SI4705::checkRDS()
{
  // DEBUG_FUNC0("checkRDS");
  if (_sendRDS) {

    // fetch the interrupt status first
    uint8_t status = _readStatus();

    // fetch the current RDS data
    _readStatusData(CMD_FM_RDS_STATUS, 0x01, rdsStatus, sizeof(rdsStatus));

    if ((rdsStatus[2] = 0x01) && (rdsStatus[3]) && (rdsStatus[12] == 0)) {
      // RDS is in sync, it's a complete entry and no errors
      _sendRDS((rdsStatus[4] << 8) + rdsStatus[5],
        (rdsStatus[6] << 8) + rdsStatus[7],
        (rdsStatus[8] << 8) + rdsStatus[9], 
        (rdsStatus[10] << 8) + rdsStatus[11]);

    } // if

  } // if _sendRDS

} // checkRDS

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
  Serial.print("RSSI:"); Serial.print(tuneStatus[4]); Serial.print(' ');
  Serial.print("SNR:");  Serial.print(tuneStatus[5]); Serial.print(' ');
  Serial.print("MULT:"); Serial.print(tuneStatus[6]); Serial.print(' ');
  Serial.print(tuneStatus[7]); Serial.print(' ');
  Serial.println();

  _readStatusData(CMD_FM_RSQ_STATUS, 0x01, rsqStatus, sizeof(rsqStatus));
  Serial.print("RSQ-Status: ");
  Serial.print(rsqStatus[0], HEX); Serial.print(' ');
  Serial.print(rsqStatus[1], HEX); Serial.print(' ');
  Serial.print(rsqStatus[2], HEX); Serial.print(' ');
  Serial.print(rsqStatus[3], HEX); Serial.print(' ');
  Serial.print("RSSI:"); Serial.print(tuneStatus[4]); Serial.print(' ');
  Serial.print("SNR:");  Serial.print(tuneStatus[5]); Serial.print(' ');
  Serial.print("MULT:"); Serial.print(tuneStatus[6]); Serial.print(' ');
  Serial.print(rsqStatus[7], HEX); Serial.print(' ');

  _readStatusData(CMD_FM_RDS_STATUS, 0x01, rdsStatus, sizeof(rdsStatus));
  Serial.print("RDS-Status: ");
  for (uint8_t n = 0; n < 12; n++) {
    Serial.print(rsqStatus[n], HEX); Serial.print(' ');
  } // for


} // debugStatus


/// Seeks out the next available station
void SI4705::_seek(bool seekUp) {
  DEBUG_FUNC0("_seek");
} // _seek


/// wait until the current seek and tune operation is over.
void SI4705::_waitEnd() {
  DEBUG_FUNC0("_waitEnd");
} // _waitEnd()


/// Send an array of bytes to the radio chip
void SI4705::_sendCommand(int cnt, int cmd, ...) {
  Serial.print("_send(");

  if (cnt > 8) {
    // see AN332: "Writing more than 8 bytes results in unpredictable device behavior."
    Serial.println("error: too much parameters!");

  } else {
    Wire.beginTransmission(SI4705_ADR);
    Wire.write(cmd);
    Serial.print(cmd, HEX); Serial.print(' ');

    va_list params;
    va_start(params, cmd);

    for (uint8_t i = 1; i < cnt; i++) {
      uint8_t c = va_arg(params, int);
      Serial.print(c, HEX); Serial.print(' ');
      Wire.write(c);
    }
    Wire.endTransmission();
    va_end(params);

    Serial.print(") >>");

    // wait for Command being processed
    Wire.requestFrom(SI4705_ADR, 1); // We want to read the status byte.
    _status = Wire.read();

    Serial.println(_status, HEX);

    delay(100);
  } // if

} // _sendCommand()


/// Set a property in the radio chip
void SI4705::_setProperty(uint16_t prop, uint16_t value) {
  DEBUG_FUNC2X("_setProperty", prop, value);

  Wire.beginTransmission(SI4705_ADR);
  Wire.write(CMD_SET_PROPERTY);
  Wire.write(0);

  Wire.write(prop >> 8);
  Wire.write(prop & 0x00FF);
  Wire.write(value >> 8);
  Wire.write(value & 0x00FF);

  Wire.endTransmission();

  int status;
  Wire.requestFrom(SI4705_ADR, 1); // We want to read the status byte.
  status = Wire.read();

  Serial.print(">>");
  Serial.println(status, HEX);


  delay(100);
} // _setProperty()


// ----- internal functions -----

// The End.


