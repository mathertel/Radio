///
/// \file RDSParser.cpp
/// \brief RDS Parser class implementation.
///
/// \author Matthias Hertel, http://www.mathertel.de
/// \copyright Copyright (c) 2014 by Matthias Hertel.\n
/// This work is licensed under a BSD style license.\n
/// See http://www.mathertel.de/License.aspx
///
/// \details
///
/// More documentation and source code is available at http://www.mathertel.de/Arduino
///
/// ChangeLog see RDSParser.h.

#include "RDSParser.h"

#define DEBUG_FUNC0(fn)          { Serial.print(fn); Serial.println("()"); }

/// Setup the RDS object and initialize private variables to 0.
RDSParser::RDSParser()
{
  memset(this, 0, sizeof(RDSParser));
} // RDSParser()


void RDSParser::init()
{
  strcpy(_PSName1, "--------");
  strcpy(_PSName2, _PSName1);
  strcpy(programServiceName, "        ");
  memset(_RDSText, 0, sizeof(_RDSText));
  _lastTextIDX = 0;
} // init()


void RDSParser::attachServicenNameCallback(receiveServicenNameFunction newFunction)
{
  _sendServiceName = newFunction;
} // attachServicenNameCallback

void RDSParser::attachTextCallback(receiveTextFunction newFunction)
{
  _sendText = newFunction;
} // attachTextCallback


void RDSParser::attachTimeCallback(receiveTimeFunction newFunction)
{
  _sendTime = newFunction;
} // attachTimeCallback


void RDSParser::processData(uint16_t block1, uint16_t block2, uint16_t block3, uint16_t block4)
{
  // DEBUG_FUNC0("process");
  uint8_t  idx; // index of rdsText
  char c1, c2;
  char *p;

  uint8_t hour; // RDS time in minutes
  uint8_t mins; // RDS time in minutes
  int8_t off;   // RDS time offset and sign
  uint32_t MJD; // Modified Julian Day

  // Serial.print('('); Serial.print(block1, HEX); Serial.print(' '); Serial.print(block2, HEX); Serial.print(' '); Serial.print(block3, HEX); Serial.print(' '); Serial.println(block4, HEX);

  if (block1 == 0)
  {
    // reset all the RDS info.
    init();
    // Send out empty data
    if (_sendServiceName)
      _sendServiceName(programServiceName);
    if (_sendText)
      _sendText("");
    return;
  }

  // analyzing Block 2
  rdsGroupType = 0x0A | ((block2 & 0xF000) >> 8) | ((block2 & 0x0800) >> 11);
  rdsTP = (block2 & 0x0400);
  rdsPTY = (block2 & 0x0400);

  switch (rdsGroupType)
  {
  case 0x0A:
  case 0x0B:
    // The data received is part of the Service Station Name
    idx = 2 * (block2 & 0x0003);

    // new data is 2 chars from block 4
    c1 = block4 >> 8;
    c2 = block4 & 0x00FF;

    // check that the data was received successfully twice
    // before publishing the station name

    if ((_PSName1[idx] == c1) && (_PSName1[idx + 1] == c2))
    {
      // retrieved the text a second time: store to _PSName2
      _PSName2[idx] = c1;
      _PSName2[idx + 1] = c2;
      _PSName2[8] = '\0';

      if ((idx == 6) && strcmp(_PSName1, _PSName2) == 0)
      {
        if (strcmp(_PSName2, programServiceName) != 0)
        {
          // publish station name
          strcpy(programServiceName, _PSName2);
          if (_sendServiceName)
            _sendServiceName(programServiceName);
        }
      }
    }

    if ((_PSName1[idx] != c1) || (_PSName1[idx + 1] != c2))
    {
      _PSName1[idx] = c1;
      _PSName1[idx + 1] = c2;
      _PSName1[8] = '\0';
      // Serial.println(_PSName1);
    }
    break;

  case 0x2A:
    // The data received is part of the RDS Text.
    _textAB = (block2 & 0x0010);
    idx = 4 * (block2 & 0x000F);

    if (idx < _lastTextIDX)
    {
      // the existing text might be complete because the index is starting at the beginning again.
      // send text if it's different than last sent.
      if ((_sendText) && (strncmp(_lastRDSText, _RDSText, sizeof(_RDSText)) != 0))
      {
        memcpy(_lastRDSText, _RDSText, sizeof(_RDSText));
        _sendText(_RDSText);
    }
    }
    _lastTextIDX = idx;

    if (_textAB != _last_textAB)
    {
      // when this bit is toggled the whole buffer should be cleared.
      _last_textAB = _textAB;
      memset(_RDSText, 0, sizeof(_RDSText));
      // Serial.println("T>CLEAR");
    }

    // new data is 2 chars from block 3
    _RDSText[idx] = (block3 >> 8);
    idx++;
    _RDSText[idx] = (block3 & 0x00FF);
    idx++;

    // new data is 2 chars from block 4
    _RDSText[idx] = (block4 >> 8);
    idx++;
    _RDSText[idx] = (block4 & 0x00FF);
    idx++;

    break;

  case 0x4A:
    // Clock time and date
    off = (30 * (((block4) & 0x3F) & 0x1F)) / 60;
    mins = (block4 >> 6) & 0x3F; // 6 bits
    hour = (((block3 & 0x0001) << 4) | ((block4 >> 12) & 0x0F));

    MJD = (block3 >> 1);                           //shift to the right since Bit0 is not relevant
    MJD = (MJD | 0x4000) | ((block2 & 0x1) << 14); //Bit 0 of block B becomes bit15 in MJD
    MJD = (MJD | 0x8000) | ((block2 & 0x2) << 15); //Bit 1 of block B becomes bit 16 in MJD

    // adjust offset
    if (((block4) & 0x3F) & 0x20)
    {
      off *= -1;
    }

    if ((_sendTime) && (mins != _lastRDSMinutes))
    {
      _lastRDSMinutes = mins;
      _sendTime(hour, mins, MJD, off);
    }
    break;

  case 0x6A:
    // IH
    break;

  case 0x8A:
    // TMC
    break;

  case 0xAA:
    // TMC
    break;

  case 0xCA:
    // TMC
    break;

  case 0xEA:
    // IH
    break;

  default:
    // Serial.print("RDS_GRP:"); Serial.println(rdsGroupType, HEX);
    break;
  }
} // processData()

// End.