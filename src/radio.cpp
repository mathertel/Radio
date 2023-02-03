/** \file Radio.cpp
 * \brief Library implementation for the radio libraries to control radio chips.
 *
 * \author Matthias Hertel, http://www.mathertel.de
 * \copyright Copyright (c) 2014 by Matthias Hertel.\n
 * This work is licensed under a BSD style license.\n
 * See http://www.mathertel.de/License.aspx
 * More documentation and source code is available at http://www.mathertel.de/Arduino
 *
 * ChangeLog see: radio.h
 */

#include <Arduino.h>
#include <Wire.h>
#include <radio.h>


// ----- Register Definitions -----

// no chip-registers without a chip.

// ----- implement

/// Setup the radio object and initialize private variables to 0.
/// Don't change the radio chip (yet).
RADIO::RADIO() {
}  // RADIO()


void RADIO::setup(int feature, int value) {
  if (feature == RADIO_RESETPIN) {
    _resetPin = value;
  } else if ((feature == RADIO_I2CADDRESS) && (value > 0)) {
    _i2caddr = value;
  } else if ((feature == RADIO_ANTENNA) && (value > 0)) {
    _antennaOption = value;
  } else if ((feature == RADIO_FMSPACING) && (value > 0)) {
    _fmSpacing = value;
  } else if ((feature == RADIO_DEEMPHASIS) && (value > 0)) {
    _deEmphasis = value;
  }

}  // setup()


/// The RADIO class doesn't implement a concrete chip so nothing has to be initialized.
bool RADIO::initWire(TwoWire &port) {
  DEBUG_FUNC0("RADIO::initWire");

  _i2cPort = &port;
  return (this->init());
}  // initWire()


/// The RADIO class doesn't implement a concrete chip so nothing has to be initialized.
bool RADIO::init() {
  if (_resetPin >= 0) {
    // create a reset impulse
    pinMode(_resetPin, OUTPUT);
    digitalWrite(_resetPin, LOW);  // Put chip into reset
    delay(5);
    digitalWrite(_resetPin, HIGH);  // Bring chip out of reset
    delay(5);
  }
  return (false);
}  // init()


/// The RADIO class doesn't implement a concrete chip so nothing has to be initialized.
void RADIO::term() {
}  // term()


// ----- Volume control -----

void RADIO::setVolume(int8_t newVolume) {
  _volume = constrain(newVolume, 0, _maxVolume);
}  // setVolume()


int8_t RADIO::getVolume() {
  return (_volume);
}  // getVolume()


int8_t RADIO::getMaxVolume() {
  return (_maxVolume);
}  // getMaxVolume()


// ----- bass boost control -----

/// Control the bass boost mode of the radio chip.
/// The base implementation ony stores the value to the internal variable.
/// @param switchOn true to switch bassBoost mode on, false to switch bassBoost mode off.
void RADIO::setBassBoost(bool switchOn) {
  DEBUG_FUNC1("setBassBoost", switchOn);
  _bassBoost = switchOn;
}  // setBassBoost()


/// Retrieve the current bass boost mode setting.
/// The base implementation returns only the value in the internal variable.
bool RADIO::getBassBoost() {
  return (_bassBoost);
}  // getBassBoost()


// ----- mono control -----

/// The base implementation ony stores the value to the internal variable.
void RADIO::setMono(bool switchOn) {
  DEBUG_FUNC1("setMono", switchOn);
  _mono = switchOn;
}  // setMono()


/// The base implementation returns only the value in the internal variable.
bool RADIO::getMono() {
  return (_mono);
}  // getMono()


// ----- mute control -----

/// The base implementation ony stores the value to the internal variable.
void RADIO::setMute(bool switchOn) {
  _mute = switchOn;
}  // setMute()


/// The base implementation returns only the value in the internal variable.
bool RADIO::getMute() {
  return (_mute);
}  // getMute()


// ----- softmute control -----

/// The base implementation ony stores the value to the internal variable.
void RADIO::setSoftMute(bool switchOn) {
  DEBUG_FUNC1("setSoftMute", switchOn);
  _softMute = switchOn;
}  // setSoftMute()


/// The base implementation returns only the value in the internal variable.
bool RADIO::getSoftMute() {
  return (_softMute);
}  // getSoftMute()


// ----- receiver control -----

// some implementations to return internal variables if used by concrete chip implementations

/// Start using the new band for receiving.
void RADIO::setBand(RADIO_BAND newBand) {
  DEBUG_FUNC1("setBand", newBand);
  _band = newBand;
  if (newBand == RADIO_BAND_FM) {
    _freqLow = 8700;
    _freqHigh = 10800;
    _freqSteps = 10;  // 20 in USA ???

  } else if (newBand == RADIO_BAND_FMWORLD) {
    _freqLow = 7600;
    _freqHigh = 10800;
    _freqSteps = 10;
  }  // if
}  // setBand()


/// Start using the new frequency for receiving.
/// The new frequency is stored for later retrieval.
void RADIO::setFrequency(RADIO_FREQ newFreq) {
  DEBUG_FUNC1("setFrequency", newFreq);
  _freq = newFreq;
}  // setFrequency()


void RADIO::setBandFrequency(RADIO_BAND newBand, RADIO_FREQ newFreq) {
  setBand(newBand);
  setFrequency(newFreq);
}  // setBandFrequency()


void RADIO::seekUp(bool) {}
void RADIO::seekDown(bool) {}

RADIO_BAND RADIO::getBand() {
  return (_band);
}
RADIO_FREQ RADIO::getFrequency() {
  return (_freq);
}
RADIO_FREQ RADIO::getMinFrequency() {
  return (_freqLow);
}
RADIO_FREQ RADIO::getMaxFrequency() {
  return (_freqHigh);
}
RADIO_FREQ RADIO::getFrequencyStep() {
  return (_freqSteps);
}


/// Return all the Radio settings.
/// This implementation only knows some values from the last settings.
void RADIO::getRadioInfo(RADIO_INFO *info) {
  // set everything to false and 0.
  memset(info, 0, sizeof(RADIO_INFO));
  // info->tuned = false;
  // info->rds = false;
  // info->stereo = false;

  // use current settings
  info->mono = _mono;

}  // getRadioInfo()


/// Return current settings as far as no chip is required.
/// When using the radio::setXXX methods, no chip specific implementation is needed.
void RADIO::getAudioInfo(AUDIO_INFO *info) {
  // set everything to false and 0.
  memset(info, 0, sizeof(AUDIO_INFO));

  // use current settings
  info->volume = _volume;
  info->mute = _mute;
  info->softmute = _softMute;
  info->bassBoost = _bassBoost;
}  // getAudioInfo()


/// In the general radio implementation there is no chip for RDS.
/// This function needs to be implemented for radio chips with RDS receiving functionality.
void RADIO::checkRDS() { /* no chip : nothing to check */
}


/// Send a 0.0.0.0 to the RDS receiver if there is any attached.
/// This is to point out that there is a new situation and all existing data should be invalid from now on.
void RADIO::clearRDS() {
  if (_sendRDS)
    _sendRDS(0, 0, 0, 0);
}  // clearRDS()


// send valid and good data to the RDS processor via newFunction
// remember the RDS function
void RADIO::attachReceiveRDS(receiveRDSFunction newFunction) {
  _sendRDS = newFunction;
}  // attachReceiveRDS()


// format the current frequency for display and printing
void RADIO::formatFrequency(char *s, uint8_t length) {
  RADIO_BAND b = getBand();
  RADIO_FREQ f = getFrequency();

  if ((s != NULL) && (length > 10)) {
    *s = '\0';

    if ((b == RADIO_BAND_FM) || (b == RADIO_BAND_FMWORLD)) {
      // " ff.ff MHz" or "fff.ff MHz"
      int16_to_s(s, (uint16_t)f);

      // insert decimal point
      s[5] = s[4];
      s[4] = s[3];
      s[3] = '.';

      // append units
      strcpy(s + 6, " MHz");
    }  // if

    //     f = _freqLow + (channel * _bandSteps);
    //     if (f < 10000) Serial.write(' ');
    //     Serial.print(f / 100); Serial.print('.'); Serial.print(f % 100); Serial.print(" MHz ");
  }  // if

}  // formatFrequency()


/**
 * Enable debugging information on Serial port.
 * This is for logging on a higher level than i2c data transport.
 * @param enable true to switch logging on.
 */
void RADIO::debugEnable(bool enable) {
  _debugEnabled = enable;
}  // debugEnable()


// print out all radio information
void RADIO::debugRadioInfo() {
  RADIO_INFO info;
  this->getRadioInfo(&info);

  Serial.print(info.rds ? " RDS" : " ---");
  Serial.print(info.tuned ? " TUNED" : " -----");
  Serial.print(info.stereo ? " STEREO" : "  MONO ");
  Serial.print("  RSSI: ");
  Serial.print(info.rssi);
  Serial.print("  SNR: ");
  Serial.print(info.snr);
  Serial.println();
}  // debugRadioInfo()


// print out all audio information
void RADIO::debugAudioInfo() {
  AUDIO_INFO info;
  this->getAudioInfo(&info);

  Serial.print(info.mute ? " MUTE" : " ----");
  Serial.print(info.softmute ? " SOFTMUTE" : " --------");
  Serial.print(info.bassBoost ? " BASS" : " ----");
  Serial.println();
}  // debugAudioInfo()


/// The RADIO class doesn't have interesting status information so nothing is sent.
void RADIO::debugStatus() {
  // no output.
}  // debugStatus


/// This is a special format routine used to format frequencies as strings with leading blanks.
/// up to 5 digits only ("    0".."99999")
/// *s MUST be able to hold the characters
void RADIO::int16_to_s(char *s, uint16_t val) {
  uint8_t n = 5;

  while (n > 0) {
    n--;
    if ((n == 4) || (val > 0)) {
      s[n] = '0' + (val % 10);
      val = val / 10;
    } else {
      s[n] = ' ';
    }
  }  // while
}  // int16_to_s()


// ===== Wire Utilities =====

bool RADIO::_wireDebugFlag = false;

/**
 * Enable low level i2c debugging information on Serial port.
 * @param enable true to switch logging on.
 */
void RADIO::_wireDebug(bool enable) {
  _wireDebugEnabled = enable;
  _wireDebugFlag = enable;
}  // _wireDebug()


bool RADIO::_wireExists(TwoWire *port, int address) {
  port->beginTransmission(address);
  uint8_t err = port->endTransmission();
  if (_wireDebugEnabled) {
    Serial.print("_wireExists(");
    Serial.print(address);
    Serial.print("): err=");
    Serial.println(err);
  }
  return (err == 0);
}

// a i2c transmission in one call
void RADIO::_wireWriteTo(TwoWire *port, int address, uint8_t *cmdData, int cmdLen) {
  if (cmdData && cmdLen > 0) {
    // send out command sequence
    port->beginTransmission(address);
    if (_wireDebugFlag) {
      Serial.print("--write(0x");
      Serial.print(address, 16);
      Serial.print("): ");
    }

    for (int i = 0; i < cmdLen; i++) {
      uint8_t d = cmdData[i];
      port->write(d);
      if (_wireDebugFlag) {
        // write a hex value to Serial
        if (d < 16) Serial.print('0');
        Serial.print(d, 16);
        Serial.print(' ');
      }  // if
    }    // for

    port->endTransmission();
  }  // if
}  // _wireWriteTo


// a i2c request in one call
uint8_t RADIO::_wireReadFrom(TwoWire *port, int address, uint8_t *data, int len) {
  uint8_t received = 0;
  if (data && len > 0) {
    while (!received) {
      received = port->requestFrom(address, len);
      if (_wireDebugFlag) {
        Serial.print('[');
        Serial.print(received);
        Serial.print(']');
      }
    }

    uint8_t *d = data;
    for (int n = 0; n < received; n++) {
      *d = port->read();
      if (_wireDebugFlag) {
        // write a hex value to Serial
        if (*d < 16) Serial.print('0');
        Serial.print(*d, 16);
        Serial.print(' ');
      }
      d++;
    }
  }
  return (received);
}  // _wireReadFrom


// write a 16 bit value in High-Low order to the Wire.
void RADIO::_write16HL(TwoWire *port, uint16_t val) {
  port->write(val >> 8);
  port->write(val & 0xFF);
}  // _write16HL


// read a 16 bit value in High-Low order from the Wire.
uint16_t RADIO::_read16HL(TwoWire *port) {
  uint8_t hiByte = port->read();
  uint8_t loByte = port->read();
  return ((hiByte << 8) + loByte);
}  // _read16HL


/**
 * Write and optionally read data on the i2c bus.
 * A debug output can be enabled using _wireDebug().
 * @param port i2c port to be used.
 * @param address i2c address to be used.
 * @param reg the register to be read (1 byte send).
 * @param data buffer array with received data. If this parameter is nullptr no data will be requested.
 * @param len length of data buffer.
 * @return number of register values received.
 */
int RADIO::_wireRead(TwoWire *port, int address, uint8_t reg, uint8_t *data, int len) {
  return (_wireRead(port, address, &reg, 1, data, len));
}  // _wireRead()


/**
 * Write and optionally read data on the i2c bus.
 * A debug output can be enabled using _wireDebug().
 * @param port i2c port to be used.
 * @param address i2c address to be used.
 * @param cmdData array with data to be send.
 * @param cmdLen length of cmdData.
 * @param data buffer array with received data. If this parameter is nullptr no data will be requested.
 * @param len length of data buffer.
 * @return number of register values received.
 */
int RADIO::_wireRead(TwoWire *port, int address, uint8_t *cmdData, int cmdLen, uint8_t *data, int len) {
  int received = 0;

  RADIO::_wireWriteTo(port, address, cmdData, cmdLen);

  // read requested data (when buffer is available)
  if (data) {
    while (received == 0) {
      if (RADIO::_wireDebugFlag) {
        Serial.print(" -> ");
      }
      received = RADIO::_wireReadFrom(port, address, data, len);
      if (!(*data & 0x80))
        received = 0;
    }

    if (RADIO::_wireDebugFlag) {
      Serial.println('.');
    }
  }  // if (data)

  return (received);
}  // _wireRead()


/// Prints a byte as 2 character hexadecimal code with leading zeros.
void RADIO::_printHex2(uint8_t val) {
  Serial.print(' ');
  if (val <= 0x000F)
    Serial.print('0');  // if less 2 Digit
  Serial.print(val, HEX);
}  // _printHex2


/// Prints a word as 4 character hexadecimal code with leading zeros.
void RADIO::_printHex4(uint16_t val) {
  Serial.print(' ');
  if (val <= 0x000F)
    Serial.print('0');  // if less 2 Digit
  if (val <= 0x00FF)
    Serial.print('0');  // if less 3 Digit
  if (val <= 0x0FFF)
    Serial.print('0');  // if less 4 Digit
  Serial.print(val, HEX);
}  // _printHex4

// The End.
