/* \file Radio.h
 * \brief Library header file for the radio libraries to control radio chips.
 *
 * \author Matthias Hertel, http://www.mathertel.de
 * \copyright Copyright (c) 2014 by Matthias Hertel.\n
 * This work is licensed under a BSD style license.\n
 * See http://www.mathertel.de/License.aspx
 *
 * The **RDA5807M** and **RDA5807FP** with I2S support from RDA Microelectronics
 * The **SI4703** from Silicon Labs, now Skyworks
 * The **SI4705** from Silicon Labs, now Skyworks
 * The **SI4721** and **SI4730** chips from Silicon Labs, now Skyworks
 * The **TEA5767** from NXP
 *
 * The following chip is planned to be supported too:
 * ...
 *
 * More documentation and source code is available at http://www.mathertel.de/Arduino
 *
 * ChangeLog:
 * ----------
 * * 08.07.2014 creation of the common radio class.
 * * 15.07.2014 examples working with RDA5807M.
 * * 26.08.2014 examples working with SI4703.
 * * 31.08.2014 Doxygen style comments added.
 * * 05.02.2015 mainpage content added.
 * * 29.04.2015 clear RDS function, need to clear RDS info after tuning.
 * * 17.09.2020 Wire Util functions added.
 * * 06.12.2020 I2C Wire and Reset initialization centralized.
 *
 * TODO:
 */

/// TODO:
/// --------
/// * multi-Band enabled

/// \mainpage
/// An Arduino library to control radio for receiving FM broadcast signals.
///
/// Currently the following chips are supported:
/// * The SI4703 from Silicon Labs
/// * The SI4705 from Silicon Labs
/// * The SI4721 from Silicon Labs
/// * The TEA5767 from NXP
/// * The RDA5807M from RDA Microelectronics
///
/// They all are capable for receiving FM radio stations in stereo with European and US settings and can be controlled by using the I2C bus.However there are differences in the sensitivity and quality and well on receiving RDS information from the stations.
///
/// For each of these chips a specific library is implemented that knows how to communicate with the chip using the I2C bus and the wire library.These libraries all share a common base, the radio library so that all the common code is only implemented once in there :
///
/// All the libraries share the same interface (defined by the radio library) so it is possible to exchange them when not using one of the chip specific functions.
///


#ifndef __RADIO_h__
#define __RADIO_h__

#include <Arduino.h>
#include <Wire.h>

#define UNUSED __attribute__((unused))

// The DEBUG_xxx Macros enable Information to the Serial port.
// They can be enabled by setting the _debugEnabled variable to true disabled by using the debugEnable function.
// When the code has to be minimized they can be redefined without implementation like:
// #define DEBUG_STR(txt) {}

/// Used for Debugging text information.
#define DEBUG_STR(txt) \
  if (_debugEnabled) { \
    Serial.print('>'); \
    Serial.println(txt); \
  }

/// Used for Debugging function entries without parameters.
#define DEBUG_VAL(label, val) \
  if (_debugEnabled) { \
    Serial.print('>'); \
    Serial.print(label); \
    Serial.print(':'); \
    Serial.println(val); \
  }
#define DEBUG_VALX(label, val) \
  if (_debugEnabled) { \
    Serial.print('>'); \
    Serial.print(label); \
    Serial.print(':'); \
    Serial.println(val, HEX); \
  }

/// Used for Debugging function entries without parameters.
#define DEBUG_FUNC0(fn) \
  if (_debugEnabled) { \
    Serial.print('>'); \
    Serial.print(fn); \
    Serial.println("()"); \
  }

/// Used for Debugging function entries with 1 parameter.
#define DEBUG_FUNC1(fn, p1) \
  if (_debugEnabled) { \
    Serial.print('>'); \
    Serial.print(fn); \
    Serial.print('('); \
    Serial.print(p1); \
    Serial.println(')'); \
  }

/// Used for Debugging function entries with 1 parameters as hex Value.
#define DEBUG_FUNC1X(fn, p1) \
  if (_debugEnabled) { \
    Serial.print('>'); \
    Serial.print(fn); \
    Serial.print("(0x"); \
    Serial.print(p1, HEX); \
    Serial.println(')'); \
  }

/// Used for Debugging function entries with 2 parameters.
#define DEBUG_FUNC2(fn, p1, p2) \
  if (_debugEnabled) { \
    Serial.print('>'); \
    Serial.print(fn); \
    Serial.print('('); \
    Serial.print(p1); \
    Serial.print(", "); \
    Serial.print(p2); \
    Serial.println(')'); \
  }

/// Used for Debugging function entries with 2 parameters and Hex Value.
#define DEBUG_FUNC2X(fn, p1, p2) \
  if (_debugEnabled) { \
    Serial.print('>'); \
    Serial.print(fn); \
    Serial.print("(0x"); \
    Serial.print(p1, HEX); \
    Serial.print(", 0x"); \
    Serial.print(p2, HEX); \
    Serial.println(')'); \
  }


// ----- Callback function types -----

/// callback function for passing RDS data.
extern "C" {
  typedef void (*receiveRDSFunction)(uint16_t block1, uint16_t block2, uint16_t block3, uint16_t block4);
}


// ----- type definitions -----

/// Band datatype.
/// The BANDs a receiver probably can implement.
enum RADIO_BAND {
  RADIO_BAND_NONE = 0,  ///< No band selected.

  RADIO_BAND_FM = 0x01,       ///< FM band 87.5 - 108 MHz (USA, Europe) selected.
  RADIO_BAND_FMWORLD = 0x02,  ///< FM band 76 - 108 MHz (Japan, Worldwide) selected.
  RADIO_BAND_AM = 0x03,       ///< AM band selected.
  RADIO_BAND_KW = 0x04,       ///< KW band selected.

  RADIO_BAND_FMTX = 0x11,  ///< Transmit for FM.
};


/// Frequency data type.
/// Only 16 bits are used for any frequency value (not the real one)
typedef uint16_t RADIO_FREQ;


/// A structure that contains information about the radio features from the chip.
struct RADIO_INFO {
  bool active;   ///< receiving is active.
  uint8_t rssi;  ///< Radio Station Strength Information.
  uint8_t snr;   ///< Signal Noise Ratio.
  bool rds;      ///< RDS information is available.
  bool tuned;    ///< A stable frequency is tuned.
  bool mono;     ///< Mono mode is on.
  bool stereo;   ///< Stereo audio is available
};


/// a structure that contains information about the audio features
struct AUDIO_INFO {
  uint8_t volume;
  bool mute;
  bool softmute;
  bool bassBoost;
};

// ----- common RADIO class definition -----

// setup() features and defined values

#define RADIO_RESETPIN 0x01
#define RADIO_SDAPIN 0x02
#define RADIO_MODEPIN 0x02
#define RADIO_I2CADDRESS 0x03

#define RADIO_ANTENNA 0x04
#define RADIO_ANTENNA_DEFAULT 0
#define RADIO_ANTENNA_OPT1 1
#define RADIO_ANTENNA_OPT2 2

// FM channel spacing configuration is supported by some radio chips.
#define RADIO_FMSPACING 0x05
#define RADIO_FMSPACING_25 25
#define RADIO_FMSPACING_50 50
#define RADIO_FMSPACING_100 100  // 100 kHz typically used in Europe / Japan = default
#define RADIO_FMSPACING_200 200  // 200 kHz typically used in US / Australia 

// FM High Frequency de-emphasis 
#define RADIO_DEEMPHASIS 0x06
#define RADIO_DEEMPHASIS_50 50 // 50µs typically used in Europe, Australia, Japan
#define RADIO_DEEMPHASIS_75 75 // 75µs typically used in USA

/// Library to control radio chips in general. This library acts as a base library for the chip specific implementations.
class RADIO {

public:
  const uint8_t MAXVOLUME = 15;  ///< max volume level for all radio library consumers.

  RADIO();  ///< create a new object from this class.

  virtual void setup(int feature, int value);  ///< configure board/hardware specific features before init().
  virtual bool init();                         ///< initialize library and the chip.
  virtual bool initWire(TwoWire &port);        // init with I2C bus
  virtual void term();                         ///< terminate all radio functions.

  // ----- Audio features -----

  virtual void setVolume(int8_t newVolume);  ///< Set the volume output of the radio chip.
  virtual int8_t getVolume();                ///< Retrieve the current output volume.
  virtual int8_t getMaxVolume();             ///< Retrieve the maximum possible output volume.

  virtual void setMute(bool switchOn);  ///< Control the mute mode of the radio chip.
  virtual bool getMute();               ///< Retrieve the current mute mode setting.

  virtual void setSoftMute(bool switchOn);  ///< Control the softmute mode (mute on low signals) of the radio chip.
  virtual bool getSoftMute();               ///< Retrieve the current soft mute mode setting.

  virtual void setBassBoost(bool switchOn);  ///< Control the bass boost mode of the radio chip.
  virtual bool getBassBoost();               ///< Retrieve the current bass boost mode setting.

  // ----- Receiver features -----

  virtual RADIO_FREQ getMinFrequency();   ///< Get the minimum frequency of the current selected band.
  virtual RADIO_FREQ getMaxFrequency();   ///< Get the maximum frequency of the current selected band.
  virtual RADIO_FREQ getFrequencyStep();  ///< Get resolution of the current selected band.

  virtual void setBand(RADIO_BAND newBand);  ///< Set the current band.
  virtual RADIO_BAND getBand();              ///< Retrieve the current band setting.

  virtual void setFrequency(RADIO_FREQ newF);  ///< Start using the new frequency for receiving.
  virtual RADIO_FREQ getFrequency(void);       ///< Retrieve the current tuned frequency.

  virtual void setBandFrequency(RADIO_BAND newBand, RADIO_FREQ newFreq);  ///< Set Band and Frequency in one call.

  virtual void seekUp(bool toNextSender = true);    ///< Start a seek upwards from the current frequency.
  virtual void seekDown(bool toNextSender = true);  ///< Start a seek downwards from the current frequency.

  virtual void setMono(bool switchOn);  ///< Control the mono mode of the radio chip.
  virtual bool getMono();               ///< Retrieve the current mono mode setting.

  // ----- combined status functions -----

  virtual void getRadioInfo(RADIO_INFO *info);  ///< Retrieve some information about the current radio function of the chip.

  virtual void getAudioInfo(AUDIO_INFO *info);  ///< Retrieve some information about the current audio function of the chip.

  // ----- Supporting RDS for FM bands -----

  virtual void attachReceiveRDS(receiveRDSFunction newFunction);  ///< Register a RDS processor function.
  virtual void checkRDS();                                        ///< Check if RDS Data is available and good.
  virtual void clearRDS();                                        ///< Clear RDS data in the attached RDS Receiver by sending 0,0,0,0.

  // ----- Utilities -----

  /// Format the current frequency for display and printing.
  virtual void formatFrequency(char *s, uint8_t length);

  // ----- debug Helpers send information to Serial port

  /**
   * Enable debugging information on Serial port.
   * This is for logging on a higher level than i2c data transport.
   * @param enable true to switch logging on.
   */
  virtual void debugEnable(bool enable = true);

  virtual void debugRadioInfo();  ///< Print out all radio information.
  virtual void debugAudioInfo();  ///< Print out all audio information.
  virtual void debugStatus();     ///< Send debug information about actual available chip functionality and other internal things.

  // ===== Wire Utilities (static) =====

  static bool _wireDebugFlag;
  static void _wireWriteTo(TwoWire *port, int address, uint8_t *cmdData, int cmdLen);
  static uint8_t _wireReadFrom(TwoWire *port, int address, uint8_t *data, int len);

  // write a 16 bit value in High-Low order to the Wire.
  static void _write16HL(TwoWire *port, uint16_t val);
  static uint16_t _read16HL(TwoWire *port);

  /**
   * Enable low level i2c debugging information on Serial port.
   * @param enable true to switch logging on.
   */
  virtual void _wireDebug(bool enable = true);

  /** check for a device on address.
   * @return true when i2c device answered.
   */
  bool _wireExists(TwoWire *port, int address);

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
  int _wireRead(TwoWire *port, int address, uint8_t reg, uint8_t *data, int len);

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
  int _wireRead(TwoWire *port, int address, uint8_t *cmdData, int cmdLen, uint8_t *data, int len);

protected:
  bool _debugEnabled = false;      ///< Set by debugEnable() and controls debugging functionality.
  bool _wireDebugEnabled = false;  ///< Set by _wireDebug() and controls i2c data level debugging.

  uint8_t _volume = 0;      ///< Last set volume level.
  uint8_t _maxVolume = 15;  ///< maximum of volume supported by the chip.

  bool _bassBoost = false;  ///< Last set bass Boost effect.
  bool _mono = false;       ///< Last set mono effect.
  bool _mute = false;       ///< Last set mute effect.
  bool _softMute = false;   ///< Last set softMute effect.

  RADIO_BAND _band;  ///< Last set band.
  RADIO_FREQ _freq;  ///< Last set frequency.

  RADIO_FREQ _freqLow;    ///< Lowest frequency of the current selected band.
  RADIO_FREQ _freqHigh;   ///< Highest frequency of the current selected band.
  RADIO_FREQ _freqSteps;  ///< Resolution of the tuner.

  receiveRDSFunction _sendRDS;  ///< Registered RDS Function that is called on new available data.

  void _printHex2(uint8_t val);   ///< Prints a byte as 2 character hexadecimal code with leading zeros.
  void _printHex4(uint16_t val);  ///< Prints a register as 4 character hexadecimal code with leading zeros.

  // i2c bus communication
  TwoWire *_i2cPort;
  int _i2caddr;

  // extra pins
  int _resetPin = -1;

  // Antenna Features
  int _antennaOption = 0;

  // FM Channel Spacing Features
  int _fmSpacing = RADIO_FMSPACING_100;

  // FM de-emphasis 
  int _deEmphasis = RADIO_DEEMPHASIS_50;

private:
  void int16_to_s(char *s, uint16_t val);  ///< Converts a int16 number to a string, similar to itoa, but using the format "00000".

};  // class RADIO


#endif

// End.