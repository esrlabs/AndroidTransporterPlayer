include Makefile.include

CFLAGS+=-DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -DHAVE_CMAKE_CONFIG -D__VIDEOCORE4__ -U_FORTIFY_SOURCE -Wall -mfpu=vfp -mfloat-abi=softfp -mno-apcs-stack-check -DHAVE_OMXLIB -DUSE_EXTERNAL_FFMPEG  -DHAVE_LIBAVCODEC_AVCODEC_H -DHAVE_LIBAVUTIL_MEM_H -DHAVE_LIBAVUTIL_AVUTIL_H -DHAVE_LIBAVFORMAT_AVFORMAT_H -DHAVE_LIBAVFILTER_AVFILTER_H -DOMX -DOMX_SKIP64BIT -ftree-vectorize -pipe -DUSE_EXTERNAL_OMX -DHAVE_PLATFORM_RASPBERRY_PI -DUSE_EXTERNAL_LIBBCM_HOST -Wno-psabi -I$(SDKSTAGE)/opt/vc/include/ 

LDFLAGS+=-L./ -lc -lWFC -lGLESv2 -lEGL -lbcm_host -lopenmaxil -lilclient
INCLUDES+=-I./

SRC=Socket.cpp \
	DatagramSocket.cpp \
	AndroidTransporterPlayer.cpp \

OBJS+=$(filter %.o,$(SRC:.cpp=.o))

all: AndroidTransporterPlayer

%.o: %.cpp
	@rm -f $@ 
	$(CXX) $(CFLAGS) $(INCLUDES) -c $< -o $@ -Wno-deprecated-declarations

AndroidTransporterPlayer: $(OBJS)
	$(CXX) $(LDFLAGS) -o AndroidTransporterPlayer -Wl,--whole-archive $(OBJS) -Wl,--no-whole-archive -rdynamic -lilclient
