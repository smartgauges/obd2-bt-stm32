PREFIX=arm-none-eabi
CC=$(PREFIX)-gcc
OBJCOPY=$(PREFIX)-objcopy
AR=$(PREFIX)-ar
COMMON_FLAGS=-Wall -mcpu=cortex-m0 -mthumb -msoft-float
CFLAGS=$(COMMON_FLAGS) -Os -std=gnu99 -I. -Ilibopencm3/include -DSTM32F0 -fno-common -ffunction-sections -fdata-sections \
       -Wstrict-prototypes -Wundef -Wextra -Wshadow -Wredundant-decls
LDFLAGS=$(COMMON_FLAGS) -lc -nostartfiles -Wl,--gc-sections -Wl,--print-gc-sections

PHONY:all
all: bl.bin main.bin

OPENCM3_OBJS = \
 		libopencm3/lib/cm3/vector.o \
		libopencm3/lib/cm3/systick.o \
 		libopencm3/lib/cm3/nvic.o \
 		libopencm3/lib/cm3/sync.o \
		libopencm3/lib/cm3/assert.o \
		libopencm3/lib/stm32/common/gpio_common_all.o \
 		libopencm3/lib/stm32/common/gpio_common_f0234.o \
		libopencm3/lib/stm32/common/usart_common_v2.o \
		libopencm3/lib/stm32/common/rcc_common_all.o \
 		libopencm3/lib/stm32/common/pwr_common_all.o \
 		libopencm3/lib/stm32/common/flash_common_f01.o \
		libopencm3/lib/stm32/f0/gpio.o \
		libopencm3/lib/stm32/f0/rcc.o \
		libopencm3/lib/stm32/f0/pwr.o \
		libopencm3/lib/stm32/f0/flash.o \
		libopencm3/lib/stm32/f0/usart.o \
		libopencm3/lib/stm32/can.o \
		libopencm3/lib/stm32/common/iwdg_common_all.o

%.o : %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

main.bin: main.o can.o led.o ring.o hdlc.o crc_xmodem.o $(OPENCM3_OBJS)
	$(CC) $^ -L. -Tstm32f04xz6-app.ld $(LDFLAGS) -Wl,-Map=main.map -o main.elf
	$(OBJCOPY) -O binary main.elf $@

bl.bin: bl.o led.o ring.o hdlc.o crc_xmodem.o $(OPENCM3_OBJS)
	$(CC) $^ -L. -Tstm32f04xz6-bl.ld $(LDFLAGS) -Wl,-Map=bl.map -o bl.elf
	$(OBJCOPY) -O binary bl.elf $@

full.bin: bl.bin main.bin
	srec_cat bl.bin -binary -output bl.srec -motorola
	srec_cat main.bin -binary -exclude 0x0 0x2 -little_endian_max 0x0 -crop 0x0 0x2 -output main-len.srec -motorola
	srec_cat main.bin -binary -little_endian_crc16 0x6ffe -cyclic_redundancy_check_16_xmodem -crop 0x6ffe 0x7000 -output main-crc.srec -motorola
	srec_cat main.bin -binary main-len.srec -motorola -offset 0x6ffc main-crc.srec -motorola -output main-full.srec -motorola
	srec_cat bl.bin -binary main-full.srec -motorola -offset 0x1000 -output full.bin -binary
	rm -f *.srec

flash: full.bin
	./stm32loader.py -p /dev/ttyUSB0 -b 115200 -V -e -w -v $<

clean:
	rm -rf main.bin bl.bin full.bin *.srec *.o $(OPENCM3_OBJS) *.elf *.map

