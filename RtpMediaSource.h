#ifndef RTPMEDIASOURCE_H_
#define RTPMEDIASOURCE_H_

#include "android/os/Thread.h"
#include "android/os/Handler.h"
#include "android/os/Lock.h"
#include "android/os/CondVar.h"
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
	public android::os::Thread {
public:
	RtpMediaSource(MediaSourceType type, const android::os::sp<android::os::Handler>& player);
	virtual ~RtpMediaSource();

	virtual void run();

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
