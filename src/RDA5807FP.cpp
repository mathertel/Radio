/// \file RDA5807FP.cpp
/// \brief Implementation for the radio library to control the RDA5807FP radio chip.
///
/// \author Marcus Degenkolbe, http://www.degenkolbe.eu
/// \copyright Copyright (c) 2023 by Marcus Degenkolbe.\n
/// This work is licensed under a BSD style license.\n
/// See http://www.mathertel.de/License.aspx
///
/// This library enables the use of the radio chip RDA5807FP from http://www.rdamicro.com/.
///
/// More documentation and source code is available at http://www.mathertel.de/Arduino
///
/// History:
/// --------
/// * 10.01.2023 created.

#include <Arduino.h>
#include <Wire.h>
#include <radio.h>

#include <RDA5807FP.h>

// ----- Register Definitions -----

// this chip only supports FM mode

#define RADIO_REG_R4 0x04
#define RADIO_REG_R4_I2S 0xF0

// regisrer R6 contains I2S options
#define RADIO_REG_R6 0x06
#define RADIO_REG_R6_I2S_WS_CNT 0xF0
#define RADIO_REG_R6_I2S_DATA_SIGNED 0x200
#define RADIO_REG_R6_I2S_MODE 0x1000

// ----- implement

// initialize the extra variables in RDA5807FP
RDA5807FP::RDA5807FP()
{

}

// Configures I2S output via GPIO1 (WS), GPIO2 (SD/DOUT), GPIO3 (SCK/BCLK)
void RDA5807FP::SetupI2S(RDA5807FP_I2SConfig config)
{
  // enable I2S in register 0x4
  registers[RADIO_REG_R4] = (registers[RADIO_REG_R4] & ~RADIO_REG_R4_I2S) | ((config.enabled << 6) & RADIO_REG_R4_I2S);
  _saveRegister(RADIO_REG_R4);

  // set I2S options in register 0x6
  registers[RADIO_REG_R6] = (registers[RADIO_REG_R6] & ~RADIO_REG_R6_I2S_DATA_SIGNED) | ((config.data_signed << 9) & RADIO_REG_R6_I2S_DATA_SIGNED);
  registers[RADIO_REG_R6] = (registers[RADIO_REG_R6] & ~RADIO_REG_R6_I2S_MODE) | ( (config.slave << 12) & RADIO_REG_R6_I2S_MODE);
  registers[RADIO_REG_R6] = (registers[RADIO_REG_R6] & ~RADIO_REG_R6_I2S_WS_CNT) | ((config.rate << 4) & RADIO_REG_R6_I2S_WS_CNT);
  _saveRegister(RADIO_REG_R6);
}

// ----- internal functions -----

// The End.
