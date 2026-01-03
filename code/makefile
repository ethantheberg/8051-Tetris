#  * ---------------------------------------------------------------------------------
#  * Ethan Berg
#  * ECEN 5613 - Fall 2025 - Prof. McClure
#  * University of Colorado Boulder
#  * Created 10/11/25 
#  *  --------------------------------------------------------------------------------
#  * Lab 3 Makefile
#    ---------------------------------------------------------------------------------*/


PROJECT := lab4
SRC_DIR := src
INC_DIR := inc


C_FLAGS := --model-large -I"$(INC_DIR)"
L_FLAGS := --model-large --code-loc 0x0000 --code-size 0x8000 --xram-loc 0x0000 --xram-size 0x400

C_SRCS := $(wildcard $(SRC_DIR)/*.c)
RELS := $(patsubst $(SRC_DIR)/%.c,build/%.rel,$(C_SRCS))


PORT := /dev/tty.usbserial-AC02483O 
# PORT := /dev/tty.usbserial-0001 
#/dev/ttyUSB1

TERM_CMD := screen $(PORT) 57600 
#minicom -D $(PORT) -b 57600

all: build/$(PROJECT).hex

build:
	mkdir build

build/%.rel: src/%.c | build
	sdcc $(C_FLAGS) -c $< -o $@

build/$(PROJECT).ihx:  $(RELS)
	sdcc $(L_FLAGS) $(RELS) -o $@

build/$(PROJECT).hex: build/$(PROJECT).ihx
# 	packihx build/$(PROJECT).ihx > build/$(PROJECT).hex
	mv build/$(PROJECT).ihx build/$(PROJECT).hex
	python3 clean_hex.py build/$(PROJECT).hex

.PHONY: clean connect deploy
clean:
	rm -rf build/

connect:
	tio -b 57600 $(PORT)
deploy: all
	python3 upload_hex.py --baud 115200 $(PORT) build/$(PROJECT).hex
 	echo -e ':0400000303010000F5\n' > $(PORT)
paulmon: 
	python3 upload_hex.py --baud 115200 $(PORT) ../paulmon.hex
test: deploy
	echo 'J' > $(PORT)
	sleep 1
	echo '2000' > $(PORT)