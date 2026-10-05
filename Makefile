TARGET = main
MCU = atmega328p
F_CPU = 16000000UL
PORT = /dev/ttyUSB0
BAUD = 115200

CC = avr-gcc
OBJCOPY = avr-objcopy
SIZE = avr-size
AVRDUDE = avrdude

CFLAGS = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -Os -Wall -Wextra

SRC = main.c uart/uart.c timer/timer.c task/task.c shell/shell.c kernel/port.c
HEADERS = include/bool.h uart/uart.h timer/timer.h task/task.h shell/shell.h kernel/port.h
ELF = $(TARGET).elf
HEX = $(TARGET).hex

.PHONY: all flash clean size

all: $(HEX)

$(ELF): $(SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(SRC) -o $@

$(HEX): $(ELF)
	$(OBJCOPY) -O ihex -R .eeprom $< $@
	$(SIZE) --mcu=$(MCU) -C $(ELF)

flash: $(HEX)
	$(AVRDUDE) \
		-p $(MCU) \
		-c arduino \
		-P $(PORT) \
		-b $(BAUD) \
		-D \
		-U flash:w:$(HEX):i

size: $(ELF)
	$(SIZE) --mcu=$(MCU) -C $(ELF)

clean:
	rm -f $(ELF) $(HEX)
