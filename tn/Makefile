TARGET = tn
OBJS = main.o utils.o ../rebootex/rebootex.o libc.o

CFLAGS = -O2 -Os -G0 -Wall -fno-pic
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LDFLAGS = -nostartfiles -T linkfile.x -nostdlib

PSPSDK = $(shell psp-config --pspsdk-path)
SCESDK = $(shell psp-config --pspdev-path)
include build_bin.mak