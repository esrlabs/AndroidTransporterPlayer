#ifndef RTPMEDIASOURCE_H_
#define RTPMEDIASOURCE_H_

#include "android/os/Ref.h"
#include "android/os/Lock.h"
#include "android/os/CondVar.h"
#include "android/os/Handler.h"
#include "List.h"
#include "Buffer.h"
#include "MediaSourceType.h"

class RtpAvcAssembler;
namespace android {
namespace net {
class DatagramSocket;
}
}

class RtpMediaSource :
	public android::os::Handler {
public:
	static const int32_t POLL_RTP_MEDIA_SOURCE = 0;

	RtpMediaSource(MediaSourceType type, const android::os::sp<android::os::Handler>& player);
	virtual ~RtpMediaSource();

	virtual void handleMessage(const android::os::sp<android::os::Message>& message);

	void pollMediaSource();
	void onPollMediaSource();

	void enqueueAccessUnit(const android::os::sp<Buffer>& accessUnit);
	int32_t dequeueAccessUnit(android::os::sp<Buffer>* accessUnit);

	bool empty() { return mAccessUnits.empty(); }

private:
	android::os::Lock mCondVarLock;
	android::os::CondVar mCondVar;
	MediaSourceType mType;
	const android::os::sp<android::os::Handler> mPlayer;
	List< android::os::sp<Buffer> > mAccessUnits;
	android::os::sp<RtpAvcAssembler> mAssembler;
	android::os::sp<android::net::DatagramSocket> mRtpSocket;
	android::os::sp<android::net::DatagramSocket> mRtcpSocket;

	NO_COPY_CTOR_AND_ASSIGNMENT_OPERATOR(RtpMediaSource)
};

#endif /* RTPMEDIASOURCE_H_ */
