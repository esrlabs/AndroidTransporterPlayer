#ifndef RTPMEDIASOURCE_H_
#define RTPMEDIASOURCE_H_

#include "mindroid/os/Handler.h"
#include "mindroid/os/Thread.h"
#include "mindroid/util/List.h"

class MediaAssembler;
namespace mindroid {
class Message;
class Buffer;
class DatagramSocket;
}

using mindroid::sp;

class RtpMediaSource :
	public mindroid::Handler
{
public:
	RtpMediaSource(uint16_t port);
	virtual ~RtpMediaSource();

	bool start(sp<MediaAssembler> mediaAssembler);
	void stop();

	virtual void handleMessage(const sp<mindroid::Message>& message);

	sp< mindroid::List< sp<mindroid::Buffer> > > getMediaQueue() { return mQueue; }

private:
	class NetReceiver :
		public mindroid::Thread
	{
	public:
		NetReceiver(uint16_t port, sp<mindroid::Message> notifyRtpPacket, sp<mindroid::Message> notifyRtcpPacket);
		virtual void run();
		void stop();

	private:
		static const uint32_t MAX_UDP_PACKET_SIZE = 65536;

		sp<mindroid::DatagramSocket> mRtpSocket;
		sp<mindroid::DatagramSocket> mRtcpSocket;
		sp<mindroid::Message> mNotifyRtpPacket;
		sp<mindroid::Message> mNotifyRtcpPacket;
		int mPipe[2];

		NO_COPY_CTOR_AND_ASSIGNMENT_OPERATOR(NetReceiver)
	};

	static const uint32_t NOTIFY_RTP_PACKET = 0;
	static const uint32_t NOTIFY_RTCP_PACKET = 1;
	static const uint32_t RTP_HEADER_SIZE = 12;
	static const uint8_t EXT_HEADER_BIT = 1 << 4;
	static const uint8_t PADDING_BIT = 1 << 5;
	static const uint8_t MARK_BIT = 1 << 7;

	int parseRtpHeader(const sp<mindroid::Buffer>& buffer);
	void processRtpPayload(const sp<mindroid::Buffer>& buffer);

	sp<NetReceiver> mNetReceiver;
	sp< mindroid::List< sp<mindroid::Buffer> > > mQueue;
	sp<MediaAssembler> mMediaAssembler;
	uint32_t mRtpPacketCounter;
	uint32_t mHighestSeqNumber;

	NO_COPY_CTOR_AND_ASSIGNMENT_OPERATOR(RtpMediaSource)
};

#endif /* RTPMEDIASOURCE_H_ */
