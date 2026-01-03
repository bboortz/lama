# PORT              ?= /dev/ttyUSB0
# # BAUD_RATE         ?= 460800
# # BAUD_RATE         ?= 230400
# BAUD_RATE         ?= 115200
# CHIP              ?= esp8266
# CHIP_ESP8266       = esp8266
# CHIP_ESP32         = esp32
# CHIP_ESP32C3       = esp32c3
# CHIP_ESP32C3S      = esp32c3s
# CHIP_ESP32S2       = esp32s2
# FIRMWARE_ESP8266   = esp8266-20230426-v1.20.0.bin 
# FIRMWARE_ESP32     = esp32-20230426-v1.20.0.bin
# FIRMWARE_ESP32C3   = firmware_generic_esp32_c3_2mb_1.18_4.3.1.bin # https://mega.nz/file/hFcRyQYa#yru2X7h9CxqwCrB_rDGKsZex_3y8dDfKxU-NtCCMWQg (https://forum.micropython.org/viewtopic.php?f=18&t=11025&sid=d9b90710278770df1b8f2c625cb4ddb3&start=10)
# FIRMWARE_ESP32C3S  = esp32c3-usb-20230426-v1.20.0.bin
# FIRMWARE_ESP32S2   = GENERIC_S2-20230426-v1.20.0.bin
# SRC_FILES         := $(wildcard *.py)
# OBJ_FILES         := $(patsubst %.py,%.pyc,$(SRC_FILES))


.PHONY: format lint braces check-braces check-all all
TIDY_FLAGS = -x c++ -std=c++11 -DARDUINO_ARCH_ESP32


help:                     ## printing out the help
	@echo
	@echo micropython-fun Makefile
	@echo
	@echo --- TARGETS ---
	@grep -F -h "##" $(MAKEFILE_LIST) | grep -F -v fgrep | sed -e 's/\\$$//' | sed -e 's/##//'



diff:                     ## git diff
	@git diff


clean:              ## clean up this project
	$(MAKE) -C lama-main-pio clean
	$(MAKE) -C lama-packet-rust clean
	$(MAKE) -C lama-core-rust clean
c: clean

fmt:                ## format the sources
	$(MAKE) -C lama-main-pio fmt
	$(MAKE) -C lama-packet-rust fmt
	$(MAKE) -C lama-core-rust fmt
f: fmt

build:              ## build the software
	$(MAKE) -C lama-main-pio build
	$(MAKE) -C lama-packet-rust build
	$(MAKE) -C lama-core-rust build
b: build

test: fmt build     ## test the software
	$(MAKE) -C lama-main-pio test
	$(MAKE) -C lama-packet-rust test
	$(MAKE) -C lama-core-rust test
t: test


upload: fmt build   ## upload the firmware
	$(MAKE) -C lama-main-pio upload
u: upload

monitor:            ## monitor the device
	$(MAKE) -C lama-main-pio monitor
m: monitor


doc:                ## create the documentation
	echo "not implemented"


