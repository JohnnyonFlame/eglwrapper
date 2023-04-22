PVR_PATH?=.
EGL_PATH?=/usr/lib/arm-linux-gnueabihf/
CROSS?=arm-linux-gnueabihf-
CXXFLAGS?=-O2
CFLAGS?=-O2
CXX=$(CROSS)g++
CC=$(CROSS)gcc

.PHONY: clean build

all: build build/libEGL.so build/eglconfig.elf

build:
	@mkdir -p build

build/libEGL.so: eglwrap.cpp
	$(CXX) $(CXXFLAGS) -shared $< -o $@ -lIMGegl -lsrv_um -L $(PVR_PATH)

build/eglconfig.elf: eglconfig.c
	$(CC) $(CFLAGS) $< -o $@ -lEGL -ldl -L $(EGL_PATH)

clean:
	rm -rf build