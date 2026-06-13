# Makefile for Pacman with Sokol Graphics

CC = clang
CFLAGS = -std=c99 -Wall -Wextra -O2 -I./sokol
OBJCFLAGS = -ObjC -fobjc-arc -O2 -I./sokol
LDFLAGS = -framework Metal -framework Cocoa -framework MetalKit -framework Quartz -framework AudioToolbox

C_SOURCES = pacman.c graphic.c rom_data.c
C_OBJECTS = $(C_SOURCES:.c=.o)
OBJC_SOURCES = sokol/sokol.c
OBJC_OBJECTS = $(OBJC_SOURCES:.c=.o)

ALL_OBJECTS = $(C_OBJECTS) $(OBJC_OBJECTS)
TARGET = pacman

all: $(TARGET)

$(TARGET): $(ALL_OBJECTS)
	$(CC) $(ALL_OBJECTS) -o $(TARGET) $(LDFLAGS)

$(C_OBJECTS): %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJC_OBJECTS): %.o: %.c
	$(CC) $(OBJCFLAGS) -c $< -o $@

clean:
	rm -f $(ALL_OBJECTS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
