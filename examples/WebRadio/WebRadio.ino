/// \file WebRadio.ino
/// \brief Radio implementation using a web frontend served by s Arduino Mega.
///
/// \author Matthias Hertel, http://www.mathertel.de
/// \copyright Copyright (c) 2014 by Matthias Hertel.\n
/// This work is licensed under a BSD style license.\n
/// See http://www.mathertel.de/License.aspx
///
/// \details
/// This is a full function radio implementation ...\n
/// The web site is stored on the SD card. You can find the web content I used in the web folder.\n
/// It can be used with various chips after adjusting the radio object definition.\n
///
/// Wiring
/// ------
/// The hardware setup for this sketch is:
/// Arduino MEGA board with Ethernet shield on top. Arduino UNO has not enough memory.
/// A 2*16 LCD Display in the I2C bus using a PCF8574 board with address 0x27.
/// A Rotary encoder with the data pins on A8 and A9.
/// A Rotary encoder press function on pin A10.
/// A button for scanning upwards on pin A11.

/// ChangeLog:
/// ----------
/// * 06.11.2014 created.
/// * 22.11.2014 working.
/// * 07.02.2015 more complete implementation
/// * 14.02.2015 works with no ethernet available (start radio first then Internet but slow).
/// * 17.04.2015 Return JSON format.
/// * 01.05.2015 faster WebServer responses by using a buffer.
/// * 16.05.2015 Using StringBuffer to collect output at several places for reducing net packages.
/// * 21.10.2017 Ethernet reset wait time
/// * 05.02.2021 Remove Compiler Warnings
///              Using Arduino Interrupt routine attachments 

// There are several tasks that have to be done when the radio is running.
// Therefore all these tasks are handled this way:
// * The setupNNN function has to initialize all the IO and variables to work.
// * The loopNNN function will be called as often as possible by passing the current (relative) time.


#include <SPI.h>
#include <Wire.h>

#include <Dhcp.h>
#include <Dns.h>
#include <Ethernet.h>

// Use the Arduino standard SD library to access files on the SD Disk
#include <SD.h>

// Include all the possible radio chips.
// #include <radio.h>
// #include <RDA5807M.h>
// #include <SI4703.h>
#include <SI4705.h>
// #include <TEA5767.h>

#include <RDSParser.h>

#include <LiquidCrystal_PCF8574.h>

#include <RotaryEncoder.h>
#include <OneButton.h>

#include "StringBuffer.h"

#define  ENCODER_FALLBACK (3*1000)  ///< after 3 seconds no turning fall back to tune mode.

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


// -----  Setup directly attached input stuff -----

/// Setup the RotaryEncoder

// RotaryEncoder encoder(A2, A3);
RotaryEncoder encoder(A9, A8);

int           encoderLastPos;  ///< last received encoder position
unsigned long encoderLastTime; ///< last time the encoder or encoder state was changed

/// Setup a Menu Button
OneButton menuButton(A10, true);

/// Setup a seek button
OneButton seekButton(A11, true);

char rdsServiceName[8 + 2]; ///< String with the actual RDS Service name. Some senders put rotating text in here too.
char rdsText[64 + 2];       ///< String with the actual RDS text.
char rdsTime[6];            ///< String with the actual time from RDS as hh:mm.

/// The radio object has to be defined by using the class corresponding to the used chip.
/// by uncommenting the right (one only) radio object definition.

// RADIO radio;    // Create an instance of a non functional radio.
// RDA5807M radio;    // Create an instance of a RDA5807 chip radio
// SI4703   radio;    // Create an instance of a SI4703 chip radio.
SI4705  radio;    // Create an instance of a SI4705 chip radio.
// TEA5767  radio;    // Create an instance of a TEA5767 chip radio.


/// The lcd object has to be defined by using a LCD library that supports the standard functions
/// When using a I2C->LCD library ??? the I2C bus can be used to control then radio chip and the lcd.

/// get a LCD instance
LiquidCrystal_PCF8574 lcd(0x27);  // set the LCD address to 0x27 for a 16 chars and 2 line display

unsigned long nextFreqTime = 0;      ///< next time for lcd update the frequency.
unsigned long nextRadioInfoTime = 0; ///< next time for lcd update the radio information

/// get a RDS parser
RDSParser rds;

/// State definition for this radio implementation.
enum RADIO_STATE {
  STATE_NONE = 0,
  STATE_PARSECOMMAND, ///< waiting for a new command character.
  STATE_PARSEINT,     ///< waiting for digits for the parameter.
  STATE_EXEC,         ///< executing the command.

  STATE_RDS,     ///< display RDS information
  STATE_VOL,
  STATE_PRESET,
  STATE_FREQ,
  STATE_MONO,
  STATE_SMUTE

};

RADIO_STATE state; ///< The state variable is used for parsing input characters.
RADIO_STATE rot_state;

// - - - - - - - - - - - - - - - - - - - - - - - - - -

boolean _debugEnabled = true; ///< enable / disable local debug output and reuse the radio macros.

#define DEBUGIP(l, n)   if (_debugEnabled) { Serial.print('>'); Serial.print(l); Ethernet.n().printTo(Serial); Serial.println(); }

/*
  Analyzed the Sources from
  Arduino Example WebServer
  http://www.ladyada.net/learn/arduino/ethfiles.html
  http://github.com/adafruit/SDWebBrowse
  and built a webServer

  http://en.wikipedia.org/wiki/Hypertext_Transfer_Protocol
  http://www.w3.org/Protocols/rfc2616/rfc2616.html
  http://www.jmarshall.com/easy/http/


  You can reach the web server on you local network with:
  http://WIZnetEFFEED
  http://WIZnetEFFEED/$list


  This sketch uses the microSD card slot on the Arduino Ethernet shield
  to serve up files over a very minimal browsing interface

  Some code is from Bill Greiman's SdFatLib examples, some is from the
  Arduino Ethernet WebServer example and the rest is from Limor Fried
  (Adafruit) so its probably under GPL

*/

#define NUL '\0'
#define CR '\r'
#define LF '\n'
#define SPACE ' '

#define REDIRECT_FNAME "/redirect.htm" ///< The filename on the disk that is used when requesting the root of the web.

// How big our line buffer should be. 80 is plenty!
#define LINE_BUFFERSIZE 256

enum WebServerState {
  WEBSERVER_OFF,    // not running
  WEBSERVER_IDLE,   // no current action
  PROCESS_GET,  // a GET request is pending
  PROCESS_PUT,  // a PUT request is pending
  PROCESS_POST, // a POST request is pending
  PROCESS_ERR,  // There was an error in processing
  PROCESS_STOP  // stop the socket after processing or timeout
}
__attribute__((packed));


/************ ETHERNET STUFF ************/
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; // Name will be WIZnetEFFEED
// byte ip[] = { 192, 168, 2, 250 };

// CompactList for file Extension -> content-type and cache-control
const char *CONTENTTYPES =
  "htm text/html 1\n"
  "css text/css 1\n"
  "js  application/x-javascript 1\n"
  "txt text/txt 0\n"
  "png image/png C\n"
  "jpg image/jpeg 1\n"
  "ico image/x-icon 1\n"
  "jsn application/json C"
  //"gif image/gif C\n"
  // ".3gp video/mpeg\n"
  // ".pdf application/pdf\n"
  // ".xml application/xml\n"
  ;

// The server instance listening at port 80.
EthernetServer server(80);

// The client instance for service the actual request.
// This is made global for speed and memory reasons.
EthernetClient _client;

WebServerState webstate;

char _readBuffer[LINE_BUFFERSIZE]; ///< a buffer that is used to read a line from the request header and for reading file content.
char _writeBuffer[LINE_BUFFERSIZE]; ///< a buffer that is used to compose a line for the reponse.

char _httpVerb[6]; // HTTP verb from the first line of the request
char _httpURI[40]; // HTTP URI from the first line of the request

int _httpContentLen;

// ----- http response texts -----

#define CRLF F("\r\n")

#define HTTP_200      F("HTTP/1.1 200 OK\r\n")
#define HTTP_CT       F("Content-Type: ")
#define HTTP_200_CT   F("HTTP/1.1 200 OK\r\nContent-Type: ")
#define HTTPERR_404   F("HTTP/1.1 404 Not Found\r\n")
#define HTTP_GENERAL  F("Server: Arduino\r\nConnection: close\r\n")
#define HTTP_NOCACHE  F("Cache-Control: no-cache\r\n")
#define HTTP_ENDHEAD  CRLF

// ----- html frame for generated response -----

#define HTML_OPEN  "<html><head></head><body>"
// <h2>File Not Found!</h2>
#define HTML_CLOSE F("</body></html>")

/// ----- forwards -----

void runRadioJSONCommand(char *cmd, int16_t value);
void runRadioSerialCommand(char cmd, int16_t value);

void setupRadio();
void loopRadio();
void respondRadioData();

void loopSerial();
void loopWebServer(unsigned long now);
void loopButtons(unsigned long now);


// ----- LCD output functions -----

/// Setup the LCD display.
void setupLCD() {
  // Initialize the lcd
  lcd.begin(16, 2);
  lcd.setBacklight(1);
  lcd.clear();
} // setupLCD()


/// Constantly check for new LCD data to be displayed.
void loopLCD(unsigned long now) {
  static RADIO_FREQ lastf = 0;
  RADIO_FREQ f = 0;

  // update the display of the frequency from time to time
  if (now > nextFreqTime) {
    f = radio.getFrequency();
    if (f != lastf) {
      // don't display a Service Name while frequency is no stable.
      DisplayServiceName("        ");
      DisplayFrequency();
      lastf = f;
      nextFreqTime = now + 400;
    } else {
      nextFreqTime = now + 1000;
    } // if
  } // if

  // update the display of the radio information from time to time
  if (now > nextRadioInfoTime) {
    RADIO_INFO info;
    radio.getRadioInfo(&info);
    lcd.setCursor(14, 0);
    lcd.print(info.rssi);
    nextRadioInfoTime = now + 8000;
  } // if

} // loopLCD()




/// ----- Web Server interface -----

/// This is a helper function that finds the entry for a given key in a table
/// that is formatted in multiple text lines separated by \n.
/// Each line starts with a key word and a SPACE character.
/// The pointer to the rest of the line is returned or null.
const char *_ctFind(const char *table, const char *key) {
  int keyLen = strlen(key);

  while ((table) && (*table)) {
    // check for the key is reached
    if (memcmp(table, key, keyLen) == 0) {
      if (*(table + keyLen) == SPACE) {
        // we found it: exit the while loop.
        break;
      }
    } // if

    // search next List entry.
    table = strchr(table, '\n');
    if (table) table++;
  } // while

  // return the position of the key.
  return (table);
} // _ctFind()


/// This is a helper function that returns the position to the next word on the same line.
/// This is done by skipping all non-SPACE characters and then all SPACE characters
/// and returns this position.
const char *_ctNextWord(const char *text)
{
  // skip all remaining characters of the word if word was too long.
  while ((text) && (*text) && (*text != SPACE)) text++;

  // skip trailing SPACE characters after word.
  while ((text) && (*text) && (*text == SPACE)) text++;
  return (text);
} // _ctNextWord()


// copy a word over to a string.
const char *_ctCopyWord(const char *text, char *word, int wordSize)
{
  while ((text) && (*text > SPACE) && (wordSize > 1)) {
    // copy over the printable characters
    *word++ = *text++;
    wordSize--;
  } // while
  *word = NUL;
  return (_ctNextWord(text));
} // _ctCopyWord()


// ----- Respond Header functions -----
// One call of these function is used to send back a valid reponse header with content type or error information.


// Response no and send not found html
void respond404NotFound()
{
  DEBUG_FUNC0("respond404NotFound");
  StringBuffer sout = StringBuffer(_writeBuffer, sizeof(_writeBuffer));
  sout.append(HTTPERR_404);
  sout.append(HTTP_GENERAL);
  sout.append(HTTP_ENDHEAD);
  _client.print(_writeBuffer);
} // respond404NotFound()


// Response no and send not found html
void respondEmptyFile()
{
  DEBUG_FUNC0("respondEmptyFile");
  StringBuffer sout = StringBuffer(_writeBuffer, sizeof(_writeBuffer));
  sout.append(HTTP_200_CT); sout.append("text/html"); sout.append(CRLF);
  sout.append(HTTP_GENERAL);
  sout.append("Content-Length: 0\r\n");
  sout.append(HTTP_ENDHEAD);
  _client.print(_writeBuffer);
} // respondEmptyFile()


// ----- Respond special data -----

// Response to a $list request.
// Send a list of all files on SD card to the client, wrapped in HTML
// The root.ls call needs a lot of stack space to complete so probably this will not work on Arduino Uno.
void respondFileList()
{
  StringBuffer sout = StringBuffer(_writeBuffer, sizeof(_writeBuffer));

  // send out a header
  sout.append(HTTP_200_CT); sout.append("text/html"); sout.append(CRLF);
  sout.append(HTTP_GENERAL);
  sout.append(HTTP_ENDHEAD);
  _client.print(_writeBuffer);

  sout.clear();
  sout.append(HTML_OPEN);
  sout.append("<h2>Files on SD:</h2>");
  sout.append("<pre>");

  // Recursive list of all directories
  File dir = SD.open("/");
  while (true) {
    File entry = dir.openNextFile();

    if (!entry) break; // no more files

    sout.append(entry.name());
    sout.append("\t");

    if (entry.isDirectory()) {
      sout.append("&lt;dir&gt;");
    } else {
      // files have sizes
      sout.append(entry.size());
    }
    sout.append("\r\n");

    // flush buffer out when filled.
    if (sout.getLength() > LINE_BUFFERSIZE - 32) {
      _client.print(_writeBuffer);
      sout.clear();
    }

    entry.close();
  } // while ()
  dir.close();

  sout.append("</pre>");
  sout.append(HTML_CLOSE);
  _client.print(_writeBuffer);

} // respondFileList()


// Response to a $info request.
// Send some system information to the client
void respondSystemInfo()
{
  StringBuffer sout = StringBuffer(_writeBuffer, sizeof(_writeBuffer));
  sout.append(HTTP_200_CT); sout.append("text/html"); sout.append(CRLF);
  sout.append(HTTP_GENERAL);
  sout.append(HTTP_ENDHEAD);

  sout.append(HTML_OPEN);
  sout.append("<pre>");
  sout.append("Free RAM: ");  sout.append(FreeRam());
  sout.append("</pre>");
  sout.append(HTML_CLOSE);

  _client.print(_writeBuffer);
} // respondSystemInfo()


/// Read one line of text from _client into the _readBuffer.
void readRequestLine()
{
  uint16_t index = 0;
  int c;

  while (_client.connected()) {
    c = _client.read();

    if (c < 0) {
      // no more data available
      // DEBUG_STR("No more data.");
      break;

    } else if (c == CR) {
      // ignore this character, because LF will follow

    } else if (c == LF) {
      // line is complete
      break;

    } else {
      // add the character to the buffer
      _readBuffer[index++] = c;

      // When lines are too long, just ignore the last characters.
      // If that happens in your application ther might be a header line that is out of interest
      // or use a bigger line buffer by incrementing LINE_BUFFERSIZE.
      if (index >= LINE_BUFFERSIZE) index = LINE_BUFFERSIZE - 1;

    } // if
  } // while
  _readBuffer[index] = NUL;
} // readRequestLine()


/// Responds the content of a file from the SD disk given by fName.
void respondFileContent(const char *fName)
{
  DEBUG_FUNC0("respondFileContent");
  StringBuffer sout = StringBuffer(_writeBuffer, sizeof(_writeBuffer));

  const char *p;
  const char *fileType = NULL;
  size_t len;

  // check for fileType
  p = strrchr(fName, '.');
  if (p != NULL) fileType = p + 1;

  File f = SD.open(fName, O_READ);
  if (! f) {
    respond404NotFound();

  } else {
    // respond the content type
    p = _ctFind(CONTENTTYPES, fileType);
    if (p) {
      char ct[32];
      p = _ctNextWord(p);
      p = _ctCopyWord(p, ct, sizeof(ct));
      sout.append(HTTP_200_CT); sout.append(ct); sout.append(CRLF);
      sout.append(HTTP_GENERAL);
    }

    if (p) {
      sout.append("Cache-Control: ");
      if (*p == '1') {
        sout.append("no-cache\r\n");
      } else if (*p == 'C') {
        sout.append("max-age=600, public\r\n");
      } else {
        sout.append("private\r\n");
      }
    } // if

    // respond the number of file bytes.
    sout.append("Content-Length: "); sout.append(f.size()); sout.append(CRLF);

    // end of headers: respond Blank Line.
    sout.append(HTTP_ENDHEAD);

    // and send out buffer.
    _client.print(_writeBuffer);

    // using a buffer is up to 10 times faster than transferring byte by byte
    // using the _readBuffer
    do {
      len = f.read((uint8_t*)_readBuffer, LINE_BUFFERSIZE);
      _client.write((uint8_t*)_readBuffer, len);
    } while (len > 0);
    f.close();
  } // if

} // respondFileContent()


/// This is the main webserver routine.
/// Constantly look for incomming webserver requests and answer them...
void loopWebServer(unsigned long now) {
  static unsigned long timeout;

  // static bool moreData = false;
  const char *p;

  File f;

  if (webstate != WEBSERVER_OFF) {
    // Find a socket that has data available and return a client for this port.
    // In the EthernetServer actually there is always a client with the port == MAX_SOCK_NUM returned but it has no data.
    _client = server.available();

    if (_client) {
      // Answer this webserver request until done and then close the port.
      while (_client.connected()) {
        if (_client.available()) {
          // Data is available...

          if (webstate == WEBSERVER_IDLE) {
            // Got a new request from a new client.
            // Read a http header and parse all information.

            // read the first line into the line buffer requestLine
            readRequestLine();

            // The requestLine consists of the method, the Request-URI and the HTTP-Version, all separated by SPACE characters.
            // The SPACE chars are substituded by NUL chars and pointers are set to their part of the requestLine.
            p = _readBuffer;
            p = _ctCopyWord(p, _httpVerb, sizeof(_httpVerb)); // extract the verb
            p = _ctCopyWord(p, _httpURI, sizeof(_httpURI)); // extract the URI
            // we don't need the rest of the first line

            // read following lines extracting some data (if there)
            _httpContentLen = 0;
            do {
              readRequestLine();
              if (memcmp(_readBuffer, "Content-Length: ", 16) == 0) {
                _httpContentLen = atoi(_readBuffer + 16);
              } // if
            } while (_readBuffer[0] != NUL);

            if (strcmp(_httpVerb, "GET") == 0) {
              webstate = PROCESS_GET;
            } else if (strcmp(_httpVerb, "PUT") == 0) {
              webstate = PROCESS_PUT;
            } else if (strcmp(_httpVerb, "POST") == 0) {
              webstate = PROCESS_POST;
            } else {
              webstate = PROCESS_ERR;
            } // if

            // convert _httpURI to lowercase for we need no case here !
            strlwr(_httpURI);

            timeout = now + 1200;
          } // webstate == idle


          if (now > timeout) {
            webstate = PROCESS_STOP;

          } else if (webstate == PROCESS_GET) {
            // ----- WebServer utitily functions - comment out if you don't need them

            if (strcmp(_httpURI, "/$list") == 0) {
              // List all files on the SD card
              respondFileList();

            } else if (strcmp(_httpURI, "/$info") == 0) {
              // give some system information back.
              respondSystemInfo();

            } else if (strcmp(_httpURI, "/$radio") == 0) {
              // respond the current radio data.
              respondRadioData();

            } else if (memcmp(_httpURI, "/", 1 + 1) == 0) {
              // The root of the web server is requested, but this is not a file.
              // So redirect if no file path was given.
              // Because this is a complicated task there is a special file "redirect.htm" that contains all the stuff needed for this.
              respondFileContent(REDIRECT_FNAME);

            } else {
              // ----- Respond the content of a file -----
              respondFileContent(_httpURI);
            } // if

            // GET requests will never have a content so its all done.
            webstate = PROCESS_STOP;

          } else if (webstate == PROCESS_POST) {
            // get data posted by a html form

            uint16_t len;
            len = _client.read((uint8_t *)_readBuffer, LINE_BUFFERSIZE);

            if ((len > 0) && (len < sizeof(_readBuffer))) {
              _httpContentLen -= len;
              _readBuffer[len] = NUL; // _readBuffer[32] = NUL;
            }

            if (_httpContentLen == 0) {
              respondEmptyFile();
              webstate = PROCESS_STOP;
            } // if

            if (strcmp(_httpURI, "/$radio") == 0) {
              const char *name = NULL; // name of command

              // simple parsing of the JSON request.
              // assume only one command like {"vol":6}
              // DEBUG_STR(_readBuffer);
              char *p = strchr(_readBuffer, '{');
              if (p) p = strchr(p, '"');
              if (p) p += 1;
              if (p) {
                name = p;
                p = strchr(p, '"');
              }
              if (p) {
                *p++ = NUL;
                p = strchr(p, ':');
              }
              if (p) {
                p += 1;
                runRadioJSONCommand(name, atoi(p));
              } // if

            } // if

          } else if (webstate == PROCESS_PUT) {
            // upload a file
            int len;
            DEBUG_VAL("Upload...", _httpURI);

            len = _client.read((uint8_t *)_readBuffer, 32);
            if (len > 0) {
              _readBuffer[len] = NUL; _readBuffer[32] = NUL;

              if (!f) {
                f = SD.open(_httpURI, O_CREAT | O_WRITE | O_TRUNC);
                if (!f) {
                  DEBUG_STR("no OPEN");
                  webstate = PROCESS_ERR;
                } // if
              }  // if
              f.write((uint8_t *)_readBuffer, len);
              DEBUG_VAL("len", len);
              _httpContentLen -= len;
            } // if
            // DEBUG_STR(_httpContentLen);

            if (_httpContentLen == 0) {
              f.close();
              webstate = PROCESS_STOP;
            }
          } // PROCESS_PUT

          if (webstate == PROCESS_ERR) {
            // everything else is a 404
            respond404NotFound();
            webstate = PROCESS_STOP;
          } // if

        } // if

        if (webstate == PROCESS_STOP) {
          // DEBUG_STR("PROCESS_STOP");
          _client.stop();
          webstate = WEBSERVER_IDLE;
        } // if

        // give the web browser time to receive the data
        delay(1);
      } // while

    } // if

    Ethernet.maintain();
  }

} // loopWebServer()


/// ----- LCD functions -----

/// Update the Frequency on the LCD display.
void DisplayFrequency()
{
  char s[12];
  radio.formatFrequency(s, sizeof(s));
  lcd.setCursor(0, 0);
  lcd.print(s);
  DEBUG_VAL("FREQ", s);
} // DisplayFrequency()


/// This function will be called by the RDS module when a rds service name was received.
/// The text be displayed on the LCD and written to the serial port
/// and will be stored for the web interface.
void DisplayServiceName(const char *name)
{
  DEBUG_VAL("RDS", name);
  strncpy(rdsServiceName, name, sizeof(rdsServiceName));

  if (rot_state == STATE_RDS) {
    lcd.setCursor(0, 1);
    lcd.print(name);
  }
} // DisplayServiceName()


/// This function will be called by the RDS module when a rds text message was received.
/// The text will not displayed on the LCD but written to the serial port
/// and will be stored for the web interface.
void DisplayText(const char *text)
{
  DEBUG_VAL("RDS-text", text);
  strncpy(rdsText, text, sizeof(rdsText));
} // DisplayText()


/// This function will be called by the RDS module when a rds time message was received.
/// The time will not displayed on the LCD but written to the serial port.
void DisplayTime(uint8_t hour, uint8_t minute) {
  rdsTime[0] = '0' + (hour / 10);
  rdsTime[1] = '0' + (hour % 10);
  rdsTime[2] = ':';
  rdsTime[3] = '0' + (minute / 10);
  rdsTime[4] = '0' + (minute % 10);
  rdsTime[5] = NUL;
  DEBUG_VAL("RDS-time", rdsTime);
} // DisplayTime()


/// Display the current volume.
/// This function is called after a new volume level is choosen.
/// The new volume is displayed on the LCD 2. Line.
void DisplayVolume(uint8_t v)
{
  DEBUG_FUNC1("DisplayVolume", v);
  lcd.setCursor(0, 1);
  lcd.print("VOL: ");  lcd.print(v);
  lcd.print("     ");
} // DisplayVolume()


/// Display the current mono switch.
void DisplayMono(uint8_t v)
{
  DEBUG_FUNC1("DisplayMono", v);
  lcd.setCursor(0, 1);
  lcd.print("MONO: "); lcd.print(v);
} // DisplayMono()


/// Display the current soft mute switch.
void DisplaySoftMute(uint8_t v)
{
  DEBUG_FUNC1("DisplaySoftMute", v);
  lcd.setCursor(0, 1);
  lcd.print("SMUTE: "); lcd.print(v);
} // DisplaySoftMute()


// ----- Radio data functions -----

/// Response to a $info request and return all information of the current radio operation.
/// Format al data as in JSON Format.\n
void respondRadioData()
{
  // DEBUG_FUNC0("respondRadioData");
  StringBuffer sout = StringBuffer(_writeBuffer, sizeof(_writeBuffer));

  // build http header in _writeBuffer and send out.
  sout.append(HTTP_200);
  sout.append(HTTP_CT); sout.append("application/json"); sout.append(CRLF);
  sout.append(HTTP_GENERAL);
  sout.append(HTTP_NOCACHE);
  sout.append(HTTP_ENDHEAD);
  _client.print(_writeBuffer);

  // JSON Data
  sout.clear();
  sout.append('{');

  // return frequency
  sout.appendJSON("freq", (int)(radio.getFrequency())); sout.append(',');
  sout.appendJSON("band", (int)(radio.getBand()));  sout.append(',');

  // return radio related features
  RADIO_INFO ri;
  radio.getRadioInfo(&ri);
  sout.appendJSON("mono", ri.mono);  sout.append(',');
  sout.appendJSON("stereo", ri.stereo); sout.append(',');
  // respondJSONObject("rds", ri.rds); sout.append(',');      // has rds signal

  // return rds information
  sout.appendJSON("servicename", rdsServiceName); sout.append(',');
  sout.appendJSON("rdstext", rdsText); sout.append(',');

  // return audio related features
  AUDIO_INFO ai;
  radio.getAudioInfo(&ai);

  sout.appendJSON("vol", ai.volume); sout.append(',');
  sout.appendJSON("mute", ai.mute); sout.append(',');
  sout.appendJSON("softmute", ai.softmute); sout.append(',');
  sout.appendJSON("bassboost", ai.bassBoost);

  sout.append('}');
  _client.print(_writeBuffer);
} // respondRadioData()

// - - - - - - - - - - - - - - - - - - - - - - - - - -

/// retrieve RDS data from the radio chip and forward to the RDS decoder library
void RDS_process(uint16_t block1, uint16_t block2, uint16_t block3, uint16_t block4) {
  static uint16_t b1;

  if (b1 != block1)
    Serial.println(block1);
  b1 = block1;
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

  } else if (rot_state == STATE_VOL) {
    // jump into mono/stereo switch
    rot_state = STATE_MONO;
    encoderLastPos = radio.getMono();
    encoder.setPosition(encoderLastPos);
    DisplayMono(encoderLastPos);

  } else if (rot_state == STATE_MONO) {
    // jump into soft mute switch
    rot_state = STATE_SMUTE;
    encoderLastPos = radio.getSoftMute();
    encoder.setPosition(encoderLastPos);
    DisplaySoftMute(encoderLastPos);

  } else if (rot_state == STATE_SMUTE) {
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

void checkPosition() {
  encoder.tick(); // just call tick() to check the state.
}

// ----- Setup and run the radio -----

/// Setup a FM only radio configuration with I/O for commands and debugging on the Serial port.
void setupRadio() {
  // Enable information to the Serial port
  radio.debugEnable();

  // Initialize the Radio
  radio.init();

  // radio.setBandFrequency(RADIO_BAND_FM, 8930); // hr3
  radio.setBandFrequency(RADIO_BAND_FM, preset[presetIndex]); // default preset.

  radio.setMono(false);
  radio.setMute(false);
  // radio._wireDebug();
  radio.setVolume(8);

  // Setup rotary encoder
  // Any change in signals on pins A8 and A9 are captured in the ISR routine `checkPosition`
  attachInterrupt(digitalPinToInterrupt(A8), checkPosition, CHANGE);
  attachInterrupt(digitalPinToInterrupt(A9), checkPosition, CHANGE);

  encoderLastPos = (radio.getFrequency() - radio.getMinFrequency()) / radio.getFrequencyStep();
  encoder.setPosition(encoderLastPos);

  // Setup the buttons
  // link the doMenuClick function to be called on a click event.
  menuButton.attachClick(doMenuClick);

  // link the doSeekClick function to be called on a click event.
  seekButton.attachClick(doSeekClick);

  state = STATE_PARSECOMMAND;
  rot_state = STATE_NONE; // the loop function will enter RDS mode immediately.

  // setup the information chain for RDS data.
  *rdsServiceName = NUL;
  /// retrieve RDS data from the radio chip and forward to the RDS decoder library
  radio.attachReceiveRDS(RDS_process); // or (rds.processData)

  rds.attachServiceNameCallback(DisplayServiceName);
  rds.attachTextCallback(DisplayText);
  rds.attachTimeCallback(DisplayTime);

} // setupRadio


/// Check once new radio data.
void loopRadio() {
  // check for RDS data
  radio.checkRDS();
} // loopRadio()


/// Execute a command identified by a character and an optional number.
/// See the "?" command for available commands.
/// \param cmd The command word.
/// \param value An optional parameter for the command.
void runRadioJSONCommand(const char *cmd, int16_t value) {
  if (strcmp(cmd, "freq") == 0) {
    radio.setFrequency(value);
  } else if (strcmp(cmd, "vol") == 0) {
    radio.setVolume(value);
  } else if (strcmp(cmd, "mute") == 0) {
    radio.setMute(value > 0);
  } else if (strcmp(cmd, "mono") == 0) {
    radio.setMono(value > 0);
  } else if (strcmp(cmd, "softmute") == 0) {
    radio.setSoftMute(value > 0);
  } else if (strcmp(cmd, "bassboost") == 0) {
    radio.setBassBoost(value > 0);

  } else if (strcmp(cmd, "seekup") == 0) {
    radio.seekUp(true);

  } // if
} // runRadioJSONCommand()


// ----- Check for commands on the Serial interface -----

/// setupSerial


/// Constantly check for serial input commands and trigger command execution.
void loopSerial() {
  char c;
  // some internal static values for parsing the input
  static char command;
  static int16_t value;

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
        runRadioSerialCommand(command, value);
        command = ' ';
        state = STATE_PARSECOMMAND;
        value = 0;
      } // if
    } // if
  } // if

} // loopSerial


/// interpret a command for the radio chip used on the Serial input.
/// It's a single char command and an optional numeric value.
/// See help output on the '?' command for syntax.
void runRadioSerialCommand(char cmd, int16_t value)
{
  if (cmd == '?') {
    Serial.println("? Help");
    Serial.println("fnnnnn: direct frequency input (n: freq*100)");
    Serial.println("vnn: direct volume input (n: 0...15)");
    Serial.println("mn: direct mute mode (n: 0|1)");

    Serial.println("> next preset");
    Serial.println("< previous preset");
    Serial.println(". scan up   : scan up to next sender");
    Serial.println(", scan down ; scan down to next sender");
    Serial.println("i station status");
    Serial.println("s mono/stereo mode");
    Serial.println("b bass boost");
    Serial.println("m mute/unmute");
    Serial.println("u soft mute/unmute");
  } // runRadioSerialCommand()

  // ----- control the volume and audio output -----

  else if (cmd == 'u') {
    // toggle soft mute mode
    radio.setSoftMute(!radio.getSoftMute());
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
      presetIndex++; radio.setFrequency(preset[presetIndex]);
    } // if
  } else if (cmd == '<') {
    // previous preset
    if (presetIndex > 0) {
      presetIndex--;
      radio.setFrequency(preset[presetIndex]);
    } // if

  } else if (cmd == 'f') {
    radio.setFrequency(value);
  } else if (cmd == 'v') {
    radio.setVolume(value);
  } else if (cmd == 'm') {
    radio.setMute(value > 0);
    delay(4000);
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
    DEBUG_VAL("Station", s);
    Serial.print("Radio:"); radio.debugRadioInfo();
    Serial.print("Audio:"); radio.debugAudioInfo();

    RADIO_INFO ri;
    radio.getRadioInfo(&ri);
  } // info

  //  else if (cmd == 'n') { radio.debugScan(); }
  else if (cmd == 'x') {
    radio.debugStatus();
  }


} // runRadioSerialCommand()


/// ----- Rotary encoder and buttons -----

/// Constantly check for changed signals on the buttons and rotary encoder.
void loopButtons(unsigned long now) {
  int newPos;

  // check for the menuButton and seekButton
  menuButton.tick();
  seekButton.tick();

  // check for the rotary encoder
  newPos = encoder.getPosition();
  if (newPos != encoderLastPos) {
    if (rot_state == STATE_RDS) {
      // modify volume instead
      rot_state = STATE_VOL;
    } // if

    if (rot_state == STATE_FREQ) {
      RADIO_FREQ f = radio.getMinFrequency() + (newPos *  radio.getFrequencyStep());
      radio.setFrequency(f);
      encoderLastPos = newPos;
      nextFreqTime = now + 10;

    } else if (rot_state == STATE_VOL) {
      // accept new volume level.
      if (newPos < 0) newPos = 0;
      if (newPos > radio.MAXVOLUME) newPos = radio.MAXVOLUME;
      radio.setVolume(newPos);
      DisplayVolume(newPos);
      encoder.setPosition(newPos);
      encoderLastPos = newPos;

    } else if (rot_state == STATE_MONO) {
      radio.setMono(newPos & 0x01);
      encoderLastPos = newPos;
      DisplayMono(newPos & 0x01);

    } else if (rot_state == STATE_SMUTE) {
      radio.setSoftMute(newPos & 0x01);
      encoderLastPos = newPos;
      DisplaySoftMute(newPos & 0x01);

    } // if
    encoderLastTime = now;

  } else if (rot_state != STATE_RDS)  {
    if (now > encoderLastTime + ENCODER_FALLBACK) {
      // fall into FREQ + RDS mode
      rot_state = STATE_RDS;
      // set rotary encoder to volume.
      encoderLastPos = radio.getVolume();
      encoder.setPosition(encoderLastPos);
      encoderLastTime = now;
    } // if

  } // if

} // loopButtons


void dumpCard() {
  DEBUG_FUNC0("dumpCard");

  // Recursive list of all directories
  File dir = SD.open("/");
  while (true) {
    File entry = dir.openNextFile();

    if (!entry) break; // no more files

    Serial.print(entry.name());
    Serial.print(" - ");
    Serial.println(entry.size());
    entry.close();
  } // while ()
  dir.close();
} // dumpCard()                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         ;

/// ----- Main functions setup() and loop() -----

void setup() {
  delay(250);  // give the Ethernet some time to reset properly

  // setup all components
  Serial.begin(57600);

  // DEBUG_STR(F("Initializing LCD..."));
  setupLCD();

  // DEBUG_STR(F("Initializing Radio..."));
  lcd.print("Radio...");
  setupRadio();

  // Initialize the SD card.
  // see if the card is present and can be initialized:
  lcd.clear();
  lcd.print("Card...");

  SD.begin(4);
  if (_debugEnabled)
    dumpCard();

  // Initialize web server.
  webstate = WEBSERVER_OFF;

  // Let's start the network
  lcd.clear();
  lcd.print("Net...");

  int result = Ethernet.begin(mac);  // Ethernet.begin(mac, ip); if there is no DHCP

  DEBUG_VAL(F("Ethernet Result"), result);
  DEBUG_STR(F("Configuration:"));

  Serial.print(F("> localIP:"));  Ethernet.localIP().printTo(Serial); Serial.println();
  Serial.print(F("> subnetMask:"));  Ethernet.subnetMask().printTo(Serial); Serial.println();
  Serial.print(F("> dnsServerIP:"));  Ethernet.dnsServerIP().printTo(Serial); Serial.println();
  Serial.print(F("> gatewayIP:"));  Ethernet.gatewayIP().printTo(Serial); Serial.println();

  if (result) {
    // Let's start the server
    DEBUG_STR(F("Starting the server..."));
    server.begin();
    webstate = WEBSERVER_IDLE;
  }
  DEBUG_VAL(F("Free RAM"), FreeRam());

  lcd.clear();
} // setup()


/// Constantly look for the things, that have to be done.
void loop()
{
  unsigned long now = millis();
  loopWebServer(now);  /// Look for incoming webserver requests and answer them...
  loopButtons(now);    /// Check for changed signals on the buttons and rotary encoder.
  loopSerial();     /// Check for serial input commands and trigger command execution.
  loopRadio();      /// Check for new radio data.
  loopLCD(now);        /// Check for new LCD data to be displayed.
} // loop()


// End.

