///
/// \file RDSParser.h
/// \brief RDS Parser class definition.
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
/// History:
/// --------
/// * 01.09.2014 created and RDS sender name working.
/// * 01.11.2014 RDS time added.
/// * 27.03.2015 Reset RDS data by sending a 0 in blockA in the case the frequency changes.
///


#ifndef __RDSPARSER_H__
#define __RDSPARSER_H__

#include <Arduino.h>

/// callback function for passing a ServicenName 
extern "C" {
  typedef void(*receiveServicenNameFunction)(char *name);
  typedef void(*receiveTextFunction)(char *name);
  typedef void(*receiveTimeFunction)(uint8_t hour, uint8_t minute);
}


/// Library for parsing RDS data values and extracting information.
class RDSParser
{
public:
  RDSParser(); ///< create a new object from this class.

  /// Initialize internal variables before starting or after a change to another channel.
  void init();

  /// Pass all available RDS data through this function.
  void processData(uint16_t block1, uint16_t block2, uint16_t block3, uint16_t block4);

  void attachServicenNameCallback(receiveServicenNameFunction newFunction); ///< Register function for displaying a new Service Name.
  void attachTextCallback(receiveTextFunction newFunction); ///< Register the function for displaying a rds text.
  void attachTimeCallback(receiveTimeFunction newFunction); ///< Register function for displaying a new time

private:
  // ----- actual RDS values
  uint8_t rdsGroupType, rdsTP, rdsPTY;
  uint8_t _textAB, _last_textAB, _lastTextIDX;

  // Program Service Name
  char _PSName1[10]; // including trailing '\00' character.
  char _PSName2[10]; // including trailing '\00' character.
  char programServiceName[10];    // found station name or empty. Is max. 8 character long.

  receiveServicenNameFunction _sendServiceName; ///< Registered ServiceName function.
  receiveTimeFunction _sendTime; ///< Registered Time function.
  receiveTextFunction _sendText;

  uint16_t _lastRDSMinutes; ///< last RDS time send to callback.

  char _RDSText[64 + 2];

}; //RDSParser

#endif //__RDSPARSER_H__
