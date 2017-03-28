# Iono Pi - C library and utility program
C library and utility program for Iono Pi (www.sferalabs.cc/iono-pi), a professional input/output expansion for Raspberry Pi.

This library provides a set of utility functions to use all the functionalities provided by Iono Pi:
* Power relays, open collectors and LED control
* Digital inputs reading and interrupts handling
* Analog inputs reading
* 1-Wire and Wiegand support

It is developed for [Raspberry Pi](https://www.raspberrypi.org/) 2/3 running [Raspbian](https://www.raspberrypi.org/downloads/raspbian/) and based on the [WiringPi GPIO access library](http://wiringpi.com/).

Moreover it includes the `iono` utility program that can be used in scripts or directly from shell to access Iono Pi's functionalities.

The [source code](./ionoPi/ionoPiUtil.c) of the `iono` utility can also be used as a simple example of how to use the ionoPi library.

## Raspberry Pi Setup

Enable SPI:

    $ sudo raspi-config
    
Select: Interfacing Options -> SPI -> Yes

If using 1-Wire bus devices (e.g. DS18B20 temperature sensors) on the TTL1 pin, enable that too:

Select: Interfacing Options -> 1-Wire -> Yes

Otherwise, make sure it is disabled.

Reboot:

    $ sudo reboot
    
## Installation

Make sure your Pi is up to date with the latest versions of Raspbian:

    $ sudo apt-get update
    $ sudo apt-get upgrade

If you don't have git installed:

    $ sudo apt-get install git-core
    
Download ionoPi (which includes wiringPi) using git:

    $ git clone --recursive https://github.com/sfera-labs/iono-pi-c-lib.git
    
Build and install:

    $ cd iono-pi-c-lib
    $ sudo chmod +x build
    $ sudo ./build
    
Test the installation running the `iono` utility:

    $ iono
    
## IonoPi library documentation

To use the library in your C code, include its header file:

    #include <ionoPi.h>

and add `-lionoPi` when compiling, e.g.:

    $ gcc -Wall -o foo foo.c -lionoPi
    
you may need to specify the paths to the library too:

    $ gcc -Wall -o foo foo.c -I/usr/local/include -L/usr/local/lib -lionoPi
    
The library defines the following `int` constants representing the pins (inputs/outputs) of Iono Pi:

   `O1`, `O2`, `O3`, `O4`    
   `OC1`, `OC2`, `OC3`    
   `DI1`, `DI2`, `DI3`, `DI4`, `DI5`, `DI6`       
   `AI1`, `AI2`, `AI3`, `AI4`    
   `TTL1`, `TTL2`, `TTL3`, `TTL4`    
   `LED`
   
They shall be used as parameters for the library functions.

Moreover, the following `int` constants are defined to simplify the state values of the digital pins:

   `CLOSED` or `OPEN`    
   `HIGH` or `LOW`    
   `ON` or `OFF`
   
which all respectively corresponds to the *high* or *low* state of the underlying GPIO pin.

Following are the functions provided by the library:

#### int ionoPiSetup()

This function **must** be called once, at the beginning of your program. It initializes the library and configures the Raspberry Pi's GPIO pins.

Returns `TRUE` upon success, `FALSE` otherwise.

#### void ionoPiPinMode(int pin, int mode)

This function sets the mode (`INPUT` or `OUTPUT`) of a pin. You might need it on the TTL lines, the other pins are initialized for their normal usage at setup.

#### void ionoPiDigitalWrite(int output, int value)

Sets the state of a digital output (`O1`, `O2`, `O3`, `O4`, `OC1`, `OC2`, `OC3`, `TTL1`, `TTL2`, `TTL3`, `TTL4`, or `LED`) to the specified value (`CLOSED` or `OPEN`, `HIGH` or `LOW`, `ON` or `OFF`).

#### int ionoPiDigitalRead(int di)

Returns the state (`HIGH` or `LOW`) of the specified digital input (`DI1`, `DI2`, `DI3`, `DI4`, `DI5`, `DI6`, `TTL1`, `TTL2`, `TTL3`, `TTL4`).

#### int ionoPiAnalogRead(int ai)

Returns the value read from the specified analog input (`AI1`, `AI2`, `AI3`, `AI4`), or `-1` if an error occurs.

#### float ionoPiVoltageRead(int ai)

Returns the voltage value read from the specified analog input (`AI1`, `AI2`, `AI3`, `AI4`), or `-1` if an error occurs.

#### int ionoPiDigitalInterrupt(int di, int mode, void (*callback)(int, int))

This function registers a callback function to be called when an interrupt is received on the specified digital input. The `mode` parameter specifies on which edge(s) the interrupt is detected, it can be `INT_EDGE_FALLING`, `INT_EDGE_RISING`, or `INT_EDGE_BOTH`.

The callback function must have the following signature:

    void myCallback(int di, int val)

When called, the parameters will be set respectively to the digital pin on which the interrupt triggered and its current state.

#### void ionoPiSetDigitalDebounce(int di, int millis)

Sets a debouce time (in milliseconds) on the specified digital input.

If set to a value greater than 0, state variations on the specified input will have effect only if stable for a period longer than the specified debounce time.     
This will affect the value returned by `ionoPiDigitalRead()` called on the same input and the triggering of interrupts if a callback function has been registered with `ionoPiDigitalInterrupt()`. 

#### int ionoPi1WireBusGetDevices(char*** ids)

This function retrieves the IDs of the devices connected to the 1-Wire bus. It will populate the array of char strings `ids` passed by address, allocating the required memory.

Returns the number of devices found or `-1` upon error.

#### int ionoPi1WireBusReadTemperature(const char* deviceId, const int attempts, int *temp)

Reads the temperature measured by the specified 1-Wire bus device. It sets the value of the `temp` parameter passed by address to the read temperature, in millis of °C. The `attempts` parameter specifies the maximum number of subsequent readings that must be attempted in case of errors. 

Returns `TRUE` upon success, `FALSE` otherwise.

#### int ionoPi1WireMaxDetectRead(int ttl, const int attempts, int *temp, int *rh)

Reads the temperature and relative humidity values measured by the 1-Wire MaxDetect probe connected to the specified TTL pin (`TTL1`, `TTL2`, `TTL3`, `TTL4`). It sets the values of the `temp` and `rh` parameters passed by address respectively to the read temperature (in tenths of °C) and humidity (in tenths of %). The `attempts` parameter specifies the maximum number of subsequent readings that must be attempted in case of errors. 

Returns `TRUE` upon success, `FALSE` otherwise.

#### int ionoPiWiegandMonitor(int interface, int (*callback)(int, int, uint64_t))

This function registers a callback function to be called when data is available on the specified Wiegand interface.

This function is **blocking**, it will not return until `ionoPiWiegandStop()` is called from a different thread, the callback function returns `FALSE` when called, or an error occurs.

The `interface` parameter shall be set to `1` to monitor the Wiegand device connected to TTL1 (Data 0) and TTL2 (Data 1), or to `2` to monitor the Wiegand device connected to TTL3 (Data 0) and TTL4 (Data 1).

Returns `TRUE` when stopped or `FALSE` if an error occurs.

The callback function shall have the following signature:

    int myCallback(int interface, int bitCount, uint64_t data)
    
It will be called any time data is available on the Wiegand interface with the parameters set as follows:

`interface` will be set to the number of the interface (`1` or `2`) the data has been read from;    
`bitCount` will be set to the number of bits read (for consistency check)     
`data` will be set to the value read (i.e. the sequence of bits interpreted as integer)

If the callback function returns `FALSE` upon completion the monitoring of the interface will be stopped and `ionoPiWiegandMonitor()` will return `TRUE`.

#### int ionoPiWiegandStop(int interface)

This function stops the monitoring of the specified Wiegand interface (`1` or `2`), see `ionoPiWiegandMonitor()`.
