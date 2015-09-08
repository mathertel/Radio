///
/// \file TEA5767.cpp
/// \brief Implementation for the radio library to control the TEA5767 radio chip.
///
/// \author Matthias Hertel, http://www.mathertel.de
/// \copyright Copyright (c) 2014-2015 by Matthias Hertel.\n
/// This work is licensed under a BSD style license.\n
/// See http://www.mathertel.de/License.aspx
///
/// This library enables the use of the Radio Chip TEA5767.
///
/// More documentation and source code is available at http://www.mathertel.de/Arduino
///
/// good links for hints how to implement this chip:
/// http://www.sparkfun.com/datasheets/Wireless/General/TEA5767.pdf
/// http://www.rockbox.org/wiki/pub/Main/DataSheets/application_note_tea5767-8.pdf
/// https://raw.githubusercontent.com/mroger/TEA5767/master/TEA5767N.cpp
/// http://www.electronicsblog.net
/// https://github.com/andykarpov/TEA5767/blob/master/TEA5767.cpp
///
/// ChangeLog see TEA5767.h:

#include <Arduino.h>
#include <Wire.h>     // The chip is controlled via the standard Arduiino Wire library and the IIC/I2C bus.

#include <radio.h>    // Include the common radio library interface
#include <TEA5767.h>

// ----- Definitions for the Wire communication

#define TEA5767_ADR 0x60 // I2C address of TEA5767

// ----- Radio chip specific definitions including the registers

#define QUARTZ 32768
#define FILTER 225000

// Define the registers

#define REG_1 0x00
#define REG_1_MUTE  0x80
#define REG_1_SM    0x40
#define REG_1_PLL   0x3F

#define REG_2 0x01
#define REG_2_PLL   0xFF

#define REG_3 0x02
#define REG_3_MS   0x08
#define REG_3_SSL  0x60
#define REG_3_SUD  0x80

#define REG_4 0x03
#define REG_4_SMUTE 0x08
#define REG_4_XTAL  0x10
#define REG_4_BL    0x20
#define REG_4_STBY  0x40

#define REG_5 0x04
#define REG_5_PLLREF  0x80
#define REG_5_DTC     0x40


#define STAT_3 0x02
#define STAT_3_STEREO 0x80

#define STAT_4 0x03
#define STAT_4_ADC 0xF0


// // Use this define to setup European FM specific settings in the chip.
#define IN_EUROPE

// ----- implement

// initialize the extra variables in SI4703
TEA5767::TEA5767() {
}

// initialize all internals.
bool TEA5767::init() {
  bool result = false; // no chip found yet.
  DEBUG_FUNC0("init");

  registers[0] = 0x00;
  registers[1] = 0x00;
  registers[2] = 0xB0;
  registers[REG_4] = REG_4_XTAL | REG_4_SMUTE;

#ifdef IN_EUROPE
  registers[REG_5] = 0; // 50 ms Europe setup
#else
  registers[REG_5] = REG_5_DTC; // 75 ms Europe setup
#endif
  Wire.begin();


  return(result);
} // init()


// switch the power off
void TEA5767::term()
{
  DEBUG_FUNC0("term");
} // term


// ----- Volume control -----

/// setVolume is a non-existing function in TEA5767. It will always me MAXVOLUME.
void TEA5767::setVolume(uint8_t newVolume)
{
  DEBUG_FUNC0("setVolume");
  RADIO::setVolume(MAXVOLUME);
} // setVolume()


/// setBassBoost is a non-existing function in TEA5767. It will never be acivated.
void TEA5767::setBassBoost(bool switchOn)
{
  DEBUG_FUNC0("setBassBoost");
  RADIO::setBassBoost(false);
} // setBassBoost()


/// force mono receiving mode.
void TEA5767::setMono(bool switchOn)
{
  DEBUG_FUNC0("setMono");
  RADIO::setMono(switchOn);

  if (switchOn) {
    registers[REG_3] |= REG_3_MS;
  } else {
    registers[REG_3] &= ~REG_3_MS;
  } // if
  _saveRegisters();
} // setMono


/// Force mute mode.
void TEA5767::setMute(bool switchOn)
{
  DEBUG_FUNC0("setMute");
  RADIO::setMute(switchOn);

  if (switchOn) {
    registers[REG_1] |= REG_1_MUTE;
  } else {
    registers[REG_1] &= ~REG_1_MUTE;
  } // if
  _saveRegisters();
} // setMute()


// ----- Band and frequency control methods -----

/// Tune to new a band.
void TEA5767::setBand(RADIO_BAND newBand) {
  if (newBand == RADIO_BAND_FM) {

    RADIO::setBand(newBand);

#ifdef IN_EUROPE
    //Freq(MHz) = 0.100(in Europe) * Channel + 87.5MHz
    registers[REG_4] &= ~REG_4_BL;

#else
    //Freq(MHz) = 0.200(in USA) * Channel + 87.5MHz
    registers[REG_4] |= REG_4_BL;
#endif

  } // if


  // FM mixer for conversion to IF of the US/Europe (87.5 MHz to 108 MHz) and Japanese
  // (76 MHz to 91 MHz) FM ba

} // setBand()




/**
* @brief Retrieve the real frequency from the chip after automatic tuning.
* @return RADIO_FREQ the current frequency.
*/
RADIO_FREQ TEA5767::getFrequency() {
  _readRegisters();

  unsigned long frequencyW = ((status[REG_1] & REG_1_PLL) << 8) | status[REG_2];
  frequencyW = ((frequencyW * QUARTZ / 4) - FILTER) / 10000;

  return ((RADIO_FREQ)frequencyW);
}  // getFrequency


/**
* @brief Change the frequency in the chip.
* @param newF
* @return void
*/
void TEA5767::setFrequency(RADIO_FREQ newF) {
  DEBUG_FUNC1("setFrequency", newF);
  _freq = newF;

  unsigned int frequencyB = 4 * (newF * 10000L + FILTER) / QUARTZ;
  Serial.print('*'); Serial.println(frequencyB);

  registers[0] = frequencyB >> 8;
  registers[1] = frequencyB & 0XFF;
  _saveRegisters();
  delay(100);
} // setFrequency()


/// Start seek mode upwards.
void TEA5767::seekUp(bool toNextSender) {
  DEBUG_FUNC0("seekUp");
  _seek(true);
} // seekUp()


/// Start seek mode downwards.
void TEA5767::seekDown(bool toNextSender) {
  _seek(false);
} // seekDown()



/// Load all status registers from to the chip
void TEA5767::_readRegisters()
{
  Wire.requestFrom(TEA5767_ADR, 5); // We want to read all the 5 registers.

  if (Wire.available()) {
    for (uint8_t n = 0; n < 5; n++) {
      status[n] = Wire.read();
    } // for
  } // if
} // _readRegisters


// Save writable registers back to the chip
// using the sequential write access mode.
void TEA5767::_saveRegisters()
{
  Wire.beginTransmission(TEA5767_ADR);
  for (uint8_t n = 0; n < sizeof(registers); n++) {
    Wire.write(registers[n]);
  } // for

  byte ack = Wire.endTransmission();
  if (ack != 0) { //We have a problem!
    Serial.print("Write Fail:"); //No ACK!
    Serial.println(ack, DEC); //I2C error: 0 = success, 1 = data too long, 2 = rx NACK on address, 3 = rx NACK on data, 4 = other error
  } // if

} // _saveRegisters


void TEA5767::getRadioInfo(RADIO_INFO *info) {
  RADIO::getRadioInfo(info);

  _readRegisters();
  if (status[STAT_3] & STAT_3_STEREO) info->stereo = true;
  info->rssi = (status[STAT_4] & STAT_4_ADC) >> 4;

} // getRadioInfo()


void TEA5767::getAudioInfo(AUDIO_INFO *info) {
  RADIO::getAudioInfo(info);
} // getAudioInfo()


void TEA5767::checkRDS()
{
  // DEBUG_FUNC0("checkRDS");
} // checkRDS

// ----- Debug functions -----

/// Send the current values of all registers to the Serial port.
void TEA5767::debugStatus()
{
  RADIO::debugStatus();
} // debugStatus


/// Seeks out the next available station
void TEA5767::_seek(bool seekUp) {
  DEBUG_FUNC0("_seek");
} // _seek


/// wait until the current seek and tune operation is over.
void TEA5767::_waitEnd() {
  DEBUG_FUNC0("_waitEnd");

} // _waitEnd()



// ----- internal functions -----

// The End.


