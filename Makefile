BUILDROOT	:= ..
TOOLCHAIN	:= $(BUILDROOT)/tools/arm-bcm2708/x86-linux64-cross-arm-linux-hardfp
ROOTFS		:= $(BUILDROOT)/rootfs
SYSROOT		:= $(BUILDROOT)/tools/arm-bcm2708/x86-linux64-cross-arm-linux-hardfp/arm-bcm2708hardfp-linux-gnueabi/sys-root
CC			:= $(TOOLCHAIN)/bin/arm-bcm2708hardfp-linux-gnueabi-gcc --sysroot=$(SYSROOT)
CXX         := $(TOOLCHAIN)/bin/arm-bcm2708hardfp-linux-gnueabi-g++ --sysroot=$(SYSROOT)
# --sysroot=dir
# Use dir as the logical root directory for headers and libraries.
# For example, if the compiler normally searches for headers in /usr/include and libraries in /usr/lib, it instead searches dir/usr/include and dir/usr/lib.
# If you use both this option and the -isysroot option, then the --sysroot option applies to libraries, but the -isysroot option applies to header files

CFLAGS		:= -I$(ROOTFS)/opt/vc/include/ -I./ -I$(ROOTFS)/opt/vc/src/hello_pi/libs -I$(ROOTFS)/opt/vc/include/interface/vcos/pthreads -I$(ROOTFS)/opt/vc/src/hello_pi/libs/ilclient/ -DHAVE_LIBOPENMAX=2 -DOMX -DOMX_SKIP64BIT -ftree-vectorize -pipe -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM -Wno-psabi
CXXFLAGS	:= $(CFLAGS)
CPPFLAGS	:= $(CFLAGS)
LDFLAGS		:= -L$(ROOTFS)/lib -L$(ROOTFS)/opt/vc/lib/ -L$(ROOTFS)/opt/vc/src/hello_pi/libs/ilclient -lGLESv2 -lEGL -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -lilclient -lmindroid

CFLAGS 		+= -march=armv6 -mfpu=vfp -mfloat-abi=hard -DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -DHAVE_CMAKE_CONFIG -D__VIDEOCORE4__ -U_FORTIFY_SOURCE -Wall -mno-apcs-stack-check -DHAVE_OMXLIB -DUSE_EXTERNAL_FFMPEG  -DHAVE_LIBAVCODEC_AVCODEC_H -DHAVE_LIBAVUTIL_MEM_H -DHAVE_LIBAVUTIL_AVUTIL_H -DHAVE_LIBAVFORMAT_AVFORMAT_H -DHAVE_LIBAVFILTER_AVFILTER_H -DOMX -DOMX_SKIP64BIT -ftree-vectorize -pipe -DUSE_EXTERNAL_OMX -DHAVE_PLATFORM_RASPBERRY_PI -DUSE_EXTERNAL_LIBBCM_HOST -Wno-psabi

CFLAGS		+= -O2 -D__ARM_CPU_ARCH__ -D__ARMv6_CPU_ARCH__
LDFLAGS		+= -L./ -lc -lWFC -lGLESv2 -lEGL -lbcm_host -lopenmaxil -lilclient -lmindroid
INCLUDES	+= -I./ -I../Mindroid

SRCS = RtspSocket.cpp \
	RPiPlayer.cpp \
	RtspMediaSource.cpp \
	RtpMediaSource.cpp \
	AvcMediaAssembler.cpp \
	PcmMediaAssembler.cpp \
	NetHandler.cpp \
	rtcp/Rtcp.cpp \
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
