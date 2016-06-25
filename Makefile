# Makefile for Thing, the robotic hand controller

MCU           := atxmega128a1
COMPILE_FLAGS := -Os -std=c99 -Wall -ffunction-sections -fdata-sections
LINK_FLAGS    := -flto -fwhole-program -Wl,-gc-sections
INCLUDES      := include/adc_driver.h include/avr_compiler.h include/board.h include/esp_driver.h include/pmic_driver.h include/serio_driver.h include/servo_driver.h include/TC_driver.h include/usart_driver.h include/utils.h
OBJECTS       := main.o esp_driver.o servo_driver.o serio_driver.o TC_driver.o pmic_driver.o adc_driver.o usart_driver.o

all: firmware.hex

%.o: src/%.c $(INCLUDES)
	@echo Compiling $<
	@avr-gcc -iquote. $(COMPILE_FLAGS) -mmcu=$(MCU) -c $<

firmware.hex: $(OBJECTS)
	@avr-gcc $(LINK_FLAGS) -mmcu=$(MCU) -o firmware.elf $(OBJECTS)
	@avr-objcopy -j .text -j .data -O ihex firmware.elf firmware.hex
	@avr-size -C --mcu=$(MCU) firmware.elf

flash: firmware.hex
	avrdude -p x128a1 -c avrispmkII -P usb -U flash:w:firmware.hex:i

clean:
	@rm -f *.o firmware*