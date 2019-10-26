## Overview

This library is about controlling an FM radio chips by using an Arduino board and some optional components like a LCD display, a rotary encoder, a LCD+Keyboard shield or an Ethernet Shield to build a standalone radio.

In the [mathertel / Radio](https://github.com/mathertel/Radio) repository on github.com you find an Arduino library for implementing an FM receiver using one of the supported radio chips for receiving FM broadcast audio signals.

There are various examples included that show using the library using different hardware setups.

## Documentation

The documentation for the libraries in DOXYGEN style can be found at [http://mathertel.github.io/Radio/html](html/index.html).

A more detailed article is availabe at [www.mathertel.de/Arduino/RadioLibrary.aspx](http://www.mathertel.de/Arduino/RadioLibrary.aspx).

Currently the following chips are supported:

*   The RDA5807M from RDA Microelectronics
*   The SI4703 from Silicon Labs
*   The SI4705 from Silicon Labs
*   The TEA5767 from NXP

They all are capable for receiving FM radio stations in stereo with European and US settings and can be controlled by using the I2C bus. However there are differences in the sensitivity and quality and well on receiving RDS information from the stations.

For each of these chips a specific library is implemented that knows how to communicate with the chip using the I2C bus and the wire library. These libraries all share a common base, the radio library so that all the common code is only implemented once in there:

All the libraries share the same interface (defined by the radio library) so it is possible to exchange them when not using one of the chip specific functions.

## Examples

Within the Arduino library you can find examples that implement different scenarios to control the radio chips:

*   The basic examples only startup the chips and set a static station and volume.
*   The SerialRadio example needs only an arduino and uses the Serial in- and output to change the settings and report information.
*   The LCDRadio example is similar to SerialRadio but also populates some information to an attached LCD.
*   The LCDKeypadRadio example uses the popular LCDKeypad shield.
*   The WebRadio example is the most advanced radio that runs on an Arduino Mega with an Ethernet Shield and an rotator encoder. You can also control the radio by using a web site that is available on the Arduino.


