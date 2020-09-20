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
/// GND (black)  | GND           | GND   
/// 3.3V (red)   | VCC           | VCC
/// 5V (red)     | -             | -
/// A5 (yellow)  | SCLK          | SCLK
/// A4 (blue)    | SDIO          | SDIO
/// D2           | RST           | -
///
/// More documentation and source code is available at http://www.mathertel.de/Arduino
///
/// History:
/// --------
/// * 05.08.2014 created.
/// * 06.10.2014 working.



#include <Wire.h>

#include <radio.h>
#include <RDA5807M.h>
#include <SI4703.h>
#include <SI4721.h>
#include <TEA5767.h>

#include <RDSParser.h>

#include <LiquidCrystal_PCF8574.h>

#include <RotaryEncoder.h>
#include <OneButton.h>

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
  9560, // Bayern 1
  9680, 9880,
  10020, // planet
  10090, // ffh
  10110, // SWR3
  10030, 10260, 10380, 10400,
  10500 // * FFH
};
// , 10650,10650,10800

int    i_sidx=5;        // Start at Station with index=5
int    i_smax=14;       // Max Index of Stations

/// Setup a RoraryEncoder for pins A2 and A3:
RotaryEncoder encoder(A2, A3);
// RotaryEncoder encoder(A9, A8);

int encoderLastPos;
unsigned long encoderLastTime;


// variables for rotator encoder
//unsigned long lastRotatorChange = 0; // last time a change of the rotator was detected.

/// The radio object has to be defined by using the class corresponding to the used chip.
/// by uncommenting the right radio object definition.

// RADIO radio;    // Create an instance of a non functional radio.
// RDA5807M radio;    // Create an instance of a RDA5807 chip radio
// SI4703   radio;    // Create an instance of a SI4703 chip radio.
SI4721   radio;    // Create an instance of a SI4721 chip radio.
// TEA5767  radio;    // Create an instance of a TEA5767 chip radio.

/// The lcd object has to be defined by using a LCD library that supports the standard functions
/// When using a I2C->LCD library ??? the I2C bus can be used to control then radio chip and the lcd. 

/// get a LCD instance
LiquidCrystal_PCF8574 lcd(0x27);  // set the LCD address to 0x27 for a 16 chars and 2 line display

OneButton menuButton(A10, true);
OneButton seekButton(A11, true);


/// get a RDS parser
RDSParser rds;


/// State definition for this radio implementation.
enum RADIO_STATE {
  STATE_NONE = 0,
  STATE_PARSECOMMAND, ///< waiting for a new command character.
  STATE_PARSEINT,     ///< waiting for digits for the parameter.
  STATE_EXEC,          ///< executing the command.
  
  STATE_FREQ,
  STATE_VOL,
  STATE_MONO,
  STATE_SMUTE
  
};

RADIO_STATE state; ///< The state variable is used for parsing input characters.
RADIO_STATE rot_state;

// - - - - - - - - - - - - - - - - - - - - - - - - - -


/// Update the Frequency on the LCD display.
void DisplayFrequency(RADIO_FREQ f)
{
  char s[12];
  radio.formatFrequency(s, sizeof(s));
  Serial.print("FREQ:"); Serial.println(s);
  lcd.setCursor(0, 0);
  lcd.print(s);
} // DisplayFrequency()


/// Update the ServiceName text on the LCD display when in RDS mode.
void DisplayServiceName(char *name)
{
  Serial.print("RDS:"); Serial.println(name);
  if (rot_state == STATE_FREQ) {
    lcd.setCursor(0, 1);
    lcd.print(name);
  }
} // DisplayServiceName()


void DisplayTime(uint8_t hour, uint8_t minute) {
  Serial.print("RDS-Time:");
  if (hour < 10) Serial.print('0');
  Serial.print(hour);
  Serial.print(':');
  if (minute < 10) Serial.print('0');
  Serial.print(minute);
} // DisplayTime()


/// Display the current volume.
void DisplayVolume(uint8_t v)
{
  Serial.print("VOL: "); Serial.println(v);

  lcd.setCursor(0, 1);
  lcd.print("VOL: "); lcd.print(v);
} // DisplayVolume()


/// Display the current mono switch.
void DisplayMono(uint8_t v)
{
  Serial.print("MONO: "); Serial.println(v);
  lcd.setCursor(0, 1);
  lcd.print("MONO: "); lcd.print(v);
} // DisplayMono()


/// Display the current soft mute switch.
void DisplaySoftMute(uint8_t v)
{
  Serial.print("SMUTE: "); Serial.println(v);
  lcd.setCursor(0, 1);
  lcd.print("SMUTE: "); lcd.print(v);
} // DisplaySoftMute()


// - - - - - - - - - - - - - - - - - - - - - - - - - -


void RDS_process(uint16_t block1, uint16_t block2, uint16_t block3, uint16_t block4) {
  rds.processData(block1, block2, block3, block4);
}


// this function will be called when the menuButton was clicked
void doMenuClick() {
  unsigned long now = millis();

  if (rot_state == STATE_FREQ) {
    // jump into volume mode
    rot_state = STATE_VOL;
    encoderLastPos = radio.getVolume();
    encoder.setPosition(encoderLastPos);
    DisplayVolume(encoderLastPos);

  }
  else if (rot_state == STATE_VOL) {
    // jump into mono/stereo switch
    rot_state = STATE_MONO;
    encoderLastPos = radio.getMono();
    encoder.setPosition(encoderLastPos);
    DisplayMono(encoderLastPos);

  }
  else if (rot_state == STATE_MONO) {
    // jump into soft mute switch
    rot_state = STATE_SMUTE;
    encoderLastPos = radio.getSoftMute();
    encoder.setPosition(encoderLastPos);
    DisplaySoftMute(encoderLastPos);

  }
  else if (rot_state == STATE_SMUTE) {
    rot_state = STATE_FREQ;
    encoderLastPos = (radio.getFrequency() - radio.getMinFrequency()) / radio.getFrequencyStep();
    encoder.setPosition(encoderLastPos);
    DisplayServiceName("...");

  } // if
  encoderLastTime = now;
    
} // doMenuClick()


// this function will be called when the seekButton was clicked
void doSeekClick() {
  Serial.println("SEEK...");
  radio.seekUp(true);
} // doSeekClick()


// The Interrupt Service Routine for Pin Change Interrupts
// On Arduino UNO you can use the PCINT1 interrupt vector that covers digital value changes on A2 and A3.
// On Arduino Mega 2560  you can use the PCINT2 interrupt vector that covers digital value changes on A8 and A9.
// Read http://www.atmel.com/Images/doc8468.pdf for more details on external interrupts.

ISR(PCINT2_vect) {
  encoder.tick(); // just call tick() to check the state.
} // ISR for PCINT2


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

  // radio.setBandFrequency(RADIO_BAND_FM, 8930); // hr3
  radio.setBandFrequency(RADIO_BAND_FM, preset[i_sidx]); // 5. preset.
  // radio.setFrequency(10140); // Radio BOB // preset[i_sidx]

  // Setup rotary encoder

  // You may have to modify the next 2 lines if using other pins than A2 and A3
  // On Arduino-Uno: rotary encoder in A2(PCINT10) and A3(PCINT11)
  // PCICR  |= (1 << PCIE1);    // This enables Pin Change Interrupt 1 that covers the Analog input pins or Port C.
  // PCMSK1 |= (1 << PCINT10) | (1 << PCINT11);  // This enables the interrupt for pin 2 and 3 of Port C.

  // On Arduino-MEGA2560: A8(PCINT16) and A9(PCINT17) for interrupt vector PCINT2
  PCICR |= (1 << PCIE2);
  PCMSK2 |= (1 << PCINT16) | (1 << PCINT17);

  encoderLastPos = (radio.getFrequency() - radio.getMinFrequency()) / radio.getFrequencyStep();
  Serial.println(encoderLastPos);
  encoder.setPosition(encoderLastPos);

  // Setup the buttons
  // link the doMenuClick function to be called on a click event.
  menuButton.attachClick(doMenuClick);

  // link the doSeekClick function to be called on a click event.
  seekButton.attachClick(doSeekClick);

  delay(100);

  radio.setMono(false);
  radio.setMute(false);
  // radio._wireDebug();

  Serial.write('>');
  
  state = STATE_PARSECOMMAND;
  rot_state = STATE_NONE;
  
  // setup the information chain for RDS data.
  radio.attachReceiveRDS(RDS_process);

  rds.attachServicenNameCallback(DisplayServiceName);

  rds.attachTimeCallback(DisplayTime);
 
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
    Serial.println("m mute/unmute");
    Serial.println("u soft mute/unmute");
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

  else if (cmd == 'm') {
    // toggle mute mode
    radio.setMute(! radio.getMute());
  }
  
  else if (cmd == 'u') {
    // toggle soft mute mode
    radio.setSoftMute(! radio.getSoftMute());
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
  menuButton.tick();

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


  // check for the rotary encoder
  newPos = encoder.getPosition();
  if (newPos != encoderLastPos) {

    if (rot_state == STATE_FREQ) {
      RADIO_FREQ f = radio.getMinFrequency() + (newPos *  radio.getFrequencyStep());
      radio.setFrequency(f);
      encoderLastPos = newPos;
      nextFreqTime = now + 10;
    
    }      
    else if (rot_state == STATE_VOL) {
      radio.setVolume(newPos);
      encoderLastPos = newPos;
      DisplayVolume(newPos);
      
    }
    else if (rot_state == STATE_MONO) {
      radio.setMono(newPos & 0x01);
      encoderLastPos = newPos;
      DisplayMono(newPos & 0x01);

    }
    else if (rot_state == STATE_SMUTE) {
      radio.setSoftMute(newPos & 0x01);
      encoderLastPos = newPos;
      DisplaySoftMute(newPos & 0x01);

    } // if
    encoderLastTime = now;
        
  }
  else if (now > encoderLastTime + 2000) {
    // fall into FREQ + RDS mode
    rot_state = STATE_FREQ;
    encoderLastPos = (radio.getFrequency() - radio.getMinFrequency()) / radio.getFrequencyStep();
    encoder.setPosition(encoderLastPos);

  } // if

  // check for RDS data
  radio.checkRDS();

  // update the display from time to time
  if (now > nextFreqTime) {
    f = radio.getFrequency();
    if (f != lastf) {
      // don't display a Service Name while frequency is no stable.
      DisplayServiceName("        ");
      DisplayFrequency(f);
      lastf = f;
    } // if
    nextFreqTime = now + 400;
  } // if  

  if (now > nextRadioInfoTime) {
    RADIO_INFO info;
    radio.getRadioInfo(&info);
    lcd.setCursor(14, 0);
    lcd.print(info.rssi);
    nextRadioInfoTime = now + 1000;
  } // update

} // loop

// End.
