CFLAGS  := -O2 -D__ARM_CPU_ARCH__ -D__ARMv6_CPU_ARCH__ -I. -I./Mindroid -I/opt/vc/include -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/src/hello_pi/libs/ilclient
LIBS	:= -L/opt/vc/src/hello_pi/libs/ilclient -L/opt/vc/lib -lWFC -lGLESv2 -lEGL -lbcm_host -lopenmaxil -lilclient -lc

SRCS = \
	RtspSocket.cpp \
	RPiPlayer.cpp \
	RtspMediaSource.cpp \
	RtpMediaSource.cpp \
	MediaAssembler.cpp \
	AvcMediaAssembler.cpp \
	PcmMediaAssembler.cpp \
	NetHandler.cpp \
	AndroidTransporterPlayer.cpp \
	Mindroid/mindroid/os/Thread.cpp \
        Mindroid/mindroid/os/Semaphore.cpp \
        Mindroid/mindroid/os/MessageQueue.cpp \
        Mindroid/mindroid/os/Message.cpp \
        Mindroid/mindroid/os/Looper.cpp \
        Mindroid/mindroid/os/Lock.cpp \
        Mindroid/mindroid/os/CondVar.cpp \
        Mindroid/mindroid/os/Handler.cpp \
        Mindroid/mindroid/os/Clock.cpp \
        Mindroid/mindroid/os/AsyncTask.cpp \
        Mindroid/mindroid/os/SerialExecutor.cpp \
        Mindroid/mindroid/os/ThreadPoolExecutor.cpp \
        Mindroid/mindroid/os/AtomicInteger.cpp \
        Mindroid/mindroid/os/Ref.cpp \
        Mindroid/mindroid/os/Bundle.cpp \
        Mindroid/mindroid/util/Buffer.cpp \
        Mindroid/mindroid/lang/String.cpp \
        Mindroid/mindroid/net/SocketAddress.cpp \
        Mindroid/mindroid/net/ServerSocket.cpp \
        Mindroid/mindroid/net/Socket.cpp \
	Mindroid/mindroid/net/DatagramSocket.cpp

OBJS += $(filter %.o,$(SRCS:.cpp=.o))

AndroidTransporterPlayer: $(OBJS)
	$(CXX) -o AndroidTransporterPlayer $(OBJS) $(LIBS)

%.o: %.cpp
	@rm -f $@ 
	$(CXX) $(CFLAGS) -c $< -o $@

clean:
	for i in $(OBJS); do (if test -e "$$i"; then ( rm $$i ); fi ); done
	@rm -f AndroidTransporterPlayer
