#ifndef MEDIAASSEMBLER_H_
#define MEDIAASSEMBLER_H_

#include "mindroid/os/Ref.h"

class MediaAssembler :
	public mindroid::Ref
{
public:
	enum Status {
		OK,
		PACKET_FAILURE,
		SEQ_NUMBER_FAILURE,
	};

	static const uint64_t TIME_PERIOD_20MS = 20000000LL;

	MediaAssembler();
	virtual ~MediaAssembler() { }
	virtual Status assembleMediaData() = 0;
	virtual uint32_t getNextSeqNum() const = 0;
	void processMediaQueue();

protected:
	uint32_t mSeqNumber;
	bool mInitSeqNumber;

private:
	uint64_t mFirstSeqNumberFailureTime;
};

#endif /* MEDIAASSEMBLER_H_ */
