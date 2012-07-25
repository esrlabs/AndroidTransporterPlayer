#ifndef AVCMEDIAASSEMBLER_H_
#define AVCMEDIAASSEMBLER_H_

#include "MediaAssembler.h"
#include "mindroid/util/List.h"

namespace mindroid {
class Message;
class Buffer;
}

using mindroid::sp;

class AvcMediaAssembler :
	public MediaAssembler
{
public:
	enum Status {
		OK,
		PACKET_FAILURE,
		SEQ_NUMBER_FAILURE,
	};

	AvcMediaAssembler(sp< mindroid::List< sp<mindroid::Buffer> > > queue, const sp<mindroid::Message>& notifyAccessUnit);
	virtual ~AvcMediaAssembler();

	virtual void processMediaQueue();

private:
	static const uint8_t F_BIT = 1 << 7;
	static const uint8_t FU_START_BIT = 1 << 7;
	static const uint8_t FU_END_BIT = 1 << 6;
	static const uint64_t TIME_PERIOD_20MS = 20000000LL;

	Status assembleNalUnits();
	void processSingleNalUnit(sp<mindroid::Buffer> nalUnit);
	Status processFragNalUnit();

	sp< mindroid::List< sp<mindroid::Buffer> > > mQueue;
	sp<mindroid::Message> mNotifyAccessUnit;
	uint32_t mSeqNumber;
	bool mInitSeqNumber;
	uint64_t mFirstSeqNumberFailureTime;
};

#endif /* AVCMEDIAASSEMBLER_H_ */
