#-------------------------------------------------------------------------
#    This source file is a part of Placid
#
#    For the latest info, see http:www.marrin.org/
#
#    Copyright (c) 2018-2019, Chris Marrin
#    All rights reserved.
#
#    Use of this source code is governed by the MIT license that can be
#    found in the LICENSE file.
#-------------------------------------------------------------------------

FLOATTYPE = FLOAT32

PLATFORMDIR = ESP

CUSTOM_LIBS = ../baremetal
EXCLUDE_DIRS = ../baremetal/RPi ../baremetal/Darwin

CHIP = esp8266
BOARD = d1_mini
FLASH_DEF = 4M3M
BUILD_DIR = build
UPLOAD_PORT = /dev/cu.wchusbserial2430
UPLOAD_SPEED = 921600
BUILD_THREADS = 1
BUILD_EXTRA_FLAGS = -D$(FLOATTYPE)

VERBOSE ?= 1

include ../baremetal/ESP/makeEspArduino.mk
