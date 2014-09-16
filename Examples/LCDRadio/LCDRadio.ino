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
/// Open the Serial console with 57600 baud to see current radio information and change various settings.
///
/// Wiring
/// ------
/// The necessary wiring of the various chips are described in the Testxxx example sketches.
/// The boards have to be connected by using the following connections:
/// 
/// Arduino port | SI4703 signal | RDA5807M signal
/// :----------: | :-----------: | :-------------:
///          GND | GND           | GND   
///         3.3V | VCC           | -
///           5V | -             | VCC
///           A5 | SCLK          | SCLK
///           A4 | SDIO          | SDIO
///           D2 | RST           | -
///
///
/// More documentation and source code is available at http://www.mathertel.de/Arduino
///
/// History:
/// --------
/// * 05.08.2014 created.



#include <Wire.h>

#include <RADIO.h>
#include <RDA5807M.h>
#include <si4703.h>
#include <TEA5767.h>

#include <RDSParser.h>

#include <LiquidCrystal_PCF8574.h>


// Define some stations available at your locations here:
// 89.40 MHz as 8940

// RADIO_FREQ preset[] = {8850, 8930, 9320,9350, 9450,9570, 9680, 9880, 10030, 10260, 10400, 10500, 10600,10650,10800};

RADIO_FREQ preset[] = {
  8770,
  8810, // hr1
  8820,
  8850, // Bayern2
  8890, // ???
  8930, // * hr3
  8980,
  9180,
  9220, 9350,
  9440, // * hr1
  9510, // - Antenne Frankfurt
  9530,
  9570, 9680, 9880,
  10110, // SWR3
  10030, 10260, 10380, 10400,
  10500 // * FFH
};
// , 10650,10650,10800

int    i_sidx=1;        // Start at Station with index=4
int    i_smax=14;       // Max Index of Stations

/// The radio object has to be defined by using the class corresponding to the used chip.
/// by uncommenting the right radio object definition.

// RADIO radio;    // Create an instance of a non functional radio.
//RDA5807M radio;    // Create an instance of a RDA5807 chip radio
SI4703   radio;    // Create an instance of a SI4703 chip radio.
// TEA5767  radio;    // Create an instance of a TEA5767 chip radio.

/// The lcd object has to be defined by using a LCD library that supports the standard functions
/// When using a I2C->LCD library ??? the I2C bus can be used to control then radio chip and the lcd. 

// LCD lcd();
LiquidCrystal_PCF8574 lcd(0x27);  // set the LCD address to 0x27 for a 16 chars and 2 line display

/// get a RDS parser
RDSParser rds;
// todo #include "RDS_Parser.h"


/// State definition for this radio implementation.
enum RADIO_STATE {
  STATE_PARSECOMMAND, ///< waiting for a new command character.
  
  STATE_PARSEINT,     ///< waiting for digits for the parameter.
  STATE_EXEC          ///< executing the command.
};

RADIO_STATE state; ///< The state variable is used for parsing input characters.

// - - - - - - - - - - - - - - - - - - - - - - - - - -



/// Update the Frequency on the LCD display.
void DisplayFrequency(RADIO_FREQ f)
{
  char s[12];
  lcd.setCursor(0, 0);
  radio.formatFrequency(s, sizeof(s));
  lcd.print(s);
  Serial.print("FREQ:"); Serial.println(s);
} // DisplayFrequency()


/// Update the ServiceName text on the LCD display.
void DisplayServiceName(char *name)
{
  lcd.setCursor(0, 1);
  lcd.print(name);
  Serial.print("RDS:"); Serial.println(name);
} // DisplayServiceName()


// - - - - - - - - - - - - - - - - - - - - - - - - - -


void RDS_process(uint16_t block1, uint16_t block2, uint16_t block3, uint16_t block4) {
  rds.processData(block1, block2, block3, block4);
}


/// Setup a FM only radio configuration with I/O for commands and debugging on the Serial port.
void setup() {
  // open the Serial port
  Serial.begin(57600);

  // Initialize the lcd
  lcd.begin(16, 2);
  lcd.setBacklight(1);
  lcd.print("Radio...");
  delay(800);
  lcd.clear();

  // Initialize the Radio 
  radio.init();

  // Enable information to the Serial port
  radio.debugEnable();

  radio.setBandFrequency(RADIO_BAND_FM, 8930); // hr3
  // radio.setFrequency(8930); // hr3
  // radio.setFrequency(10140); // Radio BOB // preset[i_sidx]
  
  delay(100);

  radio.setMono(false);
  // radio.debugRegisters();

  Serial.write('>');
  
  state = STATE_PARSECOMMAND;
  
//   radio.attachReceiveRDS(RDS_process);
//   rds.attachNewServicenName(DisplayServiceName);
  
} // Setup


/// Execute a command identified by a character and an optional number.
/// See the "?" command for available commands.
/// \param cmd The command character.
/// \param value An optional parameter for the command.
void runCommand(char cmd, int16_t value)
{
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
    if (v < 15) radio.setVolume(++v);
  }
  else if (cmd == '-') {
    // decrease volume
    int v = radio.getVolume();
    if (v > 0) radio.setVolume(--v);
  }

  else if (cmd == 'u') {
    // toggle mute mode
    radio.setMute(! radio.getMute());
  }
  
  // toggle stereo mode
  else if (cmd == 's') { radio.setMono(! radio.getMono()); }

  // toggle bass boost
  else if (cmd == 'b') { radio.setBassBoost(! radio.getBassBoost()); }

  // ----- control the frequency -----
  
  else if (cmd == '>') {
    // next preset
    if (i_sidx < (sizeof(preset) / sizeof(RADIO_FREQ))-1) {
      i_sidx++; radio.setFrequency(preset[i_sidx]);
    } // if
  }
  else if (cmd == '<') {
    // previous preset
    if (i_sidx > 0) {
      i_sidx--;
      radio.setFrequency(preset[i_sidx]);
    } // if

  }
  else if (cmd == 'f') { radio.setFrequency(value); }

  else if (cmd == '.') { radio.seekUp(false); }
  else if (cmd == ':') { radio.seekUp(true); }
  else if (cmd == ',') { radio.seekDown(false); }
  else if (cmd == ';') { radio.seekDown(true); }


  // not in help:
  else if (cmd == '!') {
    if (value == 0) radio.term();
    if (value == 1) radio.init();

  }
  else if (cmd == 'i') {
    char s[12];
    radio.formatFrequency(s, sizeof(s));
    Serial.print("Station:"); Serial.println(s);
    Serial.print("Radio:"); radio.debugRadioInfo();
    Serial.print("Audio:"); radio.debugAudioInfo();

//     Serial.print("  RSSI: ");
//     Serial.print(info.rssi);
//     
//     for (uint8_t i = 0; i < info.rssi - 15; i+=2) { Serial.write('*'); } // Empfangspegel ab 15. Zeichen
//     Serial.println();
  } // info
//  else if (cmd == 'n') { radio.debugScan(); }
  else if (cmd == 'x') { radio.debugStatus(); }
    

} // runCommand()


/// Constantly check for serial input commands and trigger command execution.
void loop() {
  unsigned long now = millis();
  static unsigned long nextTime = 0;
  
  // some internal static values for parsing the input
  static char command;
  static int16_t value;
  static RADIO_FREQ lastf = 0;
  RADIO_FREQ f = 0;
  
  char c;
  if (Serial.available() > 0) {
    // read the next char from input.
    c = Serial.peek();

    if ((state == STATE_PARSECOMMAND) && (c < 0x20)) {
      // ignore unprintable chars
      Serial.read();

    }
    else if (state == STATE_PARSECOMMAND) {
      // read a command.
      command = Serial.read();
      state = STATE_PARSEINT;

    }
    else if (state == STATE_PARSEINT) {
      if ((c >= '0') && (c <= '9')) {
        // build up the value.
        c = Serial.read();
        value = (value * 10) + (c - '0');
      }
      else {
        // not a value -> execute
        runCommand(command, value);
        command = ' ';
        state = STATE_PARSECOMMAND;
        value = 0;
      } // if
    } // if
  } // if

  radio.checkRDS();

  // update the display from time to time
  if (now > nextTime) {
    RADIO_INFO info;
    
    f = radio.getFrequency();
    if (f != lastf) {
      // don't display a Service Name while frequency is no stable.
      DisplayServiceName("        ");
      DisplayFrequency(f);
      lastf = f;
    } // if

    radio.getRadioInfo(&info);
    lcd.setCursor(14, 0);
    lcd.print(info.rssi);
    nextTime = now + 200;
  } // update

} // loop

// End.

