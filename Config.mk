# This is the Config.mk file from Circle. It has entries added to USEFLASHY to be able to flash executables to the RPi.
# To use it, copy this to the circle subdir and then read Flashy section of circle/doc/bootloader.txt.

PREFIX = arm-none-eabi-
AARCH = 32
RASPPI = 1

USEFLASHY = 1
SERIALPORT = /dev/cu.usbserial-AD02CUJ5
FLASHBAUD = 115200
USERBAUD = 115200
