# Changelog

All notable changes to this project will be documented in this file.

## [unpublished]

* Runtime Configuration of de-emphasis and channel spacing.

    ``` cpp
    // Set FM Options for Europe
    radio.setup(RADIO_FMSPACING, RADIO_FMSPACING_100);   // for EUROPE
    radio.setup(RADIO_DEEMPHASIS, RADIO_DEEMPHASIS_50);  // for EUROPE
    ```

  This code was added to the examples but not all radio chip implementation do use it yet.

* The initialization sequence on ESP32 is sensitive in case of SI4703
  as the SDA signal from I2C bus is also used to define the bus mode
  of the SI4703.
  It is important to NOT initialize the I2C bus before the reset of the radio chip.



## [3.0.0] - 2023-01-15

This is a major update as some behaviors in the library have changed.

### Major Changes

* RDSParser::attachServiceNameCallback has been renamed to avoid typo.
* The radio library includes a new function `getMaxVolume` to retrieve the max. volume level supported by the chip.
  The setVolume function now accepts values in the range of 0..getMaxVolume().
* Some functions have been corrected to use (const char *) instead of (char*) parameters to enforce that the passed strings are not changed
  (e.g. receiveServiceNameFunction or receiveTextFunction). Be sure to change the parameter type avoid casting errors/warnings.
* The interfaces to the Radio chip functionality has been changed to be more a property oriented interface.
  Use the `radio.setup()` function to specify chip specific features and extra pins.

### Other changes

* fixing warnings from ESP32 compiler
* A lot inline and example documentation
* The Wire object for I2C bus communication is now used through _i2cPort that can be initializes with initWire().
  This allows specifying the right i2c port in case the CPU supports multiple instances.
* The ESP32 processor can be used to compile some of the examples:
  * SerialRadio.ino --  just uncomment the chip you have.
  * TestSI4721.ino --  this example works with the SI4720 and SI4721 radio chips.
* RDS Error detection for SI4703
* The `.clang-format` and `.markdownlint.json` files are provided to define formatting rules.


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

* **[TransmitSI4721.ino](/examples/TransmitSI4721/TransmitSI4721.md)** - This example shows how to implement FM transmission using the SI4721 chip. Please respect your local radio transmission policies and rules by your government.


### Enhancements

* The base radio class implementations now has some I2c utility routings that will be further adopted in the chip libraries.
* Some code to initialize the I2C bus has been added to the examples to support ESP8266 boards.

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
