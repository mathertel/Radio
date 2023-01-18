# LCDRadio Example

This example allows controlling the a supported radio chip by using 

* a rotary encoder
* a push button
* a LCD display attached to the i2c bus using a PCF8574 based adapter.

The following additional libaries are required for this example:

**LiquidCrystal_PCF8574** --
The <https://github.com/mathertel/LiquidCrystal_PCF8574> library is used
to control the LCD via I2C using a PCF8574 based i2c adapter.

The LiquidCrystal_PCF8574 library is available using the Arduino Library.

**RotaryEncoder** -- 
The <https://github.com/mathertel/RotaryEncoder> library is used
to process signals from the RotaryEncoder.

The RotaryEncoder library is available using the Arduino Library.

**OneButton** --
The <https://github.com/mathertel/OneButton> library is used
to process signals from a momentary input.

The OneButton library is available using the Arduino Library.

It works with processor boards for Arduino UNO, ESP8266 like NodeMCU 1.0 and ESP32.

This sketch works with all of the implemented chips by using the common radio functions of the library.

You have to modify the source code line where the radio variable is defined to the chip you use.
There are out-commented lines for every chip in the sketch.
Be sure to activate the lines for the RST signal for SI4703 radio chips.

The Serial interface, the rotary encoder and the momentary button can be used
for controlling the chip.
Open the Serial Monitor window of the Arduino programming environment
you can see verbose output from the library and can control the settings
by entering commands in the input text line.

The command ? (and enter) will display a short summary of the available commands.

If you like to explore the chip specific settings you may use the "x" command
that outputs the chip registers or other chip specific information.

Have a look into the chip specific implementation for more details.

Please read the README.md files in the TEST____ examples for more chip specific hints.
