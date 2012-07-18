#ifndef RTPMEDIASOURCE_H_
#define RTPMEDIASOURCE_H_

#include "android/os/Thread.h"
#include "android/util/List.h"
#include "MediaSourceType.h"

class Buffer;
class RtpAssembler;
namespace android {
namespace os {
class Message;
}
namespace net {
class DatagramSocket;
}
}

using android::os::sp;

class RtpMediaSource :
	public android::os::Thread {
public:
	RtpMediaSource();
	virtual ~RtpMediaSource();

	bool start(uint16_t port, sp<RtpAssembler> assembler);
	void stop();

	virtual void run();

	android::util::List< sp<Buffer> >& getBufferQueue() { return mQueue; }

private:
	static const uint32_t MAX_UDP_PACKET_SIZE = 65536;
	static const uint32_t RTP_HEADER_SIZE = 12;
	static const uint32_t EXT_HEADER_BIT = 1 << 4;
	static const uint32_t PADDING_BIT = 1 << 5;

	bool start();
	bool processMediaData(sp<android::net::DatagramSocket> socket);
	int parseRtpHeader(const sp<Buffer>& buffer);
	void processRtpPayload(const sp<Buffer>& buffer);

	sp<android::net::DatagramSocket> mRtpSocket;
	sp<android::net::DatagramSocket> mRtcpSocket;
	android::util::List< sp<Buffer> > mQueue;
	sp<RtpAssembler> mAssembler;
	uint32_t mRtpPacketCounter;
	uint32_t mHighestSeqNumber;
	MediaSourceType mType;

	NO_COPY_CTOR_AND_ASSIGNMENT_OPERATOR(RtpMediaSource)
};

#endif /* RTPMEDIASOURCE_H_ */
