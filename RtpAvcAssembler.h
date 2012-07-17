#ifndef RTPAVCASSEMBLER_H_
#define RTPAVCASSEMBLER_H_

#include "android/os/Ref.h"
#include "List.h"

class Buffer;
namespace android {
namespace os {
class Message;
}
}

using android::os::sp;

class RtpAvcAssembler :
	public android::os::Ref {
public:
	enum Status {
		OK,
		PACKET_FAILURE,
		SEQ_NUMBER_FAILURE,
	};

	RtpAvcAssembler(List< sp<Buffer> >& queue, const sp<android::os::Message>& accessUnitNotifyMessage);
	virtual ~RtpAvcAssembler();

	void processMediaData();

private:
	static const uint8_t F_BIT = 1 << 7;
	static const uint8_t FU_START_BIT = 1 << 7;
	static const uint8_t FU_END_BIT = 1 << 6;
	static const uint64_t TIME_PERIOD_20MS = 20000000LL;

	Status assembleMediaData();
	void processSingleNalUnit(sp<Buffer> nalUnit);
	Status processFragNalUnit();

	List< sp<Buffer> >& mQueue;
	sp<android::os::Message> mAccessUnitNotifyMessage;
	uint32_t mSeqNumber;
	bool mInitSeqNumber;
	uint64_t mFirstSeqNumberFailureTime;
};

#endif /* RTPAVCASSEMBLER_H_ */
