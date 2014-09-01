/*
* RDSParser.cpp
*
* Created: 01.09.2014 19:24:43
* Author: Matthias
*/


#include "RDSParser.h"

/// Setup the RDS object and initialize private variables to 0.
RDSParser::RDSParser() {
  memset(this, 0, sizeof(RDSParser));
} // RDSParser()


void RDSParser::init() {
  strcpy(_PSName1, "--------");
  strcpy(_PSName2, "--------");
  strcpy(programServiceName, "          ");
}



void RDSParser::processData( uint16_t block1, uint16_t block2, uint16_t block3, uint16_t block4 )
{
  // FUNC_DEBUG0("process");

  //int  i_rdsnew=0;
  uint8_t  idx; // index of rdsText
  char c1, c2;
  uint8_t off, min, hr;
  
  //int i_x,i_hh,i_mm,i_ofs;
  //long l_mjd;
  //String s_rds="0123456789.123456789.123456789.123456789.123456789.123456789.123";
  //
  
  // Serial.print('('); Serial.print(block1, HEX); Serial.print(' '); Serial.print(block2, HEX); Serial.print(' '); Serial.print(block3, HEX); Serial.print(' '); Serial.println(block4, HEX);

  // analyzing Block 2
  rdsGroupType = 0x0A | ((block2 & 0xF000) >> 8) | ((block2 & 0x0800) >> 11);
  rdsTP = (block2 & 0x0400);
  rdsPTY = (block2 & 0x0400);

  switch (rdsGroupType) {
    case 0x0A:
    case 0x0B:
    idx = 2 * (block2 & 0x0003);

    // new data is 2 chars from block 4
    c1 = block4 >> 8;
    c2 = block4 & 0x00FF;

    // check that the data was received successfully twice
    // before publishing the station name
    
    if ((_PSName1[idx] == c1) && (_PSName1[idx+1] == c2)) {
      // retrieved the text a second time: store to _PSName2
      _PSName2[idx] = c1;
      _PSName2[idx+1] = c2;
      _PSName2[8] = '\0';

      if ((idx == 6) && strcmp(_PSName1, _PSName2) == 0) {
        if (strcmp(_PSName2, programServiceName) != 0) {
          // publish station name
          strcpy (programServiceName, _PSName2);
          if (_sendServiceName)
            _sendServiceName(programServiceName);
        } // if
      } // if
    } // if
    
    if ((_PSName1[idx] != c1) || (_PSName1[idx+1] != c2)) {
      _PSName1[idx] = c1;
      _PSName1[idx+1] = c2;
      _PSName1[8] = '\0';
      // Serial.println(_PSName1);
    } // if
    break;

    case 0x4A:
    // Clock time and date
    //       off = (block4      ) & 0x3F; // 6 bits
    //       min = (block4 >>  6) & 0x3F; // 6 bits
    //       hr  = ((block3 & 0x0001) << 4) | ((block4 >> 12) & 0x0F); // 4 bits
    //
    //       Serial.print(block4, HEX); Serial.print(' '); Serial.print(hr); Serial.print(':'); Serial.print(min); Serial.print('/'); Serial.println(off);
    break;
    
    default:
    // Serial.print("RDS_GRP:"); Serial.println(rdsGroupType, HEX);
    break;
  }
}

void RDSParser::attachNewServicenName( receiveServicenNameFunction newFunction )
{
  _sendServiceName = newFunction;
}
