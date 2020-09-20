# Changelog

All notable changes to this project will be documented in this file.

## [2.0.0] - 2020-09-17 

> **Important changes**
>
> This release focuses adding the si4721 radio chip to the library. The adoption was done using the board from sparkfun you can find here:
> <https://www.sparkfun.com/products/15853>.
> 
> This chip also supports sending FM signals, there is a special example 'TransmitSI4721.ino' to show this functionality.

Thanks to [@NPoole](https://github.com/NPoole) for adding the adoption of the si4721 chip.


### Added Examples

* **[TestSI4721.ino](/examples/TestSI4721/TestSI4721.md)** - This is the simple fixed settings example to proof correct functionality and wiring.

* **[TransmitSI4721.ino](/examples/TransmitSI4721/TransmitSI4721.md)** - This example shows how to implement FM transmission using the SI4721 chip. Please respect your local radio transmission policies and rules by your govermant.



### Enhancements

* The base radio class implementations now has some I2c utility routings that will be further adopted in the chip libraries.
* Some code to initialize the I2C bus has beed added to the examples to support ESP8266 boards.

### Fixes

With some chips that support multiple modes the power up should be handles when specifying the mode in setBand and power situation will be handled according the following schema:

* The init() function establishes communication with the radio chip. As a result the radio chip will not yet work and is in power down state.
* The setBand() function will configure the radio chip and start operation. The radio chip will be in power up state.
* The term() function will stop any operation. The radio chip will be in power down state. Communication with the chip will still work.
* To implement a "standby" functionality setBand() for "on" and term() for "standby" should be used.

This is verified for SI4721 for now.


### Known Issues

The TransmitSI4721.ino example and the library doesn't offer all options for transmitting but some basic functionality.
Improvements are welcome.

---

## Notes

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html) as used for the Arduino libraries.

