# What is Simple Watchdog Daemon ? ##

Simple Watchdog daemon is an hardware watchdog that run as a daemon. It does not
have all the features the other watchdogs have, however it is really light and
easy to use and it keep your system alive as a watchdog is suppose to do. So if
you need an hardware watchdog but do not want to deal with complicated software,
you might be in the right place.

### Distribution ###

Simple Watchdog Daemon is designed to work with any linux that have an harware
watchdog driver loaded.

It have been test on :
* Ubuntu 16.06 and 18.04
* STM32MP157 with OpenSTLinux

## How to compile it ? ##

To compile it, just invoke make. You can specify your compiler if you want with
`CROSS_PREFIX`

Example (with cross compilation) :
```sh
make all CROSS_PREFIX=arm-linux-gnuaebihf-gcc
```

## How to install it ? ##

In order to install Simple Watchdog Daemon, just copy the `watchdogd` elf
somewhere in your path (or somewhere else, it's up to you).

## How to use it ? ##

In order to start Simple Watchdog Daemon during the boot you can use on of the
init_script or service provide in the `init_script` folder. You can also launch
it by calling the binary
```sh
watchdogd <args>
```

Arguments :

  * -d  Enable disarmement : the deamon will disarm the watchdog when closing
  * -D<driverFile\> : the watchdog will use the specified driver file
  * -t<timeOut\>    : the watchdog will use the specified timeOut in seconds
  * -p<pidFile\>    : the watchdog will use the specified pidFile to write its PID

## Contact ##

If you have any questions feel free to mail me : <antoine.braut@gmail.com>

**I hope this software will help you ! Have a nice day !**

<img src="https://images.ecosia.org/x8hEzRW0N0B1oHUTXqREorZ73aE=/0x390/smart/https%3A%2F%2Fcdn170.picsart.com%2Fupscale-241091004033212.png%3Fr1024x1024" alt="drawing" width="200"/>
