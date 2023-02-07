///
/// \file LCDRadio.ino
/// \brief Radio implementation including LCD output.
///
/// \author Matthias Hertel, http://www.mathertel.de
/// \copyright Copyright (c) 2014 by Matthias Hertel.\n
/// This work is licensed under a BSD style license.\n
/// See http://www.mathertel.de/License.aspx
///
/// \details
/// This is a full function radio implementation that uses a LCD display to show the current station information.\n
/// It can be used with various chips after adjusting the radio object definition.\n
/// Open the Serial console with 115200 baud to see current radio information and change various settings.
///
/// Wiring
/// ------
/// The necessary wiring of the various chips are described in the Testxxx example sketches.
/// The boards have to be connected by using the following connections:
///
/// More documentation is available at http://www.mathertel.de/Arduino
/// Source Code is available on https://github.com/mathertel/Radio
///
/// History:
/// --------
/// * 05.08.2014 created.
/// * 06.10.2014 working.
/// * 16.01.2023 improved portable interrupt handling.
/// * 16.01.2023 ESP8266 adaption and fixes.


#include <Arduino.h>
#include <Wire.h>

#include <radio.h>

// all possible radio chips included.
#include <RDA5807M.h>
#include <SI4703.h>
#include <SI47xx.h>
#include <TEA5767.h>

#include <RDSParser.h>

#include <LiquidCrystal_PCF8574.h>

#include <RotaryEncoder.h>
#include <OneButton.h>


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

// ===== rotator and menu button specific pin wiring =====

#if defined(ARDUINO_ARCH_AVR)
#define ROT_PIN1 A2
#define ROT_PIN2 A3
#define MENU_PIN

#elif defined(ESP8266)
#define ROT_PIN1 D6
#define ROT_PIN2 D7
#define MENU_PIN D3

#elif defined(ESP32)
#error ESP32 adaption is missing in this sketch

#endif

// The https://github.com/mathertel/RotaryEncoder library is used
// to process signals from the RotaryEncoder
RotaryEncoder encoder(ROT_PIN1, ROT_PIN2);

// The https://github.com/mathertel/OneButton library is used
// to process signals from a momentary input
OneButton menuButton(MENU_PIN, true);

int encoderLastPos;
unsigned long encoderLastTime;


// variables for rotator encoder
// unsigned long lastRotatorChange = 0; // last time a change of the rotator was detected.

/// The radio object has to be defined by using the class corresponding to the used chip.
/// by uncommenting the right radio object definition.

// Standard I2C/Wire pins for Arduino UNO:  = SDA:A4, SCL:A5
// Standard I2C/Wire pins for ESP8266: SDA:D2, SCL:D1
// Standard I2C/Wire pins for ESP32: SDA:21, SCL:22

// RADIO radio;    // Create an instance of a non functional radio.
// RDA5807M radio;    // Create an instance of a RDA5807 chip radio
SI4703 radio;  // Create an instance of a SI4703 chip radio.
// SI47xx   radio;    // Create an instance of a SI4721 chip radio.
// TEA5767  radio;    // Create an instance of a TEA5767 chip radio.


// The https://github.com/mathertel/LiquidCrystal_PCF8574 library is used
// to control the LCD via I2C using a PCF8574 based i2c adapter.

LiquidCrystal_PCF8574 lcd(0x27);  // set the LCD address to 0x27 for a 16 chars and 2 line display

/// get a RDS parser
RDSParser rds;


/// State definition for this radio implementation.
enum RADIO_STATE {
  STATE_NONE = 0,
  STATE_PARSECOMMAND,  ///< waiting for a new command character.
  STATE_PARSEINT,      ///< waiting for digits for the parameter.
  STATE_EXEC,          ///< executing the command.

  STATE_FREQ,
  STATE_VOL,
  STATE_MONO,
  STATE_SMUTE
};

RADIO_STATE state;  ///< The state variable is used for parsing input characters.
RADIO_STATE rot_state;

// - - - - - - - - - - - - - - - - - - - - - - - - - -


/// Update the Frequency on the LCD display.
void DisplayFrequency() {
  char s[12];
  radio.formatFrequency(s, sizeof(s));
  Serial.print("FREQ:");
  Serial.println(s);
  lcd.setCursor(0, 0);
  lcd.print(s);
}  // DisplayFrequency()


/// Update the ServiceName text on the LCD display when in RDS mode.
void DisplayServiceName(const char *name) {
  Serial.print("RDS:");
  Serial.println(name);
  if (rot_state == STATE_FREQ) {
    lcd.setCursor(0, 1);
    lcd.print(name);
  }
}  // DisplayServiceName()


/// Update the RDS Text
void DisplayRDSText(const char *txt) {
  Serial.print("Text:");
  Serial.println(txt);
}  // DisplayRDSText()


void DisplayTime(uint8_t hour, uint8_t minute) {
  Serial.print("RDS-Time:");
  if (hour < 10) Serial.print('0');
  Serial.print(hour);
  Serial.print(':');
  if (minute < 10) Serial.print('0');
  Serial.println(minute);
}  // DisplayTime()


/// Display a label and value for several purpose
void DisplayMenuValue(String label, int value) {
  String out = label + ": " + value;

  Serial.println(out);

  lcd.setCursor(0, 1);
  lcd.print(out + "  ");
}


// - - - - - - - - - - - - - - - - - - - - - - - - - -


void RDS_process(uint16_t block1, uint16_t block2, uint16_t block3, uint16_t block4) {
  rds.processData(block1, block2, block3, block4);
}


// this function will be called when the menuButton was clicked
void doMenuClick() {
  Serial.println("doMenuClick()");

  unsigned long now = millis();

  if (rot_state == STATE_FREQ) {
    // jump into volume mode
    rot_state = STATE_VOL;
    encoderLastPos = radio.getVolume();
    encoder.setPosition(encoderLastPos);
    DisplayMenuValue("Vol", encoderLastPos);

  } else if (rot_state == STATE_VOL) {
    // jump into mono/stereo switch
    rot_state = STATE_MONO;
    encoderLastPos = radio.getMono();
    encoder.setPosition(encoderLastPos);
    DisplayMenuValue("Mono", encoderLastPos);

  } else if (rot_state == STATE_MONO) {
    // jump into soft mute switch
    rot_state = STATE_SMUTE;
    encoderLastPos = radio.getSoftMute();
    encoder.setPosition(encoderLastPos);
    DisplayMenuValue("SMute", encoderLastPos);
  } else if (rot_state == STATE_SMUTE) {
    rot_state = STATE_FREQ;
    encoderLastPos = (radio.getFrequency() - radio.getMinFrequency()) / radio.getFrequencyStep();
    encoder.setPosition(encoderLastPos);
    DisplayServiceName("...");

  }  // if
  encoderLastTime = now;

}  // doMenuClick()


// this function will be called when the seekButton was clicked
void doSeekClick() {
  Serial.println("doSeekClick()");
  radio.seekUp(true);
}  // doSeekClick()


// The Interrupt Service Routine for Pin Change Interrupts
// On Arduino UNO you can use the PCINT1 interrupt vector that covers digital value changes on A2 and A3.
// On Arduino Mega 2560  you can use the PCINT2 interrupt vector that covers digital value changes on A8 and A9.
// Read http://www.atmel.com/Images/doc8468.pdf for more details on external interrupts.

#if defined(ARDUINO_ARCH_AVR)
// This interrupt routine will be called on any change of one of the input signals
void checkPosition() {
  encoder.tick();  // just call tick() to check the state.
}

#elif defined(ESP8266)
// This interrupt routine will be called on any change of one of the input signals
IRAM_ATTR void checkPosition() {
  encoder.tick();  // just call tick() to check the state.
}

#endif




/// Setup a FM only radio configuration with I/O for commands and debugging on the Serial port.
void setup() {
  delay(3000);

  // open the Serial port
  Serial.begin(115200);
  Serial.println("LCDRadio...");

  // Initialize the lcd
  lcd.begin(16, 2);
  lcd.setBacklight(1);
  lcd.print("LCDRadio...");

  delay(800);
  lcd.clear();

  attachInterrupt(digitalPinToInterrupt(ROT_PIN1), checkPosition, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ROT_PIN2), checkPosition, CHANGE);

  // This is required for SI4703 chips:
#if defined(RESET_PIN)
  radio.setup(RADIO_RESETPIN, RESET_PIN);
  radio.setup(RADIO_MODEPIN, MODE_PIN);
#endif

  // Enable information to the Serial port
  radio.debugEnable(false);
  radio._wireDebug(false);

  // Initialize the Radio
  radio.initWire(Wire);

  radio.setBandFrequency(RADIO_BAND_FM, preset[presetIndex]);  // 5. preset.

  radio.setMono(false);
  radio.setMute(false);
  radio.setVolume(10);

  encoderLastPos = (radio.getFrequency() - radio.getMinFrequency()) / radio.getFrequencyStep();
  encoder.setPosition(encoderLastPos);

  // Setup the buttons
  // link the doMenuClick function to be called on a click event.
  menuButton.attachClick(doMenuClick);
  // link the doSeekClick function to be called on a double-click event.
  menuButton.attachDoubleClick(doSeekClick);

  Serial.println('>');

  state = STATE_PARSECOMMAND;
  rot_state = STATE_NONE;

  // setup the information chain for RDS data.
  radio.attachReceiveRDS(RDS_process);

  rds.attachServiceNameCallback(DisplayServiceName);
  rds.attachTextCallback(DisplayRDSText);

  rds.attachTimeCallback(DisplayTime);

  Serial.println("setup done.");
}  // Setup


/// Execute a command identified by a character and an optional number.
/// See the "?" command for available commands.
/// \param cmd The command character.
/// \param value An optional parameter for the command.
void runCommand(char cmd, int16_t value) {
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
    Serial.println("m mute/unmute");
    Serial.println("u soft mute/unmute");
  }

  // ----- control the volume and audio output -----

  else if (cmd == '+') {
    // increase volume
    int v = radio.getVolume() + 1;
    v = constrain(v, 0, 15);
    radio.setVolume(v);
  } else if (cmd == '-') {
    // decrease volume
    int v = radio.getVolume() - 1;
    v = constrain(v, 0, 15);
    radio.setVolume(v);
  }

  else if (cmd == 'm') {
    // toggle mute mode
    radio.setMute(!radio.getMute());
  }

  else if (cmd == 'u') {
    // toggle soft mute mode
    radio.setSoftMute(!radio.getSoftMute());
  }

  // toggle stereo mode
  else if (cmd == 's') { radio.setMono(!radio.getMono()); }

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
      rds.init();
    }  // if
  } else if (cmd == '<') {
    // previous preset
    if (presetIndex > 0) {
      presetIndex--;
      radio.setFrequency(preset[presetIndex]);
      rds.init();
    }  // if

  } else if (cmd == 'f') {
    radio.setFrequency(value);
    rds.init();
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


  // not in help:
  else if (cmd == '!') {
    if (value == 0) radio.term();
    if (value == 1) radio.init();

  } else if (cmd == 'i') {
    char s[12];
    radio.formatFrequency(s, sizeof(s));
    Serial.print("Station:");
    Serial.println(s);
    Serial.print("Radio:");
    radio.debugRadioInfo();
    Serial.print("Audio:");
    radio.debugAudioInfo();

    //     Serial.print("  RSSI: ");
    //     Serial.print(info.rssi);
    //
    //     for (uint8_t i = 0; i < info.rssi - 15; i+=2) { Serial.write('*'); } // Empfangspegel ab 15. Zeichen
    //     Serial.println();
  }  // info
     //  else if (cmd == 'n') { radio.debugScan(); }
  else if (cmd == 'x') { radio.debugStatus(); }


}  // runCommand()


/// Constantly check for serial input commands and trigger command execution.
void loop() {
  int newPos;
  unsigned long now = millis();
  static unsigned long nextFreqTime = 0;
  static unsigned long nextRadioInfoTime = 0;

  // some internal static values for parsing the input
  static char command;
  static int16_t value;
  static RADIO_FREQ lastf = 0;
  RADIO_FREQ f = 0;
  char c;

  // check for the menuButton
  encoder.tick();
  menuButton.tick();

  // check for commands on the Serial input
  if (Serial.available() > 0) {
    // read the next char from input.
    c = Serial.peek();

    if ((state == STATE_PARSECOMMAND) && (c < 0x20)) {
      // ignore unprintable chars
      Serial.read();

    } else if (state == STATE_PARSECOMMAND) {
      // read a command.
      command = Serial.read();
      state = STATE_PARSEINT;

    } else if (state == STATE_PARSEINT) {
      if ((c >= '0') && (c <= '9')) {
        // build up the value.
        c = Serial.read();
        value = (value * 10) + (c - '0');
      } else {
        // not a value -> execute
        runCommand(command, value);
        command = ' ';
        state = STATE_PARSECOMMAND;
        value = 0;
      }  // if
    }    // if
  }      // if

  // check for the rotary encoder changes
  newPos = encoder.getPosition();
  if (newPos != encoderLastPos) {

    if (rot_state == STATE_FREQ) {
      RADIO_FREQ f = radio.getMinFrequency() + (newPos * radio.getFrequencyStep());
      radio.setFrequency(f);
      rds.init();
      encoderLastPos = newPos;
      nextFreqTime = now + 10;

    } else if (rot_state == STATE_VOL) {
      newPos = constrain(newPos, 0, 15);
      encoder.setPosition(newPos);
      radio.setVolume(newPos);
      encoderLastPos = newPos;
      DisplayMenuValue("Vol", encoderLastPos);

    } else if (rot_state == STATE_MONO) {
      radio.setMono(newPos & 0x01);
      encoderLastPos = newPos;
      DisplayMenuValue("Mono", newPos & 0x01);

    } else if (rot_state == STATE_SMUTE) {
      radio.setSoftMute(newPos & 0x01);
      encoderLastPos = newPos;
      DisplayMenuValue("SMute", newPos & 0x01);

    }  // if
    encoderLastTime = now;

  } else if (now > encoderLastTime + 2000) {
    // rotary encoder was not changed since 2 seconds:
    // fall into FREQ + RDS mode and set rotary encoder to frequency mode.
    if (rot_state != STATE_FREQ) {
      DisplayServiceName("...");
      lcd.setCursor(0, 1);
      lcd.print("        ");
      rot_state = STATE_FREQ;
    }
    encoderLastPos = (radio.getFrequency() - radio.getMinFrequency()) / radio.getFrequencyStep();
    if (encoderLastPos != newPos) {
      encoder.setPosition(encoderLastPos);
    }
    encoderLastTime = now;

  }  // if

  // check for RDS data
  radio.checkRDS();

  // update the display from time to time
  if (now > nextFreqTime) {
    f = radio.getFrequency();
    if (f != lastf) {
      // don't display a Service Name while frequency is no stable.
      DisplayFrequency();
      lcd.setCursor(0, 1);
      lcd.print("        ");
      lastf = f;
    }  // if
    nextFreqTime = now + 400;
  }  // if

  if (now > nextRadioInfoTime) {
    RADIO_INFO info;
    radio.getRadioInfo(&info);
    lcd.setCursor(14, 0);
    lcd.print(info.rssi);
    nextRadioInfoTime = now + 1000;
  }  // update

}  // loop

// End.
