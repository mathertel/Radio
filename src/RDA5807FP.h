///
/// \file RDA5807FP.h
/// \brief Library header file for the radio library to control the RDA5807FP radio chip.
///
/// \author Marcus Degenkolbe, http://www.degenkolbe.eu
/// \copyright Copyright (c) 2023 by Marcus Degenkolbe.\n
/// This work is licensed under a BSD style license.\n
/// See http://www.mathertel.de/License.aspx
///
/// \details
/// This library enables the use of the Radio Chip RDA5807FP from http://www.rdamicro.com/ that supports FM radio bands, RDS data and I2S output.
///
/// More documentation and source code is available at http://www.mathertel.de/Arduino
///
/// History:
/// --------
/// * 10.01.2023 created.

// inherits all features from RDA5807M and adds basic I2S support

#ifndef RDA5807FP_h
#define RDA5807FP_h

#include <Arduino.h>
#include <Wire.h>
#include <radio.h>
#include <RDA5807M.h>

enum RDA5807FP_I2S_WS_CNT
{
  WS_STEP_48 = 0b1000,
  WS_STEP_44_1 = 0b0111,
  WS_STEP_32 = 0b0110,
  WS_STEP_24 = 0b0101,
  WS_STEP_22_05 = 0b0100,
  WS_STEP_16 = 0b0011,
  WS_STEP_12 = 0b0010,
  WS_STEP_11_025 = 0b0001,
  WS_STEP_8 = 0,
};

struct RDA5807FP_I2SConfig
{
  bool enabled = false;
  bool slave = false;
  RDA5807FP_I2S_WS_CNT rate = WS_STEP_48;
  bool data_signed = false;
};

// ----- library definition -----

/// Library to control the RDA5807FP radio chip.
class RDA5807FP : public RDA5807M
{
public:
  RDA5807FP();

  void SetupI2S(RDA5807FP_I2SConfig config);
};

#endif
