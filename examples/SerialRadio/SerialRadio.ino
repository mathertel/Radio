///
/// \file SerialRadio.ino
/// \brief Radio implementation using the Serial communication.
///
/// \author Matthias Hertel, http://www.mathertel.de
/// \copyright Copyright (c) by Matthias Hertel.\n
/// This work is licensed under a BSD 3-Clause license.\n
/// See http://www.mathertel.de/License.aspx
///
/// \details
/// This is a Arduino sketch radio implementation that can be controlled using commands on the Serial input.
/// It can be used with various chips after adjusting the radio object definition.\n
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
/// * 05.08.2014 created.
/// * 04.10.2014 working.
/// * 15.01.2023 ESP32, cleanup compiler warnings.

#include <Arduino.h>
#include <Wire.h>

#include <radio.h>

// all possible radio chips included.
#include <RDA5807M.h>
#include <RDA5807FP.h>
#include <SI4703.h>
#include <SI4705.h>
#include <SI47xx.h>
#include <TEA5767.h>

#include <RDSParser.h>


// Define some stations available at your locations here:
// 89.30 MHz as 8930

RADIO_FREQ preset[] = {
  8930,   // Sender:<  hr3   >
  9060,   // Sender:<  hr1   >
  9310,   //
  9340,   // Sender:<BAYERN 3>
  9440,   // Sender:<  hr1   >
  9530,   // Sender:< YOU FM >
  9670,   // Sender:<  hr2   >
  9870,   // Sender:<  Dlf   >
  10020,  // Sender:< planet >
  10140,  // Sender:<RADIOBOB>
  10160,  // Sender:<  hr4   >
  10300   // Sender:<ANTENNE >
};

uint16_t presetIndex = 0;  ///< Start at Station with index = 1


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


// Standard I2C/Wire pins for Arduino UNO:  = SDA:A4, SCL:A5
// Standard I2C/Wire pins for ESP8266: SDA:D2, SCL:D1
// Standard I2C/Wire pins for ESP32: SDA:21, SCL:22

/// The radio object has to be defined by using the class corresponding to the used chip.
/// by uncommenting the right radio object definition.

// RADIO radio;     ///< Create an instance of a non functional radio.
// RDA5807FP radio; ///< Create an instance of a RDA5807FP chip radio
// RDA5807M radio;  ///< Create an instance of a RDA5807M chip radio
SI4703 radio;  ///< Create an instance of a SI4703 chip radio.
// SI4705   radio;  ///< Create an instance of a SI4705 chip radio.
// SI47xx radio;    ///<  Create an instance of a SI4720,21,30 (and maybe more) chip radio.
// TEA5767  radio;  ///< Create an instance of a TEA5767 chip radio.

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

bool lowLevelDebug = false;


/// Update the Frequency on the LCD display.
void DisplayFrequency() {
  char s[12];
  radio.formatFrequency(s, sizeof(s));
  Serial.print("FREQ:");
  Serial.println(s);
}  // DisplayFrequency()


/// Update the ServiceName text on the LCD display.
void DisplayServiceName(const char *name) {
  Serial.print("RDS:");
  Serial.println(name);
}  // DisplayServiceName()


// - - - - - - - - - - - - - - - - - - - - - - - - - -


void RDS_process(uint16_t block1, uint16_t block2, uint16_t block3, uint16_t block4) {
  rds.processData(block1, block2, block3, block4);
}


/// Execute a command identified by a character and an optional number.
/// See the "?" command for available commands.
/// \param cmd The command character.
/// \param value An optional parameter for the command.
void runSerialCommand(char cmd, int16_t value) {
  if (cmd == '?') {
    Serial.println();
    Serial.println("? Help");
    Serial.println("+ increase volume");
    Serial.println("- decrease volume");
    Serial.println("> next preset");
    Serial.println("< previous preset");
    Serial.println(". scan up   : scan up to next sender");
    Serial.println(", scan down ; scan down to next sender");
    Serial.println("fnnnnn: direct frequency input");
    Serial.println("i station status");
    Serial.println("s mono/stereo mode");
    Serial.println("b bass boost");
    Serial.println("u mute/unmute");
  }

  // ----- control the volume and audio output -----

  else if (cmd == '+') {
    // increase volume
    int v = radio.getVolume();
    radio.setVolume(++v);
  } else if (cmd == '-') {
    // decrease volume
    int v = radio.getVolume();
    if (v > 0)
      radio.setVolume(--v);
  }

  else if (cmd == 'u') {
    // toggle mute mode
    radio.setMute(!radio.getMute());
  }

  // toggle stereo mode
  else if (cmd == 's') {
    radio.setMono(!radio.getMono());
  }

  // toggle bass boost
  else if (cmd == 'b') {
    radio.setBassBoost(!radio.getBassBoost());
  }

  // ----- control the frequency -----

  else if (cmd == '>') {
    // next preset
    if (presetIndex < (sizeof(preset) / sizeof(RADIO_FREQ)) - 1) {
      presetIndex++;
      radio.setFrequency(preset[presetIndex]);
    }  // if
  } else if (cmd == '<') {
    // previous preset
    if (presetIndex > 0) {
      presetIndex--;
      radio.setFrequency(preset[presetIndex]);
    }  // if

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
  }


  else if (cmd == '!') {
    // not in help
    RADIO_FREQ f = radio.getFrequency();
    if (value == 0) {
      radio.term();
    } else if (value == 1) {
      radio.initWire(Wire);
      radio.setBandFrequency(RADIO_BAND_FM, f);
      radio.setVolume(10);
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
  Serial.println("SerialRadio...");
  delay(200);

#if defined(RESET_PIN)
  // This is required for SI4703 chips:
  radio.setup(RADIO_RESETPIN, RESET_PIN);
  radio.setup(RADIO_MODEPIN, MODE_PIN);
#endif

  // Enable information to the Serial port
  radio.debugEnable(true);
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

  radio.setBandFrequency(RADIO_BAND_FM, preset[presetIndex]);

  radio.setMono(false);
  radio.setMute(false);
  radio.setVolume(radio.getMaxVolume() / 2);

  // setup the information chain for RDS data.
  radio.attachReceiveRDS(RDS_process);
  rds.attachServiceNameCallback(DisplayServiceName);

  runSerialCommand('?', 0);
  kbState = STATE_PARSECOMMAND;
}  // Setup


/// Constantly check for serial input commands and trigger command execution.
void loop() {
  unsigned long now = millis();
  static unsigned long nextFreqTime = 0;

  // some internal static values for parsing the input
  static RADIO_FREQ lastFrequency = 0;
  RADIO_FREQ f = 0;

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

  // update the display from time to time
  if (now > nextFreqTime) {
    f = radio.getFrequency();
    if (f != lastFrequency) {
      // print current tuned frequency
      DisplayFrequency();
      lastFrequency = f;
    }  // if
    nextFreqTime = now + 400;
  }  // if

}  // loop

// End.
