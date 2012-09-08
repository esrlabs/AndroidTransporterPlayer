BUILDROOT := ..
TOOLCHAIN := $(BUILDROOT)/tools/arm-bcm2708/x86-linux64-cross-arm-linux-hardfp
SYSROOT	  := $(BUILDROOT)/tools/arm-bcm2708/x86-linux64-cross-arm-linux-hardfp/arm-bcm2708hardfp-linux-gnueabi/sys-root
ROOTFS    := $(BUILDROOT)/rootfs
CC		  := $(TOOLCHAIN)/bin/arm-bcm2708hardfp-linux-gnueabi-gcc --sysroot=$(SYSROOT)
CXX       := $(TOOLCHAIN)/bin/arm-bcm2708hardfp-linux-gnueabi-g++ --sysroot=$(SYSROOT)
LD        := $(TOOLCHAIN)/bin/arm-bcm2708hardfp-linux-gnueabi-g++ --sysroot=$(SYSROOT)
CFLAGS    := -D__ARM_CPU_ARCH__ -D__ARMv6_CPU_ARCH__ -march=armv6 -mfpu=vfp -mfloat-abi=hard -O2
LDFLAGS   := 
LIBS      := -L$(ROOTFS)/opt/vc/lib -L$(ROOTFS)/opt/vc/src/hello_pi/libs/ilclient -L../Mindroid -L../fdk-aac -lGLESv2 -lEGL -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -lilclient -lmindroid -laac

SRCS      := $(wildcard *.cpp)
OBJS      := $(patsubst %.cpp,out/%.o,$(SRCS))
INCLUDES  := -I. -I../Mindroid -I$(ROOTFS)/opt/vc/include/ -I./ -I$(ROOTFS)/opt/vc/src/hello_pi/libs -I$(ROOTFS)/opt/vc/include/interface/vcos/pthreads -I$(ROOTFS)/opt/vc/src/hello_pi/libs/ilclient -I../fdk-aac/libAACdec/include -I../fdk-aac/libAACenc/include -I../fdk-aac/libPCMutils/include -I../fdk-aac/libFDK/include -I../fdk-aac/libSYS/include -I../fdk-aac/libMpegTPDec/include -I../fdk-aac/libMpegTPEnc/include -I../fdk-aac/libSBRdec/include -I../fdk-aac/libSBRenc/include
BUILD_DIR := out/

out/%.o: %.cpp
	$(CXX) $(CFLAGS) $(INCLUDES) -c $< -o $@

.PHONY: all checkdirs clean

all: checkdirs AndroidTransporterPlayer

AndroidTransporterPlayer: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@ $(LIBS)

checkdirs: $(BUILD_DIR)

$(BUILD_DIR):
	@mkdir -p $@

clean:
	@rm -rf $(BUILD_DIR)

