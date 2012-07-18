#ifndef RTPPCMASSEMBLER_H_
#define RTPPCMASSEMBLER_H_

#include "RtpAssembler.h"
#include "android/util/List.h"

class Buffer;
namespace android {
namespace os {
class Message;
}
}

using android::os::sp;

class RtpPcmAssembler :
	public RtpAssembler
{
public:
	RtpPcmAssembler(android::util::List< sp<Buffer> >& queue, const sp<android::os::Message>& notifyAccessUnit);
	virtual ~RtpPcmAssembler();

	virtual void processMediaData();

private:
	android::util::List< sp<Buffer> >& mQueue;
	sp<android::os::Message> mNotifyAccessUnit;
};

#endif /* RTPPCMASSEMBLER_H_ */
