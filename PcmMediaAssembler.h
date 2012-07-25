#ifndef PCMMEDIAASSEMBLER_H_
#define PCMMEDIAASSEMBLER_H_

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

class PcmMediaAssembler :
	public MediaAssembler
{
public:
	PcmMediaAssembler(sp< android::util::List< sp<android::util::Buffer> > > queue, const sp<android::os::Message>& notifyAccessUnit);
	virtual ~PcmMediaAssembler();

	virtual void processMediaQueue();

private:
	sp< android::util::List< sp<android::util::Buffer> > > mQueue;
	sp<android::os::Message> mNotifyAccessUnit;
	sp<android::util::Buffer> mAccessUnit;
	int mAccessUnitOffset;

	bool mStartStream;
	size_t mLastRtpPacketSize;
};

#endif /* PCMMEDIAASSEMBLER_H_ */
