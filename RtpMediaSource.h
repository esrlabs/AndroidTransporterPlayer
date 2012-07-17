#ifndef RTPMEDIASOURCE_H_
#define RTPMEDIASOURCE_H_

#include "android/os/Thread.h"
#include "List.h"
#include "MediaSourceType.h"

class Buffer;
class RtpAvcAssembler;
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

	virtual void run();

	void start(uint16_t port,
			const sp<android::os::Message>& reply);
	void stop();

private:
	static const uint32_t RTP_HEADER_SIZE = 12;
	static const uint32_t EXT_HEADER_BIT = 1 << 4;
	static const uint32_t PADDING_BIT = 1 << 5;

	bool start();
	bool processMediaData(sp<android::net::DatagramSocket> socket);
	int parseRtpHeader(const sp<Buffer>& buffer);
	void processRtpPayload(const sp<Buffer>& buffer);

	sp<android::net::DatagramSocket> mRtpSocket;
	sp<android::net::DatagramSocket> mRtcpSocket;
	MediaSourceType mType;
	List< sp<Buffer> > mQueue;
	sp<RtpAvcAssembler> mAssembler;
	uint32_t mRtpPacketCounter;
	uint32_t mHighestSeqNumber;

	NO_COPY_CTOR_AND_ASSIGNMENT_OPERATOR(RtpMediaSource)
};

#endif /* RTPMEDIASOURCE_H_ */
