This directory contains the sources and pre-compiled hex files for the
bootloader. There are two bootloaders available:

Hexbright
=========

This variant is derived directly from the factory bootloader, but thanks to
code downsizing, it also includes Erik Lins' MONITOR code, which was omitted
from the original ATmega168 bootloader because of its size. You can enter
MONITOR mode by sending three consecutive exclamation marks (`!`) over the
serial line while the bootloader is running. Following commands are available:

* `t`: Toggle the green rear LED.
* `r aaaa`: Read byte from RAM address _aaaa_ (specified as 4 hexadecimal
  digits).
* `w aaaa vv`: Write the value _vv_ into RAM location _aaaa_.
* `u`: UART loopback (everything you write is echoed back to you).
* `j`: Jump to the application.

The modified bootloader also flashes the green LED once at startup. The
factory code has no such indication.

Hexbright Tiny
==============

Hexbright Tiny is a bootloader with only the basic functionality. However, it
fits into 1024 bytes of code, leaving an extra 1k for the actual Hexbright
program. Note that this appears as a separate "board" in Arduino.

Signature Bytes Mismatch
========================

The Hexbright contains an ATmega168PA (signature bytes `1E 94 0B`), i.e. the
picoPower version of ATmega168. However the factory code was compiled for a
standard ATmega168 (signature bytes `1E 94 06`). The memory layout is exactly
the same on both models, so an ATmega168 bootloader will work fine on an
ATmega168PA, but it causes trouble when programming using the Serial
Peripheral Interface (SPI).

In short, when programmed with the bootloader, the Hexbright appears to
Arduino as an ATmega168, but when programmed through an external ISP, it
appears as an ATmega168P, so Arduino detects a mismatch in the signature bytes
and stops.

The signature bytes have been fixed in the bootloader sources, but there are
too many Hexbright lights around with the (incorrect) factory settings, and
since most people won't update the bootloader, the Hexbright entry in
`boards.txt` was left unchanged.

To sum it up:

* It is not possible to burn a new bootloader into your Hexbright using
  Arduino, unless you modify `boards.txt` first (see below).
* If you do modify `boards.txt`, you will not be able to use this entry to
  program a Hexbright with the default bootloader.
* Everything works perfectly if you select Hexbright Tiny as your Board in
  Arduino.

Burning the Bootloader
======================

First, you may not need to update your bootloader. All currently available
programs work just fine with the stock bootloader. You have to update the
bootloader if:

1. you need a bit more space for your custom Hexbright program;
2. you want to add your own features to the bootloader;
3. or you simply love to do "cool" things.

You still want to try it? OK, then you need a few things before you start:

* An AVR In-System Programmer (ISP)

  The bootloader cannot be programmed using the Hexbright alone, because the
  bootloader memory area is locked (write protected). The lock can be removed
  only with a Chip Erase command, and there is no way to issue this command
  without additional hardware. You can buy an Atmel AVR ISP mkII, or any of
  the many third-party devices on the market, e.g. the USBTinyISP device from
  Adafruit.

* 6 Pins

  The Hexbright doesn't have any connector on the motherboard. The standard
  AVR programming connector would even be too big to fit inside the outer
  case, so you probably don't want to solder it there. To make a temporary
  electric connection between, you can insert individual pins into the ISP
  connector (to make it a kind of a male connector), and then insert these
  pins into the corresponding 6 plated through-holes on the Hexbright
  motherboard.

Proceed as follows:

1. Unscrew the rear cap of your Hexbright and slide the interior frame out of
   the Hexbright.

2. Connect your AVR ISP device to the ICSP connector on the Hexbright
   motherboard.

3. Connect your Hexbright to a USB charger.

   This step is needed to supply power to the MCU while programming it through
   SPI. Since the RESET pin is low during serial downloading, DPIN_PWR will be
   in hi-Z state, so the MIC5353 voltage regulator is disabled. Thankfully,
   there is another voltage regulator in the CP2102 USB-to-serial converter,
   which can supply power to the MCU.
   
   Alternatively, if your programmer has an option to supply power to the MCU,
   you can switch it on. Make sure you select 3.3V for the output! For
   example, my Diamex-AVR DIP switch settings is 1=ON, 2=OFF. If you do this,
   then do not connect the Hexbright to a USB charger.

3. Plug the ISP device into your computer. Consult your device's guide for
   details.

4. Select your programmer in Arduino in the Tools->Programmer menu.

5. Select the desired board in Tools->Board (either Hexbright or Hexbright
   Tiny).
   
   Please note that if you select Hexbright here, you will have to modify your
   `boards.txt` and change this line:
   
        hexbright.build.mcu=atmega168

   to:
   
        hexbright.build.mcu=atmega168p

   See Signature Bytes Mismatch for an explanation.

6. Select Tools->Burn Bootloader from the menu.

You can also burn the bootloader by running `make hexbright_isp` or `make
hexbright_tiny_isp` in this directory. Before you do that, you will probably
have to adjust the programmer parameters in `Makefile` (`ISPTOOL`, `ISPPORT`
and `ISPSPEED`).
