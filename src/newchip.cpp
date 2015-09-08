/// \file newchip.cpp
/// \brief Implementation for the radio library to control the newchip radio chip.
///
/// \author Matthias Hertel, http://www.mathertel.de
/// \copyright Copyright (c) 2014-2015 by Matthias Hertel.\n
/// This work is licensed under a BSD style license.\n
/// See http://www.mathertel.de/License.aspx
///
/// This library enables the use of the Radio Chip newchip.
///
/// More documentation and source code is available at http://www.mathertel.de/Arduino
///
/// History:
/// --------
/// * 05.08.2014 created.

#include <Arduino.h>
#include <Wire.h>     // The chip is controlled via the standard Arduiino Wire library and the IIC/I2C bus.

#include <radio.h>    // Include the common radio library interface
#include <newchip.h>

// ----- Definitions for the Wire communication

#define newchip_ADR 0x10 // I2C address of newchip

// ----- Radio chip specific definitions including the registers

// // Use this define to setup European FM specific settings in the chip.
// #define IN_EUROPE
// 
// // int STATUS_LED = 13;
// #define resetPin 2
// // SDA/A4 on Arduino
// #define SDIO A4
// // int SCLK = A5; //SCL/A5 on Arduino
// 
// // Register Definitions -----
// 
// //Define the register names
// #define DEVICEID 0x00
// #define CHIPID  0x01
// #define POWERCFG  0x02
// #define CHANNEL  0x03
// #define SYSCONFIG1  0x04
// #define SYSCONFIG2  0x05
// #define STATUSRSSI  0x0A
// #define READCHAN  0x0B
// #define RDSA  0x0C
// #define RDSB  0x0D
// #define RDSC  0x0E
// #define RDSD  0x0F
// 
// //Register 0x02 - POWERCFG
// #define SMUTE  15
// #define DMUTE  14
// #define SKMODE  10
// #define SEEKUP  9
// #define SEEK  8
// 
// //Register 0x03 - CHANNEL
// #define TUNE  15
// 
// //Register 0x04 - SYSCONFIG1
// #define RDS  12
// #define DE  11
// 
// #define DE  11
// 
// 
// //Register 0x05 - SYSCONFIG2
// #define SPACE1  5
// #define SPACE0  4
// 
// //Register 0x0A - STATUSRSSI
// #define RDSR  15
// #define STC  14
// #define SFBL  13
// #define AFCRL  12
// #define RDSS  11
// #define STEREO  8

// ----- implement

// initialize the extra variables in SI4703
newchip::newchip() {
}

// initialize all internals.
bool newchip::init() {
  bool result = false; // no chip found yet.
  DEBUG_FUNC0("init");

  return(result);
} // init()


// switch the power off
void newchip::term()
{
  DEBUG_FUNC0("term");
} // term


// ----- Volume control -----

void newchip::setVolume(uint8_t newVolume)
{
  DEBUG_FUNC0("setVolume");
} // setVolume()


void newchip::setBassBoost(bool switchOn)
{
  DEBUG_FUNC0("setBassBoost");
} // setBassBoost()


// Mono / Stereo
void newchip::setMono(bool switchOn)
{
  DEBUG_FUNC0("setMono");
} // setMono


// Switch mute mode.
void newchip::setMute(bool switchOn)
{
  DEBUG_FUNC0("setMute");
} // setMute()


// ----- Band and frequency control methods -----

// tune to new band.
void newchip::setBand(RADIO_BAND newBand) {
} // setBand()


/**
 * @brief Retrieve the real frequency from the chip after automatic tuning.
 * @return RADIO_FREQ the current frequency.
 */
RADIO_FREQ newchip::getFrequency() {
  return (_freq);
}  // getFrequency


/**
 * @brief Change the frequency in the chip.
 * @param newF
 * @return void
 */
void newchip::setFrequency(RADIO_FREQ newF) {
  DEBUG_FUNC1("setFrequency", newF);
} // setFrequency()


// start seek mode upwards
void newchip::seekUp(bool toNextSender) {
  DEBUG_FUNC0("seekUp");
  _seek(true);
} // seekUp()


// start seek mode downwards
void newchip::seekDown(bool toNextSender) {
  _seek(false);
} // seekDown()



// Load all status registers from to the chip
void newchip::_readRegisters()
{
}


// Save writable registers back to the chip
// using the sequential write access mode.
void newchip::_saveRegisters()
{
} // _saveRegisters


// write a register value using 2 bytes into the Wire.
void newchip::_write16(uint16_t val)
{
  Wire.write(val >> 8); Wire.write(val & 0xFF);
} // _write16


// read a register value using 2 bytes in a row
uint16_t newchip::_read16(void)
{
  uint8_t hiByte = Wire.read();
  uint8_t loByte = Wire.read();
  return((hiByte << 8) + loByte);
} // _read16


void newchip::getRadioInfo(RADIO_INFO *info) {
  RADIO::getRadioInfo(info);
} // getRadioInfo()


void newchip::getAudioInfo(AUDIO_INFO *info) {
  RADIO::getAudioInfo(info);
} // getAudioInfo()


void newchip::checkRDS()
{
  // DEBUG_FUNC0("checkRDS");
} // checkRDS

// ----- Debug functions -----

/// Send the current values of all registers to the Serial port.
void newchip::debugStatus()
{
  RADIO::debugStatus();
} // debugStatus


/// Seeks out the next available station
void newchip::_seek(bool seekUp) {
  DEBUG_FUNC0("_seek");
} // _seek


/// wait until the current seek and tune operation is over.
void newchip::_waitEnd() {
  DEBUG_FUNC0("_waitEnd");

} // _waitEnd()



// ----- internal functions -----

// The End.


