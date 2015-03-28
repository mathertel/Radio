/// WebRadio.ino

/// History:
/// --------
/// * 06.11.2014 created.
/// * 22.11.2014 working.
/// * 07.02.2015 more complete implementation
//  * 14.02.2015 still works with no ethernet available (slow start).

// There are several tasks that have to be done when the radio is running.
// Therefore all these tasks are handled this way:
// * The setupNNN function has to initialize all the IO and variables to work.
// * The loopNNN function will be called as often as possible by passing the current (relative) time.

 
#include <SPI.h>
#include <Wire.h>

#include <Dhcp.h>
#include <Dns.h>
#include <Ethernet.h>
// #include <EthernetClient.h>
// #include <EthernetServer.h>
// #include <EthernetUdp.h>
// #include <util.h>

// Use the Arduino standard SD library to access files on the SD Disk
#include <SD.h>

// Include all the possible radio chips.
#include <RADIO.h>
#include <RDA5807M.h>
#include <SI4703.h>
#include <SI4705.h>
#include <TEA5767.h>

#include <RDSParser.h>

#include <LiquidCrystal_PCF8574.h>

#include <RotaryEncoder.h>
#include <OneButton.h>

#define  ENCODER_FALLBACK (3*1000)  ///< after 3 seconds no turning fall back to tune mode.


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

int    i_sidx = 5;        // Start at Station with index=5
int    i_smax = 14;       // Max Index of Stations

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

char rdsServiceName[10];
char rdsText[64 + 2];

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

/// next time for lcd update the frequency
unsigned long nextFreqTime = 0;

/// next time for lcd update the radio information
unsigned long nextRadioInfoTime = 0;


/// get a RDS parser
RDSParser rds;


/// State definition for this radio implementation.
enum RADIO_STATE {
  STATE_NONE = 0,
  STATE_PARSECOMMAND, ///< waiting for a new command character.
  STATE_PARSEINT,     ///< waiting for digits for the parameter.
  STATE_EXEC,         ///< executing the command.

  STATE_FREQ,
  STATE_VOL,
  STATE_MONO,
  STATE_SMUTE

};

RADIO_STATE state; ///< The state variable is used for parsing input characters.
RADIO_STATE rot_state;

void respondRadioData();

// - - - - - - - - - - - - - - - - - - - - - - - - - -

#define DEBUGTEXT(txt)  { Serial.print('>'); Serial.println(txt); }
#define DEBUGFUNC0(fn)  { Serial.print('>'); Serial.print(fn); Serial.println("()"); }
#define DEBUGFUNC1(fn, v)  { Serial.print('>'); Serial.print(fn); Serial.print('('); Serial.print(v); Serial.println(')');}
#define DEBUGVAR(l, v)  { Serial.print('>'); Serial.print(l); Serial.print(": "); Serial.println(v); }
#define DEBUGIP(l, n)   { Serial.print('>'); Serial.print(l); Ethernet.n().printTo(Serial); Serial.println(); }

/*
Analyzed the Sources from
* Arduino Example WebServer
* http://www.ladyada.net/learn/arduino/ethfiles.html
* http://github.com/adafruit/SDWebBrowse
and built a webServer

http://en.wikipedia.org/wiki/Hypertext_Transfer_Protocol
http://www.w3.org/Protocols/rfc2616/rfc2616.html
http://www.jmarshall.com/easy/http/


You can reach the web server on you local network with:
http://WIZnetEFFEED
http://WIZnetEFFEED/$list


* This sketch uses the microSD card slot on the Arduino Ethernet shield
* to serve up files over a very minimal browsing interface
*
* Some code is from Bill Greiman's SdFatLib examples, some is from the
* Arduino Ethernet WebServer example and the rest is from Limor Fried
* (Adafruit) so its probably under GPL
*
*/

#define NUL '\0'
#define CR '\r'
#define LF '\n'
#define SPACE ' '

// How big our line buffer should be. 80 is plenty!
#define BUFSIZ 80

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
char *CONTENTTYPES =
"htm text/html 1\n"
"css text/css 1\n"
"js  application/x-javascript 1\n"
"txt text/txt 0\n"
"png image/png C\n"
"gif image/gif C\n"
"jpg image/jpeg 1\n"
"ico image/x-icon 1"
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

char _readBuffer[BUFSIZ]; // a buffer that is used to read a line from the request header.

char _httpVerb[6]; // HTTP verb from the first line of the request
char _httpURI[40]; // HTTP URI from the first line of the request

char *_fileType;
char _httpContentType[28];

int _httpContentLen;


/************ SDCARD STUFF ************/
SdFile   root;

// error nmumbers and texts
#define ERR_CARDINIT 101 // card.init failed!
#define ERR_VOLINIT  102 // vol.init failed!
#define ERR_OPENROOT 103 // openRoot failed!

// ----- http response texts -----

#define HTTP_200_CT   F("HTTP/1.1 200 OK\r\nContent-Type: ")
#define HTTPERR_404   F("HTTP/1.1 404 Not Found\nContent-Type: text/html\r\n")
#define HTTP_GENERAL  F("Server: Arduino\r\nConnection: close\r\n")
#define HTTP_ENDHEAD  F("\r\n")

// ----- html frame for generated response -----

#define HTML_OPEN  F("<html><head></head><body>")
// <h2>File Not Found!</h2>
#define HTML_CLOSE F("</body></html>")


/// ----- Main functions setup() and loop() -----

void setup() {
  Serial.begin(57600);
  // DEBUGTEXT(F("Initializing LCD..."));
  setupLCD();

  // DEBUGTEXT(F("Initializing Radio..."));
  lcd.print("Radio...");
  setupRadio();

  // Initialize the SD card.
  // see if the card is present and can be initialized:
  // DEBUGTEXT(F("Initializing SD card..."));
  SD.begin(4);

  delay(2500);

  webstate = WEBSERVER_OFF;

  // Let's start the network
  lcd.print("Net...");
  // DEBUGTEXT(F("Starting network..."));

  int result = Ethernet.begin(mac);  // Ethernet.begin(mac, ip); if there is no DHCP

  DEBUGVAR(F("Ethernet Result"), result);
  DEBUGTEXT(F("Configuration:"));
  DEBUGIP(F(" localIP: "), localIP);
  DEBUGIP(F(" subnetMask: "), subnetMask);
  DEBUGIP(F(" dnsServerIP: "), dnsServerIP);
  DEBUGIP(F(" gatewayIP: "), gatewayIP);

  if (result) {
    // Let's start the server
    DEBUGTEXT(F("Starting the server..."));
    server.begin();
    webstate = WEBSERVER_IDLE;
  }
  DEBUGVAR(F("Free RAM"), FreeRam());

  lcd.clear();
} // setup()


// constantly look for the things, that have to be done.
void loop()
{
  unsigned long now = millis();

  loopWebServer(now);
  loopButtons(now);
  loopSerial(now);
  loopRadio(now);
  loopLCD(now);
} // loop()



/// ----- Web Server interface -----

/// This is a helper function that finds the entry for a given key in a table
/// that is formatted in multiple text lines separated by \n.
/// Each line starts with a key word and a SPACE character.
/// The pointer to the rest of the line is returned or null.
char *_ctFind(char *table, char *key) {
  char *p = table;
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
char *_ctNextWord(char *text)
{
  // skip all remaining characters of the word if word was too long.
  while ((text) && (*text) && (*text != SPACE)) text++;

  // skip trailing SPACE characters after word.
  while ((text) && (*text) && (*text == SPACE)) text++;
  return (text);
} // _ctNextWord()


// copy a word over to a string.
char *_ctCopyWord(char *text, char *word, int len)
{
  while ((text) && (*text > SPACE) && (len > 1)) {
    // copy over the printable characters
    *word++ = *text++;
    len--;
  } // while
  *word = NUL;
  return (_ctNextWord(text));
} // _ctCopyWord()


// ----- Respond Header functions -----
// One call of these function is used to send back a valid reponse header with content type or error information.

// Response ok and send the content type.
void respondHeaderContentType(char *type)
{
  // DEBUGFUNC0("respondHeaderContentType");
  _client.print(HTTP_200_CT); _client.print(type); _client.print("\r\n");
  _client.print(HTTP_GENERAL);
} // respondHeaderContentType()


// Response no and send not found html
void respondHeaderNotFound()
{
  // DEBUGFUNC0("respondHeaderNotFound");
  _client.print(HTTPERR_404);
  _client.print(HTTP_GENERAL);
  _client.print(HTTP_ENDHEAD);
  _client.print(HTML_OPEN);
  _client.print("<h2>File Not Found!</h2>");
  _client.print(HTML_CLOSE);
} // respondHeaderNotFound()


// ----- Respond special data -----

// Response to a $list request.
// Send a list of all files on SD card to the client, wrapped in HTML
// The root.ls call needs a lot of stack space to complete so probably this will not work on Arduino Uno.
void respondFileList()
{
  respondHeaderContentType("text/html");
  _client.print(HTTP_ENDHEAD);

  _client.print(HTML_OPEN);
  _client.print(F("<h2>Files on SD:</h2>"));
  _client.write("<pre>");
  // Recursive list of all directories
  File root = SD.open("\\");
  // root.ls
  // root.ls((Print *)&_client, LS_R | LS_DATE | LS_SIZE);
  // todo: implement ls()
  _client.print("</pre>");
  _client.print(HTML_CLOSE);
} // respondFileList()


// Response to a $info request.
// Send some system information to the client
void respondSystemInfo()
{
  respondHeaderContentType("text/html");
  _client.print(HTTP_ENDHEAD);

  _client.print(HTML_OPEN);
  _client.print("<pre>");
  _client.print("Free RAM: ");    _client.println(FreeRam());
  _client.print("</pre>");
  _client.print(HTML_CLOSE);
} // respondSystemInfo()


// read a text line from _client into the requestLine buffer.
void readRequestLine()
{
  uint8_t index = 0;
  int c;

  while (_client.connected()) {
    c = _client.read();

    if (c < 0) {
      // no more data available
      // DEBUGTEXT("No more data.");
      break;

    } else if (c == CR) {
      // ignore this character, because LF will follow

    } else if (c == LF) {
      // line is complete
      break;

    } else {
      // add the character to the buffer
      _readBuffer[index++] = c;

      // start tossing out data when buffer is full.
      if (index >= BUFSIZ) index = BUFSIZ - 1;

    } // if
  } // while
  _readBuffer[index] = NUL;

  // if (_client.connected()) DEBUGTEXT("still connected...");
  // DEBUGTEXT(_readBuffer);
} // readRequestLine()


// Serve a static file from the SD disk.
// Responds the content of a file given by _httpURI.
void respondFileContent()
{
  char *p;

  _fileType = NULL;

  // check for fileType
  p = strrchr(_httpURI, '.');
  if (p != NULL) _fileType = p + 1;

  Serial.print("Serve:"); Serial.println(_httpURI);

  File f = SD.open(_httpURI, O_READ);
  if (!f) {
    respondHeaderNotFound();

  } else {
    // respond the content type
    p = _ctFind(CONTENTTYPES, _fileType);
    if (p) {
      p = _ctNextWord(p);
      p = _ctCopyWord(p, _httpContentType, sizeof(_httpContentType));
      respondHeaderContentType(_httpContentType);
    }

    if (p) {
      _client.print("Cache-Control: ");
      if (*p == '1') {
        _client.print("no-cache\r\n");
      } else if (*p == 'C') {
        _client.print("max-age=600, public\r\n");
      } else {
        _client.print("private\r\n");
      }
    } // if

    // respond the number of file bytes.
    _client.print("Content-Length: "); _client.print(f.size()); _client.print("\r\n");

    // end of headers: respond Blank Line.
    _client.print(HTTP_ENDHEAD);

    // using a buffer is up to 10 times faster than transferring byte by byte
    // using the _readBuffer
    size_t len;
    do {
      len = f.readBytes((uint8_t*)_readBuffer, 32);
      _client.write((uint8_t*)_readBuffer, len);
    } while (len > 0);
    f.close();
  } // if
} // respondFileContent()


// constantly look for incomming webserver requests and answer them...
void loopWebServer(unsigned long now) {
  static unsigned long timeout;

  // static bool moreData = false;
  int index = 0;
  char c;
  char *p, *t;

  File f;

  if (webstate != WEBSERVER_OFF) {
    _client = server.available();

    if (_client) {
      // DEBUGTEXT('>');
      while (_client.connected()) {
        if (_client.available()) {

          if (webstate == WEBSERVER_IDLE) {
            // got some new data from a new client
            // read a http header and parse all information.

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
              // redirect if no file path was given.
              // Because this is a complicated task there is a special file "redirect.htm" that contains all the stuff needed for this.
              strcpy(_httpURI, "/redirect.htm");
              respondFileContent();

            } else {
              // ----- Respond the content of a file -----
              respondFileContent();
            } // if

            // GET requests will never have a content so its all done.
            webstate = PROCESS_STOP;

          } else if (webstate == PROCESS_POST) {
            // get data posted by a html form

            // DEBUGVAR("uri", _httpURI);
            int len;
            len = _client.read((uint8_t *)_readBuffer, BUFSIZ);
            // DEBUGVAR("len", len);

            if ((len > 0) && (len < sizeof(_readBuffer))) {
              _httpContentLen -= len;
              _readBuffer[len] = NUL; _readBuffer[32] = NUL;
              // DEBUGVAR("data", _readBuffer);
            }
            // DEBUGVAR("_httpContentLen", _httpContentLen);

            if (_httpContentLen == 0) {
              respondHeaderContentType("text/html");
              _client.print("Content-Length: 0\r\n");
              webstate = PROCESS_STOP;
            } // if

            if (strcmp(_httpURI, "/$radio") == 0) {
              // search the space
              p = strchr(_readBuffer, SPACE);
              if (p) {
                *p++ = NUL;
                runRadioCommand(_readBuffer, atoi(p));
              } // if              
            } // if
          } else if (webstate == PROCESS_PUT) {
            // upload a file
            int len;
            len = _client.read((uint8_t *)_readBuffer, 32);
            if (len > 0) {
              _readBuffer[len] = NUL; _readBuffer[32] = NUL;

              if (!f) {
                f = SD.open(_httpURI, O_CREAT | O_WRITE | O_TRUNC);
                if (!f) {
                  webstate = PROCESS_ERR;
                } // if
              }  // if
              f.write((uint8_t *)_readBuffer, len);
              _httpContentLen -= len;
            } // if
            // DEBUGTEXT(_httpContentLen);

            if (_httpContentLen == 0) {
              f.close();
              webstate = PROCESS_STOP;
            }
          } // PROCESS_PUT

          if (webstate == PROCESS_ERR) {
            // everything else is a 404
            respondHeaderNotFound();
            webstate = PROCESS_STOP;
          } // if

        } // if

        if (webstate == PROCESS_STOP) {
          // DEBUGTEXT("PROCESS_STOP");
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



/// Update the Frequency on the LCD display.
void DisplayFrequency()
{
  char s[12];
  radio.formatFrequency(s, sizeof(s));
  lcd.setCursor(0, 0);
  lcd.print(s);
  DEBUGVAR("FREQ", s);
} // DisplayFrequency()


/// This function will be called by the RDS module when a rds service name was received.
/// The text be displayed on the LCD and written to the serial port
/// and will be stored for the web interface.
void DisplayServiceName(char *name)
{
  Serial.print("RDS:"); Serial.println(name);
  strncpy(rdsServiceName, name, sizeof(rdsServiceName));

  if (rot_state == STATE_FREQ) {
    lcd.setCursor(0, 1);
    lcd.print(name);
  }
} // DisplayServiceName()


/// This function will be called by the RDS module when a rds text message was received.
/// The text will not displayed on the LCD but written to the serial port
/// and will be stored for the web interface.
void DisplayText(char *text)
{
  Serial.print("TEXT:"); Serial.println(text);
  strncpy(rdsText, text, sizeof(rdsText));
} // DisplayText()


/// This function will be called by the RDS module when a rds time message was received.
/// The time will not displayed on the LCD but written to the serial port.
void DisplayTime(uint8_t hour, uint8_t minute) {
  Serial.print("RDS-Time:");
  if (hour < 10) Serial.print('0');
  Serial.print(hour);
  Serial.print(':');
  if (minute < 10) Serial.print('0');
  Serial.println(minute);
} // DisplayTime()


/// Display the current volume.
/// This function is called after a new volume level is choosen.
/// The new volume is displayed on the LCD 2. Line.
void DisplayVolume(uint8_t v)
{
  DEBUGFUNC1("DisplayVolume", v);
  lcd.setCursor(0, 1);
  lcd.print("VOL: ");  lcd.print(v);
  lcd.print("     ");
} // DisplayVolume()


/// Display the current mono switch.
void DisplayMono(uint8_t v)
{
  DEBUGFUNC1("DisplayMono", v);
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


/// Response to a $info request and return all information of the current radio operation.
/// Format all data to the format NAME blank VALUE\r\n.
void respondRadioData()
{
  // DEBUGFUNC0("respondRadioData");
  char s[12];
  respondHeaderContentType("text/plain");
  _client.print("Cache-Control: no-cache\r\n");
  _client.print(HTTP_ENDHEAD);

  // return frequency
  _client.print("freq ");  _client.println(radio.getFrequency());
  _client.print("band ");  _client.println(radio.getBand());

  // return radio related features
  RADIO_INFO ri;
  radio.getRadioInfo(&ri);
  _client.print("mono "); _client.println(ri.mono);      // force mono mode.
  _client.print("stereo "); _client.println(ri.stereo);  // has stereo signal
  // _client.print("rds ");  _client.println(ri.rds);       // has rds signal

  // return rds information 
  _client.print("servicename ");  _client.println(rdsServiceName);
  _client.print("rdstext ");  _client.println(rdsText);

  // return audio related features
  AUDIO_INFO ai;
  radio.getAudioInfo(&ai);

  _client.print("vol ");       _client.println(ai.volume);
  _client.print("mute ");      _client.println(ai.mute);
  _client.print("softmute ");  _client.println(ai.softmute);
  _client.print("bassboost "); _client.println(ai.bassBoost);

} // respondSystemInfo()

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
// Read http://www.atmel.com/Images/doc8468.pdf for more details on external interrupts.

ISR(PCINT2_vect) {
  encoder.tick(); // just call tick() to check the state.
} // ISR for PCINT2


// ----- Setup and run the radio -----

/// Setup a FM only radio configuration with I/O for commands and debugging on the Serial port.
void setupRadio() {
  // Enable information to the Serial port
  radio.debugEnable();

  // Initialize the Radio
  radio.init();

  // radio.setBandFrequency(RADIO_BAND_FM, 8930); // hr3
  radio.setBandFrequency(RADIO_BAND_FM, preset[i_sidx]); // 5. preset.
  // radio.setFrequency(10140); // Radio BOB // preset[i_sidx]

  radio.setMono(false);
  radio.setMute(false);
  // radio.debugRegisters();
  radio.setVolume(6);

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

  state = STATE_PARSECOMMAND;
  rot_state = STATE_NONE;

  // setup the information chain for RDS data.
  *rdsServiceName = NUL;
  radio.attachReceiveRDS(RDS_process);

  rds.attachServicenNameCallback(DisplayServiceName);
  rds.attachTextCallback(DisplayText);
  rds.attachTimeCallback(DisplayTime);

} // setupRadio


/// Check once new radio data.
void loopRadio(unsigned long now) {
  // check for RDS data
  radio.checkRDS();
} // loopRadio()


/// Execute a command identified by a character and an optional number.
/// See the "?" command for available commands.
/// \param cmd The command word.
/// \param value An optional parameter for the command.
void runRadioCommand(char *cmd, int16_t value) {
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
  } // if
} // runRadioCommand()


// ----- Check for commands on the Serial interface -----
// ----- Setup and run the radio -----

/// setupSerial


/// Constantly check for serial input commands and trigger command execution.
void loopSerial(unsigned long now) {
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
        runSerialCommand(command, value);
        command = ' ';
        state = STATE_PARSECOMMAND;
        value = 0;
      } // if
    } // if
  } // if

} // loopSerial


void runSerialCommand(char cmd, int16_t value)
{
  if (cmd == '?') {
    Serial.println();
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
  }

  // ----- control the volume and audio output -----

  else if (cmd == 'u') {
    // toggle soft mute mode
    radio.setSoftMute(!radio.getSoftMute());
  }

  // toggle stereo mode
  else if (cmd == 's') { radio.setMono(!radio.getMono()); }

  // toggle bass boost
  else if (cmd == 'b') { radio.setBassBoost(!radio.getBassBoost()); }

  // ----- control the frequency -----

  else if (cmd == '>') {
    // next preset
    if (i_sidx < (sizeof(preset) / sizeof(RADIO_FREQ)) - 1) {
      i_sidx++; radio.setFrequency(preset[i_sidx]);
    } // if
  } else if (cmd == '<') {
    // previous preset
    if (i_sidx > 0) {
      i_sidx--;
      radio.setFrequency(preset[i_sidx]);
    } // if

  } else if (cmd == 'f') { radio.setFrequency(value); } else if (cmd == 'v') { radio.setVolume(value); } else if (cmd == 'm') { radio.setMute(value > 0); }

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

    RADIO_INFO ri;
    radio.getRadioInfo(&ri);


    //     Serial.print("  RSSI: ");
    //     Serial.print(info.rssi);
    //
    //     for (uint8_t i = 0; i < info.rssi - 15; i+=2) { Serial.write('*'); } // Empfangspegel ab 15. Zeichen
    //     Serial.println();
    delay(3000);

  } // info
  //  else if (cmd == 'n') { radio.debugScan(); }
  else if (cmd == 'x') { radio.debugStatus(); }


} // runSerialCommand()


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
    Serial.println("B1");
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

  } else if (rot_state != STATE_FREQ)  {
    if (now > encoderLastTime + ENCODER_FALLBACK) {
      // fall into FREQ + RDS mode
      rot_state = STATE_FREQ;
      encoderLastPos = (radio.getFrequency() - radio.getMinFrequency()) / radio.getFrequencyStep();
      encoder.setPosition(encoderLastPos);
      encoderLastTime = now;
    } // if

  } // if

} // loopButtons


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

// End.

