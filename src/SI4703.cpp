///
/// \file SI4703.cpp
/// \brief Implementation for the radio library to control the SI4703 radio chip.
///
/// \author Matthias Hertel, http://www.mathertel.de
/// \copyright Copyright (c) 2014-2015 by Matthias Hertel.\n
/// This work is licensed under a BSD style license.\n
/// See http://www.mathertel.de/License.aspx
///
/// This library enables the use of the Radio Chip SI4703.
///
/// More documentation is available at http://www.mathertel.de/Arduino
/// Source Code is available on https://github.com/mathertel/Radio
///
/// History, see <SI4703.h>

#include <Arduino.h>
#include <Wire.h>  // The chip is controlled via the standard Arduiino Wire library and the IIC/I2C bus.

#include <radio.h>  // Include the common radio library interface
#include <SI4703.h>

// ----- Definitions for the Wire communication

#define SI4703_ADR 0x10  // 0b._001.0000 = I2C address of Si4703 - note that the Wire function assumes non-left-shifted I2C address, not 0b.0010.000W
#define I2C_FAIL_MAX 10  // This is the number of attempts we will try to contact the device before erroring out


// ----- Radio chip specific definitions including the registers

// Register Definitions -----

// Define the register names
#define SI4703_DEVICEID 0x00
#define SI4703_CHIPID 0x01
#define POWERCFG 0x02
#define CHANNEL 0x03
#define SYSCONFIG1 0x04
#define SYSCONFIG2 0x05
#define SYSCONFIG3 0x06
#define STATUSRSSI 0x0A
#define READCHAN 0x0B
#define RDSA 0x0C
#define RDSB 0x0D
#define RDSC 0x0E
#define RDSD 0x0F

// Register 0x02 - POWERCFG
#define DSMUTE 15
#define DMUTE 14
#define SETMONO 13
#define RDSMODE 11
#define SKMODE 10
#define SEEKUP 9
#define SEEK 8

// Register 0x03 - CHANNEL
#define TUNE 15

// Register 0x04 - SYSCONFIG1
#define DEEMPHASIS50 0x0800
#define RDS 12
#define DE 11


// Register 0x05 - SYSCONFIG2
#define SEEKTH_MASK 0xFF00
#define SEEKTH_MIN 0x0000
#define SEEKTH_MID 0x1000
#define SEEKTH_MAX 0x7F00

#define FMSPACE_MASK 0x0030
#define FMSPACE_50 0x0020
#define FMSPACE_100 0x0010
#define FMSPACE_200 0x0000

#define VOLUME_MASK 0x000F

// Register 0x06 - SYSCONFIG3
#define SKSNR_MASK 0x00F0
#define SKSNR_OFF 0x0000
#define SKSNR_MIN 0x0010
#define SKSNR_MID 0x0030
#define SKSNR_MAX 0x0070

#define SKCNT_MASK 0x000F
#define SKCNT_OFF 0x0000
#define SKCNT_MIN 0x000F
#define SKCNT_MID 0x0003
#define SKCNT_MAX 0x0001


// Register 0x0A - STATUSRSSI
#define RDSR 0x8000  ///< RDS ready
#define STC 0x4000   ///< Seek Tune Complete
#define SFBL 0x2000  ///< Seek Fail Band Limit
#define AFCRL 0x1000
#define RDSS 0x0800  ///< RDS syncronized
#define SI 0x0100    ///< Stereo Indicator
#define RSSI 0x00FF

// ----- implement

// initialize the extra variables in SI4703
SI4703::SI4703() {
  _i2caddr = 0x10;  // standard address of chip
}

void SI4703::setup(int feature, int value) {
  RADIO::setup(feature, value);
  if (feature == RADIO_SDAPIN) {
    _sdaPin = value;
  }  // if
}  // setup()


// initialize all internals.
bool SI4703::init() {
  bool found = false;  // no chip found yet.
  DEBUG_FUNC0("SI4703::init");

  // To get the Si4703 into 2-wire mode, SEN needs to be high and SDIO needs to be low after a reset
  // The breakout board has SEN pulled high, but also has SDIO pulled high. Therefore, after a normal power up
  // The Si4703 will be in an unknown state. RST must be controlled

  if (_sdaPin >= 0) {
    pinMode(_sdaPin, OUTPUT);    // SDIO is connected to SDA for I2C
    digitalWrite(_sdaPin, LOW);  // A low SDA during reset indicates a 2-wire interface
    delay(5);
  }

  RADIO::init();  // will create reset impulse

  _i2cPort->begin();  // Now that the unit is reset and I2C inteface mode, we need to begin I2C
  found = RADIO::_wireExists(_i2cPort, _i2caddr);

  _readRegisters();  // Read the current register set
  // registers[0x07] = 0xBC04; //Enable the oscillator, from AN230 page 9, rev 0.5 (DOES NOT WORK, wtf Silicon Labs datasheet?)
  registers[0x07] = 0x8100;  // Enable the oscillator, from AN230 page 9, rev 0.61 (works)
  _saveRegisters();          // Update

  delay(500);  // Wait for clock to settle - from AN230 page 9

  return (found);
}  // init()


// switch the power off
void SI4703::term() {
  DEBUG_FUNC0("SI4703::term");
  RADIO::term();
}  // term


// ----- Volume control -----

void SI4703::setVolume(int8_t newVolume) {
  DEBUG_FUNC1("setVolume", newVolume);
  if (newVolume > 15)
    newVolume = 15;
  _readRegisters();                         // Read the current register set
  registers[SYSCONFIG2] &= ~(VOLUME_MASK);  // Clear volume bits
  registers[SYSCONFIG2] |= newVolume;       // Set new volume
  _saveRegisters();                         // Update
  RADIO::setVolume(newVolume);
}  // setVolume()


// Mono / Stereo
void SI4703::setMono(bool switchOn) {
  DEBUG_FUNC1("setMono", switchOn);
  RADIO::setMono(switchOn);
  _readRegisters();  // Read the current register set
  if (switchOn) {
    registers[POWERCFG] |= (1 << SETMONO);  // set force mono bit
  } else {
    registers[POWERCFG] &= ~(1 << SETMONO);  // clear force mono bit
  }                                          // if
  _saveRegisters();
}  // setMono


/// Switch mute mode.
void SI4703::setMute(bool switchOn) {
  DEBUG_FUNC1("setMute", switchOn);
  RADIO::setMute(switchOn);

  if (switchOn) {
    registers[POWERCFG] &= ~(1 << DMUTE);  // clear mute bit
  } else {
    registers[POWERCFG] |= (1 << DMUTE);  // set mute bit
  }                                       // if
  _saveRegisters();

}  // setMute()


/// Switch soft mute mode.
void SI4703::setSoftMute(bool switchOn) {
  DEBUG_FUNC1("setSoftMute", switchOn);
  RADIO::setSoftMute(switchOn);

  if (switchOn) {
    registers[POWERCFG] &= ~(1 << DSMUTE);  // clear mute bit
  } else {
    registers[POWERCFG] |= (1 << DSMUTE);  // set mute bit
  }                                        // if
  _saveRegisters();

}  // setSoftMute()


// ----- Band and frequency control methods -----

// tune to new band.
void SI4703::setBand(RADIO_BAND newBand) {

  if (newBand == RADIO_BAND_FM) {
    // init chip here for FM
    RADIO::setBand(newBand);
    _freqLow = 8750;

    _readRegisters();  // Read the current register set

    // PowerConfig
    registers[POWERCFG] = 0x4001;  // Enable the IC
    if (!_mute)
      registers[POWERCFG] |= (1 << DMUTE);  // disable Mute
    if (!_softMute)
      registers[POWERCFG] |= (1 << DSMUTE);  // disable softmute

    registers[SYSCONFIG1] |= (1 << RDS);  // Enable RDS


    if (_deEmphasis == RADIO_DEEMPHASIS_50) {
      registers[SYSCONFIG1] |= DEEMPHASIS50;  // 50µs
    } else {
      registers[SYSCONFIG1] &= ~DEEMPHASIS50;  // 75µs
    }

    if (_fmSpacing == RADIO_FMSPACING_50) {
      _freqSteps = 5;
      registers[SYSCONFIG2] &= ~(FMSPACE_MASK);  // Clear all
      registers[SYSCONFIG2] |= FMSPACE_50;

    } else if (_fmSpacing == RADIO_FMSPACING_100) {
      _freqSteps = 10;
      registers[SYSCONFIG2] &= ~(FMSPACE_MASK);  // Clear all
      registers[SYSCONFIG2] |= FMSPACE_100;      // set 100

    } else if (_fmSpacing == RADIO_FMSPACING_200) {
      _freqSteps = 20;
      registers[SYSCONFIG2] &= ~(FMSPACE_MASK);  // Clear all
      registers[SYSCONFIG2] |= FMSPACE_200;
    }

    _volume = 1;
    registers[SYSCONFIG2] &= ~(VOLUME_MASK);
    registers[SYSCONFIG2] |= (_volume & VOLUME_MASK);  // Set volume

    // set seek parameters
    registers[SYSCONFIG2] |= SEEKTH_MID;     // Set volume
    registers[SYSCONFIG3] &= ~(SKSNR_MASK);  // Clear seek mask bits
    registers[SYSCONFIG3] |= SKSNR_MID;      // Set volume

    registers[POWERCFG] |= (1 << RDSMODE);  // set force verbose bit

    _saveRegisters();  // Update
    delay(110);        // Max powerup time, from datasheet page 13
  }                    // if
}  // setBand()


/**
 * @brief Retrieve the real frequency from the chip after automatic tuning.
 * @return RADIO_FREQ the current frequency.
 */
RADIO_FREQ SI4703::getFrequency() {
  _readRegisters();
  int channel = registers[READCHAN] & 0x03FF;  // Mask out everything but the lower 10 bits
  _freq = (channel * _freqSteps) + _freqLow;
  return (_freq);
}  // getFrequency


/**
 * @brief Change the frequency in the chip.
 * @param newF
 * @return void
 */
void SI4703::setFrequency(RADIO_FREQ newF) {
  DEBUG_FUNC1("setFrequency", newF);
  if (newF < _freqLow)
    newF = _freqLow;
  if (newF > _freqHigh)
    newF = _freqHigh;

  _readRegisters();
  int channel = (newF - _freqLow) / _freqSteps;

  // These steps come from AN230 page 20 rev 0.5
  registers[CHANNEL] &= 0xFE00;       // Clear out the channel bits
  registers[CHANNEL] |= channel;      // Mask in the new channel
  registers[CHANNEL] |= (1 << TUNE);  // Set the TUNE bit to start
  _saveRegisters();
  if (_sendRDS) {
    _sendRDS(0, 0, 0, 0);
  }
  _waitEnd();
}  // setFrequency()


// start seek mode upwards
void SI4703::seekUp(bool toNextSender) {
  DEBUG_FUNC1("seekUp", toNextSender);
  _seek(true);
}  // seekUp()


// start seek mode downwards
void SI4703::seekDown(bool toNextSender) {
  DEBUG_FUNC1("seekDown", toNextSender);
  _seek(false);
}  // seekDown()


// Load all status registers from to the chip
void SI4703::_readRegisters() {
  // Si4703 begins reading from register upper register of 0x0A and reads to 0x0F, then loops to 0x00.
  _i2cPort->requestFrom(_i2caddr, 32);  // We want to read the entire register set from 0x0A to 0x09 = 32 bytes.

  // Remember, register 0x0A comes in first so we have to shuffle the array around a bit
  for (int x = 0x0A;; x++) {  // Read in these 32 bytes
    if (x == 0x10)
      x = 0;  // Loop back to zero
    registers[x] = _read16HL(_i2cPort);
    if (x == 0x09)
      break;  // We're done!
  }           // for
}  // _readRegisters()


// Load all status registers from to the chip
void SI4703::_readRegister0A() {
  _i2cPort->requestFrom(_i2caddr, 2);  // We want to read the entire register set from 0x0A to 0x09 = 32 bytes.
  registers[0x0A] = _read16HL(_i2cPort);
}  // _readRegister0A()


// Save writable registers back to the chip
// The registers 02 through 06, containing the configuration
// using the sequential write access mode.
void SI4703::_saveRegisters() {
  // Write the current 9 control registers (0x02 to 0x07) to the Si4703
  // It's a little weird, you don't write an I2C addres
  // The Si4703 assumes you are writing to 0x02 first, then increments

  _i2cPort->beginTransmission(_i2caddr);
  // A write command automatically begins with register 0x02 so no need to send a write-to address
  // First we send the 0x02 to 0x07 control registers
  // In general, we should not write to registers 0x08 and 0x09
  for (int regSpot = 0x02; regSpot < 0x08; regSpot++) {
    _write16HL(_i2cPort, registers[regSpot]);
  }

  // End this transmission
  byte ack = _i2cPort->endTransmission();
  if (ack != 0) {                 // We have a problem!
    Serial.print("Write Fail:");  // No ACK!
    Serial.println(ack, DEC);     // I2C error: 0 = success, 1 = data too long, 2 = rx NACK on address, 3 = rx NACK on data, 4 = other error
  }
}  // _saveRegisters


/// Retrieve all the information related to the current radio receiving situation.
void SI4703::getRadioInfo(RADIO_INFO *info) {
  RADIO::getRadioInfo(info);  // all settings to last current settings

  _readRegisters();
  info->active = true;  // ???
  if (registers[STATUSRSSI] & SI)
    info->stereo = true;
  info->rssi = registers[STATUSRSSI] & RSSI;
  if (registers[STATUSRSSI] & (RDSS))
    info->rds = true;
  if (registers[STATUSRSSI] & STC)
    info->tuned = true;
  if (registers[POWERCFG] & (1 << SETMONO))
    info->mono = true;
}  // getRadioInfo()


/// Return current audio settings.
void SI4703::getAudioInfo(AUDIO_INFO *info) {
  RADIO::getAudioInfo(info);

  _readRegisters();
  if (!(registers[POWERCFG] & (1 << DMUTE)))
    info->mute = true;
  if (!(registers[POWERCFG] & (1 << DSMUTE)))
    info->softmute = true;
  info->bassBoost = false;  // no bassBoost
  info->volume = registers[SYSCONFIG2] & VOLUME_MASK;
}  // getAudioInfo()


void SI4703::checkRDS() {
  // DEBUG_FUNC0("checkRDS");
  unsigned long now = millis();

  // check if there is a listener !
  if ((_sendRDS) && (now > _lastRDSPoll + 40)) {
    _readRegister0A();
    _lastRDSPoll = now;

    // int r1 = registers[STATUSRSSI];
    // _readRegisters();
    // int r2 = registers[STATUSRSSI];
    // Serial.printf("== 0x%04x 0x%04x\n", r1, r2);


    // check for a RDS data set ready
    if (registers[STATUSRSSI] & RDSR) {
      _readRegisters();
      _lastRDSPoll = now;
      uint8_t errA = (registers[STATUSRSSI] >> 9) & 3;
      uint8_t errB = (registers[READCHAN] >> 14) & 3;
      uint8_t errC = (registers[READCHAN] >> 12) & 3;
      uint8_t errD = (registers[READCHAN] >> 10) & 3;
      if ((errA != 3) && (errB != 3) && (errC != 3) && (errD != 3))
        _sendRDS(registers[RDSA], registers[RDSB], registers[RDSC], registers[RDSD]);
      /*
      Serial.print(" = 0x"); _printHex4( (registers[STATUSRSSI] >> 9)&3 ); Serial.print(' ');
      Serial.print(" = 0x"); _printHex4( (registers[READCHAN] >> 14)&3 ); Serial.print(' ');
      Serial.print(" = 0x"); _printHex4( (registers[READCHAN] >> 12)&3 ); Serial.print(' ');
      Serial.print(" = 0x"); _printHex4( (registers[READCHAN] >> 10)&3 ); Serial.println();
      */
    }  // if
  }    // if
}  // checkRDS


void SI4703::writeGPIO(int GPIO, int val) {
  _readRegisters();  // Read the current register set

  switch (GPIO) {
    case GPIO1:
      registers[SYSCONFIG1] &= ~(0b11 << GPIO1);
      registers[SYSCONFIG1] |= (val << GPIO1);
      break;
    case GPIO2:
      registers[SYSCONFIG1] &= ~(0b11 << GPIO2);
      registers[SYSCONFIG1] |= (val << GPIO2);
      break;
    case GPIO3:
      registers[SYSCONFIG1] &= ~(0b11 << GPIO3);
      registers[SYSCONFIG1] |= (val << GPIO3);
      break;
    default:
      break;
  }
  _saveRegisters();  // Write to registers
}

// ----- Debug functions -----

/// Send the current values of all registers to the Serial port.
void SI4703::debugStatus() {
  RADIO::debugStatus();

  _readRegisters();
  // Print all registers for debugging
  for (int x = 0; x < 16; x++) {
    Serial.print("Reg: 0x0");
    Serial.print(x, HEX);
    Serial.print(" = 0x");
    _printHex4(registers[x]);
    Serial.println();
  }  // for
}  // debugStatus


/// Seeks out the next available station
void SI4703::_seek(bool seekUp) {
  uint16_t reg;

  _readRegisters();
  // not wrapping around.
  reg = registers[POWERCFG] & ~((1 << SKMODE) | (1 << SEEKUP));

  if (seekUp)
    reg |= (1 << SEEKUP);  // Set the Seek-up bit

  reg |= (1 << SEEK);  // Start seek now

  // save the registers and start seeking...
  registers[POWERCFG] = reg;
  _saveRegisters();
  if (_sendRDS) {
    _sendRDS(0, 0, 0, 0);
  }
  _waitEnd();
}  // _seek


/// wait until the current seek and tune operation is over.
void SI4703::_waitEnd() {
  DEBUG_FUNC0("_waitEnd");

  // wait until STC gets high
  do {
    _readRegister0A();
    delay(10);
  } while ((registers[STATUSRSSI] & STC) == 0);

  // DEBUG_VAL("Freq:", getFrequency());

  _readRegisters();
  // get the SFBL bit.
  if (registers[STATUSRSSI] & SFBL)
    DEBUG_STR("Seek limit hit");

  // end the seek mode
  registers[POWERCFG] &= ~(1 << SEEK);
  registers[CHANNEL] &= ~(1 << TUNE);  // Clear the tune after a tune has completed
  _saveRegisters();

  // wait until STC gets down again
  do {
    _readRegisters();
  } while ((registers[STATUSRSSI] & STC) != 0);
}  // _waitEnd()


// ----- internal functions -----

// The End.
