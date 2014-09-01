/* 
* RDSParser.h
*
* Created: 01.09.2014 19:24:43
* Author: Matthias
*/


#ifndef __RDSPARSER_H__
#define __RDSPARSER_H__

#include <arduino.h>

/// callback function for passing a ServicenName 
extern "C" {
  typedef void (*receiveServicenNameFunction)(char *name);
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
  
  void attachNewServicenName(receiveServicenNameFunction newFunction); ///< Register function for displaying a new Service Name.

private:
  // ----- actual RDS values
  uint8_t rdsGroupType, rdsTP, rdsPTY;

  // Program Service Name
  char _PSName1[10]; // including trailing '\00' character.
  char _PSName2[10]; // including trailing '\00' character.
  char programServiceName[10];    // found station name or empty. Is max. 8 character long.

  receiveServicenNameFunction _sendServiceName; ///< Registered Service name function.
}; //RDSParser

#endif //__RDSPARSER_H__
