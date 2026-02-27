CURDIR := $(shell pwd)

include $(CURDIR)/build/arm-hisiv300-linux.mk
include $(CURDIR)/build/sdk_path.mk

TARGET  = ipcamera
OUTDIR  = $(CURDIR)/output

SRC_DIRS = \
    src/common \
    src/mpp \
    src/record \
    src/motion \
    src/config

C_SRCS   := $(foreach d,$(SRC_DIRS),$(wildcard $(d)/*.c))
CXX_SRCS := $(wildcard src/rtsp/*.cpp)
C_SRCS   += src/main.c

C_OBJS   := $(patsubst %.c,$(OUTDIR)/%.o,$(C_SRCS))
CXX_OBJS := $(patsubst %.cpp,$(OUTDIR)/%.o,$(CXX_SRCS))
OBJS     := $(C_OBJS) $(CXX_OBJS)

DEPS     := $(OBJS:.o=.d)

INC_DIRS = \
    src/common \
    src/mpp \
    src/rtsp \
    src/record \
    src/motion \
    src/config \
    include/hisi \
    include/live555

CFLAGS  = $(COMMON_CFLAGS) $(MPP_CFLAGS) $(LIVE555_CFLAGS) \
    $(addprefix -I,$(INC_DIRS))

CXXFLAGS = $(CFLAGS) -std=c++11

LDFLAGS = $(COMMON_LDFLAGS) $(MPP_LDFLAGS) $(LIVE555_LDFLAGS) \
    -lstdc++

.PHONY: all clean install

all: $(OUTDIR)/$(TARGET)

$(OUTDIR)/$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CXX) -o $@ $^ $(LDFLAGS)
	@echo "Built: $@"

$(OUTDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(OUTDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS)

clean:
	rm -rf $(OUTDIR)

install: all
	install -d $(DESTDIR)/usr/bin
	install -m 755 $(OUTDIR)/$(TARGET) $(DESTDIR)/usr/bin/$(TARGET)
	install -d $(DESTDIR)/etc
	install -m 644 src/config/ipcamera.conf $(DESTDIR)/etc/ipcamera.conf
