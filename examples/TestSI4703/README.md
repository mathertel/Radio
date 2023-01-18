# Si4703

These are some hints when using a SI4703 radio chip.

* The Antenna is not available as a separate connector.
  The GND signal on the earphones is used instead as antenna.
* The RST pin must be connected as this is required at startup to initialize I2C control mode.
* The SI4703 has 3 GPIO pins that also can be used for input and output.
  The `writeGPIO` function can be used for controlling these pins.
* The GPIO3 is used for wiring the XTAL so it cannot be used on these boards
  and also cannot be used as a stereo indicator.

A schema for a typical board is available on <http://domoticx.com/module-si4703-fm-tuner/>
