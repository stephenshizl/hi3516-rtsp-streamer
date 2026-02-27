CROSS_COMPILE ?= arm-hisiv300-linux-

CC  = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
AR  = $(CROSS_COMPILE)ar
LD  = $(CROSS_COMPILE)ld
STRIP = $(CROSS_COMPILE)strip
OBJCOPY = $(CROSS_COMPILE)objcopy

ARCH = arm
TARGET_ABI = -march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=softfp

COMMON_CFLAGS = $(TARGET_ABI) \
    -Wall \
    -Wno-unused-function \
    -Wno-pointer-sign \
    -ffunction-sections \
    -fdata-sections \
    -O2 \
    -g

COMMON_LDFLAGS = -Wl,--gc-sections \
    -lpthread \
    -lm \
    -ldl \
    -lrt
