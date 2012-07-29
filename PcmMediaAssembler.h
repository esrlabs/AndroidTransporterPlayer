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
	PcmMediaAssembler(sp< mindroid::List< sp<mindroid::Buffer> > > queue, const sp<mindroid::Message>& notifyBuffer);
	virtual ~PcmMediaAssembler();

	virtual Status assembleMediaData();
	virtual uint32_t getNextSeqNum() const;

private:
	void swapPcmDataEndianess(const sp<mindroid::Buffer>& buffer);

	sp< mindroid::List< sp<mindroid::Buffer> > > mQueue;
	sp<mindroid::Message> mNotifyBuffer;
};

#endif /* PCMMEDIAASSEMBLER_H_ */
