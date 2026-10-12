.RECIPEPREFIX = >
DEVKITPRO ?= /opt/devkitpro
DEVKITARM ?= $(DEVKITPRO)/devkitARM
CC := $(DEVKITARM)/bin/arm-none-eabi-gcc
CTRULIB := $(DEVKITPRO)/libctru
TARGET := saute-saute-arena
ARCH := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS := -g -Wall -O2 -mword-relocations -ffunction-sections $(ARCH) -D__3DS__ -I$(CTRULIB)/include
LDFLAGS := -specs=3dsx.specs -g $(ARCH) -L$(CTRULIB)/lib

all: $(TARGET).3dsx

$(TARGET).elf: source/main.c
> $(CC) $(CFLAGS) $< $(LDFLAGS) -lctru -lm -o $@

$(TARGET).smdh:
> $(DEVKITPRO)/tools/bin/smdhtool --create "Saute-Saute Arena" "Ecrase l'adversaire 3 fois" "Moi" $(CTRULIB)/default_icon.png $@

$(TARGET).3dsx: $(TARGET).elf $(TARGET).smdh
> $(DEVKITPRO)/tools/bin/3dsxtool $(TARGET).elf $@ --smdh=$(TARGET).smdh

clean:
> rm -f *.elf *.smdh *.3dsx
