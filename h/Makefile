TARGET = h
OBJS = main.o

CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LDFLAGS = -nostartfiles -T linkfile.x -nostdlib

PSPSDK = $(shell psp-config --pspsdk-path)
include build_bin.mak