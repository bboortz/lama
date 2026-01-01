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




format:                   ## formatting all files
	@echo "Formatting code..."
	@find . -maxdepth 2 \( -name "*.ino" -o -name "*.h" -o -name "*.cpp" \) \
	  | xargs clang-format -i --verbose

braces:			  ## add braces to all files
	@echo "Adding braces to all files..."
	@for file in *.ino *.h *.cpp; do \
	  [ -f "$$file" ] && echo "Fixing $$file..." && \
	  clang-tidy -fix -fix-errors "$$file" \
	    -checks='readability-braces-around-statements' \
	    -- $(TIDY_FLAGS); \
	done

check-braces:             ## check braces
	@echo "Checking for missing braces..."
	@for file in *.ino *.h *.cpp; do \
	  [ -f "$$file" ] && \
	  clang-tidy "$$file" \
	    -checks='readability-braces-around-statements' \
	    -- $(TIDY_FLAGS) 2>&1 | grep "warning:" || true; \
	done

check-bugs:               ## check bugs
	@echo "Checking for potential bugs..."
	@for file in *.ino *.h *.cpp; do \
	  [ -f "$$file" ] && echo "Checking $$file..." && \
	  clang-tidy "$$file" \
	    -checks='bugprone-*' \
	    -- $(TIDY_FLAGS) 2>&1 | grep "warning:" || true; \
	done

check-performance:         ## check performance
	@echo "Checking for performance issues..."
	@for file in *.ino *.h *.cpp; do \
	  [ -f "$$file" ] && echo "Checking $$file..." && \
	  clang-tidy "$$file" \
	    -checks='performance-*' \
	    -- $(TIDY_FLAGS) 2>&1 | grep "warning:" || true; \
	done

check-all:                 ## check all
	@echo "Running all clang-tidy checks..."
	@for file in *.ino *.h *.cpp; do \
	  [ -f "$$file" ] && echo "Checking $$file..." && \
	  clang-tidy "$$file" -- $(TIDY_FLAGS) 2>&1 | grep "warning:" || true; \
	done

lint:	                   ## lint
	@echo "Running cppcheck..."
	@cppcheck --enable=all --std=c++11 --platform=avr8 \
	  --suppressions-list=cppcheck-suppressions.txt \
	  --language=c++ \
	  --file-list=<(find . -name "*.ino" -o -name "*.cpp" -o -name "*.h")

all: check-all format lint    ## all
	@echo "All checks complete!"


monitor:                  ## monitor the devices using serial
	pyserial-miniterm $(PORT) $(BAUD_RATE)

