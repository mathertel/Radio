# SerialRadio Example

This example allows controlling the a supported radio chip by using the Serial interface
and without specific other hardware.

It works with processor boards for Arduino UNO, ESP8266 like NodeMCU 1.0 and ESP32.

This sketch works with all of the implemented chips by using the common radio functions of the library.

You have to modify the source code line where the radio variable is defined to the chip you use.
There are out-commented lines for every chip in the sketch.
Be sure to activate the lines for the RST signal for SI4703 radio chips.

The only IO this sketch does is through the Serial interface and when you open the Serial Monitor window
of the Arduino programming environment you can see verbose output from the library
and can control the settings by entering commands in the input text line.

The command ? (and enter) will display a short summary of the available commands.

If you like to explore the chip specific settings you may use the "x" command
that outputs the chip registers or other chip specific information.

Have a look into the chip specific implementation for more details.

Please read the README.md files in the TEST____ examples for more chip specific hints.
