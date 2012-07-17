BUILDROOT	:= ..
TOOLCHAIN	:= $(BUILDROOT)/tools/arm-bcm2708/linux-x86
SYSROOT		:= $(BUILDROOT)/rootfs
CC			:= $(TOOLCHAIN)/bin/arm-bcm2708-linux-gnueabi-gcc --sysroot=$(SYSROOT)
CXX         := $(TOOLCHAIN)/bin/arm-bcm2708-linux-gnueabi-g++ --sysroot=$(SYSROOT)

CFLAGS		:= -Isystem$(PREFIX)/include -I$(SYSROOT)/opt/vc/include/ -I./ -I$(SYSROOT)/opt/vc/src/hello_pi/libs -DHAVE_LIBOPENMAX=2 -DOMX -DOMX_SKIP64BIT -ftree-vectorize -pipe -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM -Wno-psabi
CXXFLAGS	:= $(CFLAGS)
CPPFLAGS	:= $(CFLAGS)
LDFLAGS		:= -L$(BUILDROOT)/lib -L$(SYSROOT)/opt/vc/lib/ -L$(SYSROOT)/opt/vc/src/hello_pi/libs -lGLESv2 -lEGL -lopenmaxil -lbcm_host -lilclient

CFLAGS 		+= -DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -DHAVE_CMAKE_CONFIG -D__VIDEOCORE4__ -U_FORTIFY_SOURCE -Wall -mfpu=vfp -mfloat-abi=softfp -mno-apcs-stack-check -DHAVE_OMXLIB -DUSE_EXTERNAL_FFMPEG  -DHAVE_LIBAVCODEC_AVCODEC_H -DHAVE_LIBAVUTIL_MEM_H -DHAVE_LIBAVUTIL_AVUTIL_H -DHAVE_LIBAVFORMAT_AVFORMAT_H -DHAVE_LIBAVFILTER_AVFILTER_H -DOMX -DOMX_SKIP64BIT -ftree-vectorize -pipe -DUSE_EXTERNAL_OMX -DHAVE_PLATFORM_RASPBERRY_PI -DUSE_EXTERNAL_LIBBCM_HOST -Wno-psabi -I$(SDKSTAGE)/opt/vc/include/ 
CFLAGS		+= -D__ARM_CPU_ARCH__ -D__ARMv6_CPU_ARCH__
LDFLAGS		+= -L./ -lc -lWFC -lGLESv2 -lEGL -lbcm_host -lopenmaxil -lilclient -lAndroidNativePlatform
INCLUDES	+= -I./ -I../AndroidNativePlatform/include

SRCS = RtspSocket.cpp \
	Buffer.cpp \
	RPiPlayer.cpp \
	RtspMediaSource.cpp \
	RtpMediaSource.cpp \
	RtpAvcAssembler.cpp \
	NetHandler.cpp \
	Bundle.cpp \
	AndroidTransporterPlayer.cpp \

OBJS += $(filter %.o,$(SRCS:.cpp=.o))

all: AndroidTransporterPlayer

%.o: %.cpp
	@rm -f $@ 
	$(CXX) $(CFLAGS) $(INCLUDES) -c $< -o $@

AndroidTransporterPlayer: $(OBJS)
	$(CXX) $(LDFLAGS) -o AndroidTransporterPlayer -Wl,--whole-archive $(OBJS) -Wl,--no-whole-archive -rdynamic -lilclient

clean:
	for i in $(OBJS); do (if test -e "$$i"; then ( rm $$i ); fi ); done
	@rm -f AndroidTransporterPlayer
