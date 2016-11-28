# Iono Pi - C library and utility program
C library and utility program for Iono Pi (www.sferalabs.cc/iono-pi), a professional input/output expansion for Raspberry Pi.

This library provides a set of utility functions to use all the functionalities provided by Iono Pi:
* Power relays, open collectors and LED control
* Digital inputs reading and interrupts handling
* Analog inputs reading
* 1-Wire and Wiegand support

It is developed for [Raspberry Pi](https://www.raspberrypi.org/) 2/3 running [Raspbian](https://www.raspberrypi.org/downloads/raspbian/) and based on the [WiringPi GPIO access library](http://wiringpi.com/).

Moreover it includes the `iono` utility program that can be used in scripts or directly from shell to access Iono Pi's functionalities.

The [source code](./ionoPi/ionoPiUtil.c) of the `iono` utility can also be used as a simple axample of how to use the ionoPi library.

## Raspberry Pi Setup

Enable SPI:

    $ sudo raspi-config
    
Select: Advanced Options -> SPI -> enable

If using 1-Wire bus devices (e.g. DS18B20 temperature sensors) on the TTL1 pin, enable thet too:

Select: Advanced Options -> 1-wire -> enable

Otherwise, make sure it is disabled.

Reboot:

    $ sudo reboot
    
## Installation

Make sure your Pi is up to date with the latest versions of Raspbian:

    $ sudo apt-get update
    $ sudo apt-get upgrade

If you don't have git installed:

    $ sudo apt-get install git-core
    
Download ionoPi using git:

    $ git clone --recursive https://github.com/sfera-labs/iono-pi-c-lib.git
    
Build and install:

    $ cd iono-pi-c-lib
    $ ./build
    
Test the installation running the `iono` utility:

    $ iono
    
## IonoPi library documentation

TODO
