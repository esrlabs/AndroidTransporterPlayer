#ifndef RTPMEDIASOURCE_H_
#define RTPMEDIASOURCE_H_

#include "android/os/Handler.h"
#include "android/os/Thread.h"
#include "android/util/List.h"
#include "MediaSourceType.h"

class MediaAssembler;
namespace android {
namespace os {
class Message;
}
namespace util {
class Buffer;
}
namespace net {
class DatagramSocket;
}
}

using android::os::sp;

class RtpMediaSource :
	public android::os::Handler
{
public:
	RtpMediaSource(uint16_t port);
	virtual ~RtpMediaSource();

	bool start(sp<MediaAssembler> mediaAssembler);
	void stop();

	virtual void handleMessage(const sp<android::os::Message>& message);

	android::util::List< sp<android::util::Buffer> >& getMediaQueue() { return mQueue; }

private:
	class NetReceiver :
		public android::os::Thread
	{
	public:
		NetReceiver(uint16_t port, sp<android::os::Message> notifyRtpPacket);
		virtual void run();
		void stop();

	private:
		static const uint32_t MAX_UDP_PACKET_SIZE = 65536;

		sp<android::net::DatagramSocket> mRtpSocket;
		sp<android::net::DatagramSocket> mRtcpSocket;
		sp<android::os::Message> mNotifyRtpPacket;
		int mPipe[2];

		NO_COPY_CTOR_AND_ASSIGNMENT_OPERATOR(NetReceiver)
	};

	static const uint32_t NOTIFY_RTP_PACKET = 0;
	static const uint32_t RTP_HEADER_SIZE = 12;
	static const uint32_t EXT_HEADER_BIT = 1 << 4;
	static const uint32_t PADDING_BIT = 1 << 5;

	int parseRtpHeader(const sp<android::util::Buffer>& buffer);
	void processRtpPayload(const sp<android::util::Buffer>& buffer);

	sp<NetReceiver> mNetReceiver;
	android::util::List< sp<android::util::Buffer> > mQueue;
	sp<MediaAssembler> mMediaAssembler;
	uint32_t mRtpPacketCounter;
	uint32_t mHighestSeqNumber;
	MediaSourceType mType;

	NO_COPY_CTOR_AND_ASSIGNMENT_OPERATOR(RtpMediaSource)
};

#endif /* RTPMEDIASOURCE_H_ */
