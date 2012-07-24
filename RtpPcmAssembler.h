#ifndef RTPPCMASSEMBLER_H_
#define RTPPCMASSEMBLER_H_

#include "MediaAssembler.h"
#include "android/util/List.h"

namespace android {
namespace os {
class Message;
}
namespace util {
class Buffer;
}
}

using android::os::sp;

class RtpPcmAssembler :
	public MediaAssembler
{
public:
	RtpPcmAssembler(android::util::List< sp<android::util::Buffer> >& queue, const sp<android::os::Message>& notifyAccessUnit);
	virtual ~RtpPcmAssembler();

	virtual void processMediaData();

private:
	android::util::List< sp<android::util::Buffer> >& mQueue;
	sp<android::os::Message> mNotifyAccessUnit;
	sp<android::util::Buffer> mAccessUnit;
	int mAccessUnitOffset;

	bool mStartStream;
	size_t mLastRtpPacketSize;
};

#endif /* RTPPCMASSEMBLER_H_ */
