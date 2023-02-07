///
/// \file ScanRadio.ino
/// \brief This sketch implements a FM scanner that lists all availabe radio stations including some information.
///
/// \author Matthias Hertel, http://www.mathertel.de
/// \copyright Copyright (c) by Matthias Hertel.\n
/// This work is licensed under a BSD 3-Clause license.\n
/// See http://www.mathertel.de/License.aspx
///
/// \details
/// This is a Arduino sketch radio implementation that can be controlled using commands on the Serial input.
/// There are some experimental scan algorythms implemented that uses a state machine to scan through all radio stations
/// the radio chip can detect and outputs them on the Serial interface.
/// Open the Serial console with 115200 baud to see current radio information and change various settings.
///
/// Wiring
/// ------
/// The necessary wiring of the various chips are described in the Testxxx example sketches.
/// No additional components are required because all is done through the serial interface.
///
/// More documentation is available at http://www.mathertel.de/Arduino
/// Source Code is available on https://github.com/mathertel/Radio
///
/// History:
/// --------
/// * 17.05.2015 created.
/// * 27.05.2015 first version is working (beta with SI4705).
/// * 04.07.2015 2 scan algorithms working with good results with SI4705.
/// * 18.09.2020 more RDS output, better command handling.

#include <Arduino.h>
#include <Wire.h>
#include <radio.h>

// all possible radio chips included.
#include <RDA5807M.h>
#include <SI4703.h>
#include <SI4705.h>
#include <SI47xx.h>
#include <TEA5767.h>

#include <RDSParser.h>


// ===== SI4703 specific pin wiring =====
#define ENABLE_SI4703

#ifdef ENABLE_SI4703
#if defined(ARDUINO_ARCH_AVR)
#define RESET_PIN 2
#define MODE_PIN A4  // same as SDA

#elif defined(ESP8266)
#define RESET_PIN D5
#define MODE_PIN D2  // same as SDA

#elif defined(ESP32)
#define RESET_PIN 4
#define MODE_PIN 21  // same as SDA

#endif
#endif


/// The radio object has to be defined by using the class corresponding to the used chip.
/// by uncommenting the right radio object definition.

/// Create the radio instance that fits the current chip:
// RDA5807M radio;  ///< Create an instance of a RDA5807 chip radio
SI4703 radio;  ///< Create an instance of a SI4703 chip radio.
// SI4705 radio;    ///< Create an instance of a SI4705 chip radio.
// SI47xx radio; ///< Create an instance of a SI4705 chip radio.
// TEA5767  radio;  ///< Create an instance of a TEA5767 chip radio.

// Standard I2C/Wire pins for Arduino UNO:  = SDA:A4, SCL:A5
// Standard I2C/Wire pins for ESP8266: SDA:D2, SCL:D1
// Standard I2C/Wire pins for ESP32: SDA:21, SCL:22

/// get a RDS parser
RDSParser rds;


/// State of Keyboard input for this radio implementation.
enum RADIO_STATE {
  STATE_PARSECOMMAND,  ///< waiting for a new command character.
  STATE_PARSEINT,      ///< waiting for digits for the parameter.
  STATE_EXEC           ///< executing the command.
};

RADIO_STATE kbState;  ///< The state of parsing input characters.
char kbCommand;
int16_t kbValue;

uint16_t g_block1;
bool radioDebug = false;
bool lowLevelDebug = false;
String lastServiceName;

// - - - - - - - - - - - - - - - - - - - - - - - - - -

// use a function in between the radio chip and the RDS parser
// to catch the block1 value (used for sender identification)
void RDS_process(uint16_t block1, uint16_t block2, uint16_t block3, uint16_t block4) {
  // Serial.printf("RDS: 0x%04x 0x%04x 0x%04x 0x%04x\n", block1, block2, block3, block4);
  g_block1 = block1;
  rds.processData(block1, block2, block3, block4);
}

/// Update the Time
void DisplayTime(uint8_t hour, uint8_t minute) {
  Serial.print("Time: ");
  if (hour < 10)
    Serial.print('0');
  Serial.print(hour);
  Serial.print(':');
  if (minute < 10)
    Serial.print('0');
  Serial.println(minute);
}  // DisplayTime()


/// Update the ServiceName text on the LCD display.
void DisplayServiceName(const char *name) {
  bool found = false;

  for (uint8_t n = 0; n < 8; n++)
    if (name[n] != ' ')
      found = true;

  if (found) {
    lastServiceName = name;
    Serial.print("Sender:<");
    Serial.print(name);
    Serial.println('>');
  }
}  // DisplayServiceName()


/// Update the ServiceName text on the LCD display.
void DisplayText(const char *txt) {
  Serial.print("Text: <");
  Serial.print(txt);
  Serial.println('>');
}  // DisplayText()


void PrintScanInfo(RADIO_INFO *ri) {
  char sFreq[12];
  radio.formatFrequency(sFreq, sizeof(sFreq));
  Serial.print(sFreq);
  Serial.print(' ');

  Serial.print(ri->rssi);
  Serial.print(' ');
  // Serial.print(ri->snr);
  // Serial.print(' ');
  Serial.print(ri->tuned ? 'T' : '-');
  Serial.print(ri->stereo ? 'S' : '-');
  Serial.print(ri->rds ? 'R' : '-');
  Serial.println();
}

/// Execute a command identified by a character and an optional number.
/// See the "?" command for available commands.
/// \param cmd The command character.
/// \param value An optional parameter for the command.
void runSerialCommand(char cmd, int16_t value) {
  unsigned long startSeek;  // after 300 msec must be tuned. after 500 msec must have RDS.
  RADIO_FREQ fSave, fLast = 0;
  RADIO_FREQ f = radio.getMinFrequency();
  RADIO_FREQ fMax = radio.getMaxFrequency();
  RADIO_INFO ri;

  if ((cmd == '\n') || (cmd == '\r')) {
    return;
  }

  Serial.print("do:");
  Serial.println(cmd);

  if (cmd == '?') {
    Serial.println();
    Serial.println("? Help");
    Serial.println("+ increase volume");
    Serial.println("- decrease volume");
    Serial.println("1 scan Frequency + Data");
    Serial.println("2 scan version 2");
    Serial.println("3 scan RDS stations");
    Serial.println(". scan up   : scan up to next sender");
    Serial.println(", scan down ; scan down to next sender");
    Serial.println("i station status");
    Serial.println("s mono/stereo mode");
    Serial.println("b bass boost");
    Serial.println("m mute/unmute");
    Serial.println("u soft mute/unmute");
    Serial.println("x debug...");
    Serial.println("y toggle Debug Messages...");
    Serial.println("* toggle i2c debug output");

    // ----- control the volume and audio output -----

  } else if (cmd == '+') {
    // increase volume
    int v = radio.getVolume();
    if (v < 15)
      radio.setVolume(++v);
  } else if (cmd == '-') {
    // decrease volume
    int v = radio.getVolume();
    if (v > 0)
      radio.setVolume(--v);

  } else if (cmd == 'm') {
    // toggle mute mode
    radio.setMute(!radio.getMute());

  } else if (cmd == 'u') {
    // toggle soft mute mode
    radio.setSoftMute(!radio.getSoftMute());

  } else if (cmd == 's') {
    // toggle stereo mode
    radio.setMono(!radio.getMono());

  } else if (cmd == 'b') {
    // toggle bass boost
    radio.setBassBoost(!radio.getBassBoost());


  } else if (cmd == '1') {
    // ----- control the frequency -----
    Serial.println("Scanning all available frequencies... (1)");
    fSave = radio.getFrequency();

    // start Simple Scan: all channels
    while (f <= fMax) {
      radio.setFrequency(f);
      delay(100);

      radio.getRadioInfo(&ri);

      // you may adjust the following condition to adjust to the chip you use.
      // some do not report snr or tuned and good rssi differs.

      if (true) {  // print all frequencies
      // if (ri.stereo) {  // print all stereo frequencies
      // if (ri.rssi >= 32) {  // si4703 usable threshold value

        // PrintScanInfo(&ri);

        RADIO_FREQ f = radio.getFrequency();
        Serial.print(f);
        Serial.print(',');
        Serial.print(ri.stereo);
        Serial.print(',');
        Serial.print(ri.rds);
        Serial.print(',');
        Serial.println(ri.rssi);
      }

      // tune up by 1 step
      f += radio.getFrequencyStep();
    }  // while
    Serial.println("done.");
    radio.setFrequency(fSave);

  } else if (cmd == '2') {
    Serial.println("Seeking all frequencies... (2)");
    fSave = radio.getFrequency();

    // start Scan
    radio.setFrequency(f);

    while (f <= fMax) {
      radio.seekUp(true);
      delay(100);  //
      startSeek = millis();

      // wait for seek complete
      do {
        radio.getRadioInfo(&ri);
      } while ((!ri.tuned) && (startSeek + 600 > millis()));

      // check frequency
      f = radio.getFrequency();
      if (f < fLast) {
        break;
      }
      fLast = f;

      // you may adjust the following condition to adjust to the chip you use.
      // some do not report snr or tuned and good rssi differs.

      if (true) {  // every frequency detected by builtin scan
      // if ((ri.tuned) && (ri.rssi > 34) && (ri.snr > 12)) {
      // if (ri.rssi >= 32) {  // si4703 threshold value

        radio.getRadioInfo(&ri);
        PrintScanInfo(&ri);
      }  // if
    }    // while
    Serial.println("done.");
    radio.setFrequency(fSave);


  } else if (cmd == '3') {
    Serial.println("Seeking all RDS senders...");
    fSave = radio.getFrequency();

    // start Scan
    radio.setFrequency(f);

    // start Simple Scan: all channels
    while (f <= fMax) {
      radio.setFrequency(f);
      lastServiceName.clear();
      delay(100);

      radio.getRadioInfo(&ri);

      // you may adjust the following condition to adjust to the chip you use.
      // some do not report snr or tuned and good rssi differs.

      // if (true) {  // print all frequencies
      // if (ri.stereo) {  // print all stereo frequencies

      if (ri.rssi >= 32) {  // si4703 usable threshold value
        if (ri.rds) {
          radio.checkRDS();
          PrintScanInfo(&ri);
          startSeek = millis();

          // wait 3 secs for sender name
          do {
            radio.checkRDS();
            delay(30);
          } while (lastServiceName.isEmpty() && (startSeek + 6000 > millis()));
        }  // if
      }

      // tune up by 1 step
      f += radio.getFrequencyStep();
    }  // while
    Serial.println("done.");
    radio.setFrequency(fSave);
  } else if (cmd == 'f') {
    radio.setFrequency(value);
  }

  else if (cmd == '.') {
    radio.seekUp(false);
  } else if (cmd == ':') {
    radio.seekUp(true);
  } else if (cmd == ',') {
    radio.seekDown(false);
  } else if (cmd == ';') {
    radio.seekDown(true);

  } else if (cmd == '!') {
    // not in help
    RADIO_FREQ f = radio.getFrequency();
    if (value == 0) {
      radio.term();
    } else if (value == 1) {
      radio.init();
      radio.setBandFrequency(RADIO_BAND_FM, f);
    }
  } else if (cmd == 'i') {
    // info
    char s[12];
    radio.formatFrequency(s, sizeof(s));
    Serial.print("Station:");
    Serial.println(s);
    Serial.print("Radio:");
    radio.debugRadioInfo();
    Serial.print("Audio:");
    radio.debugAudioInfo();
  } else if (cmd == 'x') {
    radio.debugStatus();  // print chip specific data.
  } else if (cmd == 'y') {
    radioDebug = !radioDebug;
    radio.debugEnable(radioDebug);
  } else if (cmd == '*') {
    lowLevelDebug = !lowLevelDebug;
    radio._wireDebug(lowLevelDebug);
  }
}  // runSerialCommand()


/// Setup a FM only radio configuration with I/O for commands and debugging on the Serial port.
void setup() {
  delay(3000);

  // open the Serial port
  Serial.begin(115200);
  Serial.println("ScanRadio...");
  delay(200);

#if defined(RESET_PIN)
  // This is required for SI4703 chips:
  radio.setup(RADIO_RESETPIN, RESET_PIN);
  radio.setup(RADIO_MODEPIN, MODE_PIN);
#endif

  // Enable information to the Serial port
  radio.debugEnable(radioDebug);
  radio._wireDebug(lowLevelDebug);

  // Set FM Options for Europe
  radio.setup(RADIO_FMSPACING, RADIO_FMSPACING_100);   // for EUROPE
  radio.setup(RADIO_DEEMPHASIS, RADIO_DEEMPHASIS_50);  // for EUROPE

  // Initialize the Radio
  if (!radio.initWire(Wire)) {
    Serial.println("no radio chip found.");
    delay(4000);
    while (1) {};
  };

  radio.setBandFrequency(RADIO_BAND_FM, 8930);

  radio.setMono(false);
  radio.setMute(false);
  radio.setVolume(5);

  // setup the information chain for RDS data.
  radio.attachReceiveRDS(RDS_process);
  rds.attachServiceNameCallback(DisplayServiceName);
  // rds.attachTextCallback(DisplayText);
  // rds.attachTimeCallback(DisplayTime);

  runSerialCommand('?', 0);
  kbState = STATE_PARSECOMMAND;
}  // Setup


/// Constantly check for serial input commands and trigger command execution.
void loop() {
  if (Serial.available() > 0) {
    // read the next char from input.
    char c = Serial.peek();

    if ((kbState == STATE_PARSECOMMAND) && (c < 0x20)) {
      // ignore unprintable chars
      Serial.read();

    } else if (kbState == STATE_PARSECOMMAND) {
      // read a command.
      kbCommand = Serial.read();
      kbState = STATE_PARSEINT;

    } else if (kbState == STATE_PARSEINT) {
      if ((c >= '0') && (c <= '9')) {
        // build up the value.
        c = Serial.read();
        kbValue = (kbValue * 10) + (c - '0');
      } else {
        // not a value -> execute
        runSerialCommand(kbCommand, kbValue);
        kbCommand = ' ';
        kbState = STATE_PARSECOMMAND;
        kbValue = 0;
      }  // if
    }    // if
  }      // if

  // check for RDS data
  radio.checkRDS();


}  // loop

// End.
