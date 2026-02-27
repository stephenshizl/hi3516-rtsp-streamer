SDK_PATH ?= /opt/Hi3516CV300_SDK_V2.0.4.0

MPP_PATH     = $(SDK_PATH)/mpp
MPP_INC_PATH = $(MPP_PATH)/include
MPP_LIB_PATH = $(MPP_PATH)/out/lib

SENSOR_TYPE  ?= IMX335_MIPI_5M_30FPS

LIVE555_PATH     = $(CURDIR)/lib/live555
LIVE555_INC_PATH = $(CURDIR)/include/live555
LIVE555_LIB_PATH = $(LIVE555_PATH)

HISI_LIBS = -lmpi -lVoiceEngine -lresampler -laec -lans -lupvqe -ldnvqe
MPP_LDFLAGS = -L$(MPP_LIB_PATH) $(HISI_LIBS)
MPP_CFLAGS  = -I$(MPP_INC_PATH)

LIVE555_LIBS = -lliveMedia -lgroupsock -lBasicUsageEnvironment -lUsageEnvironment
LIVE555_LDFLAGS = -L$(LIVE555_LIB_PATH) $(LIVE555_LIBS)
LIVE555_CFLAGS  = -I$(LIVE555_INC_PATH)
