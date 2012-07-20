#ifndef RTPAVCASSEMBLER_H_
#define RTPAVCASSEMBLER_H_

#include "RtpAssembler.h"
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

class RtpAvcAssembler :
	public RtpAssembler
{
public:
	enum Status {
		OK,
		PACKET_FAILURE,
		SEQ_NUMBER_FAILURE,
	};

	RtpAvcAssembler(android::util::List< sp<android::util::Buffer> >& queue, const sp<android::os::Message>& notifyAccessUnit);
	virtual ~RtpAvcAssembler();

	virtual void processMediaQueue();

private:
	static const uint8_t F_BIT = 1 << 7;
	static const uint8_t FU_START_BIT = 1 << 7;
	static const uint8_t FU_END_BIT = 1 << 6;
	static const uint64_t TIME_PERIOD_20MS = 20000000LL;

	Status assembleNalUnits();
	void processSingleNalUnit(sp<android::util::Buffer> nalUnit);
	Status processFragNalUnit();

	android::util::List< sp<android::util::Buffer> >& mQueue;
	sp<android::os::Message> mNotifyAccessUnit;
	uint32_t mSeqNumber;
	bool mInitSeqNumber;
	uint64_t mFirstSeqNumberFailureTime;
};

#endif /* RTPAVCASSEMBLER_H_ */
