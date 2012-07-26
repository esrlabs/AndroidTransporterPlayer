#ifndef PCMMEDIAASSEMBLER_H_
#define PCMMEDIAASSEMBLER_H_

#include "MediaAssembler.h"
#include <mindroid/util/List.h>

namespace mindroid {
class Message;
class Buffer;
}

using mindroid::sp;

class PcmMediaAssembler :
	public MediaAssembler
{
public:
	PcmMediaAssembler(sp< mindroid::List< sp<mindroid::Buffer> > > queue, const sp<mindroid::Message>& notifyAccessUnit);
	virtual ~PcmMediaAssembler();

	virtual void processMediaQueue();

private:
	void newAccessUnit();
	void appendToAccessUnit(sp<mindroid::Buffer>& buffer);
	void accessUnitFinished();
	bool sequenceStarted();

	sp< mindroid::List< sp<mindroid::Buffer> > > mQueue;
	sp<mindroid::Message> mNotifyAccessUnit;
	sp<mindroid::Buffer> mAccessUnit;
	int mAccessUnitOffset;

	static const uint32_t ILLEGAL_SEQUENCE_NUMBER = 1 << 17;
	bool mStartStream;
	size_t mLastRtpPacketSize;
	uint32_t mLastSequence;
};

#endif /* PCMMEDIAASSEMBLER_H_ */
