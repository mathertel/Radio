///
/// \file ScanRadio.ino 
/// \brief This sketch implements a scanner that lists all availabe radio stations including some information.
/// 
/// \author Matthias Hertel, http://www.mathertel.de
/// \copyright Copyright (c) 2015 by Matthias Hertel.\n
/// This work is licensed under a BSD style license.\n
/// See http://www.mathertel.de/License.aspx
///
/// \details
/// This is a Arduino sketch that uses a state machine to scan through all radio stations the radio chip can detect
/// and outputs them on the Serial interface.\n
/// Open the Serial console with 57600 baud to see current radio information and change various settings.
///
/// Wiring
/// ------
/// The necessary wiring of the various chips are described in the Testxxx example sketches.
/// No additional components are required because all is done through the serial interface.
///
/// More documentation and source code is available at http://www.mathertel.de/Arduino
///
/// History:
/// --------
/// * 17.05.2015 created.
/// * 27.05.2015 first version is working (beta with SI4705).
/// * 04.07.2015 2 scan algorithms working with good results with SI4705.

#include <Wire.h>

#include <radio.h>
#include <RDA5807M.h>
#include <SI4703.h>
#include <SI4705.h>
#include <TEA5767.h>

#include <RDSParser.h>

/// The radio object has to be defined by using the class corresponding to the used chip.
/// by uncommenting the right radio object definition.

// RADIO radio;       ///< Create an instance of a non functional radio.
// RDA5807M radio;    ///< Create an instance of a RDA5807 chip radio
// SI4703   radio;    ///< Create an instance of a SI4703 chip radio.
SI4705   radio;    ///< Create an instance of a SI4705 chip radio.
// TEA5767  radio;    ///< Create an instance of a TEA5767 chip radio.

/// get a RDS parser
RDSParser rds;

/// State definition for this radio implementation.
enum SCAN_STATE {
  STATE_START,      ///< waiting for a new command character.
  STATE_NEWFREQ,
  STATE_WAITFIXED,
  STATE_WAITRDS,
  STATE_PRINT,
  STATE_END          ///< executing the command.
};

SCAN_STATE state; ///< The state variable is used for parsing input characters.

uint16_t g_block1;

// - - - - - - - - - - - - - - - - - - - - - - - - - -


// use a function inbetween the radio chip and the RDS parser
// to catch the block1 value (used for sender identification)
void RDS_process(uint16_t block1, uint16_t block2, uint16_t block3, uint16_t block4) {
  g_block1 = block1;
  rds.processData(block1, block2, block3, block4);
}

/// Update the ServiceName text on the LCD display.
void DisplayServiceName(char *name)
{
  bool found = false;

  for (uint8_t n = 0; n < 8; n++)
    if (name[n] != ' ') found = true;

  if (found) {
    Serial.print("RDS:");
    Serial.print(name);
    Serial.println('.');
  }
} // DisplayServiceName()


/// Execute a command identified by a character and an optional number.
/// See the "?" command for available commands.
/// \param cmd The command character.
/// \param value An optional parameter for the command.
void runSerialCommand(char cmd, int16_t value)
{
  unsigned long startSeek; // after 300 msec must be tuned. after 500 msec must have RDS.
  RADIO_FREQ fSave, fLast;
  RADIO_FREQ f = radio.getMinFrequency();
  RADIO_FREQ fMax = radio.getMaxFrequency();
  char sFreq[12];
  RADIO_INFO ri;

  if (cmd == '?') {
    Serial.println();
    Serial.println("? Help");
    Serial.println("+ increase volume");
    Serial.println("- decrease volume");
    Serial.println("1 start scan version 1");
    Serial.println("2 start scan version 2");
    Serial.println(". scan up   : scan up to next sender");
    Serial.println(", scan down ; scan down to next sender");
    Serial.println("i station status");
    Serial.println("s mono/stereo mode");
    Serial.println("b bass boost");
    Serial.println("u mute/unmute");
  }

  // ----- control the volume and audio output -----

  else if (cmd == '+') {
    // increase volume
    int v = radio.getVolume();
    if (v < 15) radio.setVolume(++v);
  } else if (cmd == '-') {
    // decrease volume
    int v = radio.getVolume();
    if (v > 0) radio.setVolume(--v);
  }

  else if (cmd == 'u') {
    // toggle mute mode
    radio.setMute(!radio.getMute());
  }

  // toggle stereo mode
  else if (cmd == 's') { radio.setMono(!radio.getMono()); }

  // toggle bass boost
  else if (cmd == 'b') { radio.setBassBoost(!radio.getBassBoost()); }

  // ----- control the frequency -----

  else if (cmd == '1') {
    Serial.println("Scanning all available frequencies... (1)");
    fSave = radio.getFrequency();

    // start Simple Scan: all channels
    while (f <= fMax) {
      radio.setFrequency(f);
      delay(50);

      radio.getRadioInfo(&ri);
      if (ri.tuned) {
        radio.formatFrequency(sFreq, sizeof(sFreq));
        Serial.print(sFreq);
        Serial.print(' ');

        Serial.print(ri.rssi); Serial.print(' ');
        Serial.print(ri.snr); Serial.print(' ');
        Serial.print(ri.stereo ? 'S' : '-');
        Serial.print(ri.rds ? 'R' : '-');
        Serial.println();
      } // if

      // tune up by 1 step
      f += radio.getFrequencyStep();
    } // while
    radio.setFrequency(fSave);
    Serial.println();

  } else if (cmd == '2') {
    Serial.println("Seeking all frequencies... (2)");
    fSave = radio.getFrequency();

    // start Scan
    radio.setFrequency(f);

    while (f <= fMax) {
      radio.seekUp(true);
      delay(100); // 
      startSeek = millis();

      // wait for seek complete
      do {
        radio.getRadioInfo(&ri);
      } while ((!ri.tuned) && (startSeek + 300 > millis()));

      // check frequency
      f = radio.getFrequency();
      if (f < fLast) {
        break;
      }
      fLast = f;

      if ((ri.tuned) && (ri.rssi > 42) && (ri.snr > 12)) {
        radio.checkRDS();

        // print frequency.
        radio.formatFrequency(sFreq, sizeof(sFreq));
        Serial.print(sFreq);
        Serial.print(' ');

        do {
          radio.checkRDS();
          // Serial.print(g_block1); Serial.print(' ');
        } while ((!g_block1) && (startSeek + 600 > millis()));

        // fetch final status for printing
        radio.getRadioInfo(&ri);
        Serial.print(ri.rssi); Serial.print(' ');
        Serial.print(ri.snr); Serial.print(' ');
        Serial.print(ri.stereo ? 'S' : '-');
        Serial.print(ri.rds ? 'R' : '-');

        if (g_block1) {
          Serial.print(' ');
          Serial.print('[');  Serial.print(g_block1, HEX); Serial.print(']');
        } // if
        Serial.println();
      } // if
    } // while
    radio.setFrequency(fSave);
    Serial.println();


  } else if (cmd == 'f') { radio.setFrequency(value); }

  else if (cmd == '.') { radio.seekUp(false); } else if (cmd == ':') { radio.seekUp(true); } else if (cmd == ',') { radio.seekDown(false); } else if (cmd == ';') { radio.seekDown(true); }


  // not in help:
  else if (cmd == '!') {
    if (value == 0) radio.term();
    if (value == 1) radio.init();

  } else if (cmd == 'i') {
    char s[12];
    radio.formatFrequency(s, sizeof(s));
    Serial.print("Station:"); Serial.println(s);
    Serial.print("Radio:"); radio.debugRadioInfo();
    Serial.print("Audio:"); radio.debugAudioInfo();

  } // info

  else if (cmd == 'x') {
    radio.debugStatus(); // print chip specific data.
  }
} // runSerialCommand()


/// Setup a FM only radio configuration with I/O for commands and debugging on the Serial port.
void setup() {
  // open the Serial port
  Serial.begin(57600);
  Serial.print("Radio...");
  delay(500);

  // Initialize the Radio 
  radio.init();

  // Enable information to the Serial port
  radio.debugEnable();

  radio.setBandFrequency(RADIO_BAND_FM, 8930);

  // delay(100);

  radio.setMono(false);
  radio.setMute(false);
  // radio.debugRegisters();
  radio.setVolume(5);

  Serial.write('>');

  // setup the information chain for RDS data.
  radio.attachReceiveRDS(RDS_process);
  rds.attachServicenNameCallback(DisplayServiceName);

  runSerialCommand('?', 0);
} // Setup


/// Constantly check for serial input commands and trigger command execution.
void loop() {
  char c;
  if (Serial.available() > 0) {
    // read the next char from input.
    c = Serial.read();
    runSerialCommand(c, 8930);
  } // if

  // check for RDS data
  radio.checkRDS();


} // loop

// End.