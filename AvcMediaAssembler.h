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
	AvcMediaAssembler(sp< mindroid::List< sp<mindroid::Buffer> > > queue, const sp<mindroid::Message>& notifyAccessUnit);
	virtual ~AvcMediaAssembler();

	virtual Status assembleMediaData();
	virtual uint32_t getNextSeqNum() const;

private:
	static const uint8_t F_BIT = 1 << 7;
	static const uint8_t FU_START_BIT = 1 << 7;
	static const uint8_t FU_END_BIT = 1 << 6;

	void processSingleNalUnit(sp<mindroid::Buffer> nalUnit);
	Status processFragNalUnit();

	sp< mindroid::List< sp<mindroid::Buffer> > > mQueue;
	sp<mindroid::Message> mNotifyAccessUnit;
};

#endif /* AVCMEDIAASSEMBLER_H_ */
