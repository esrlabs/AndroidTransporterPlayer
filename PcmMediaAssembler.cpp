#include "PcmMediaAssembler.h"
#include <mindroid/os/Message.h>
#include <mindroid/util/Buffer.h>
#include <mindroid/os/Clock.h>
#include <string.h>
#include <stdio.h>

using namespace mindroid;

static const size_t FRAMES_PER_UNIT = 2205;
static const size_t BYTES_PER_FRAME = 4;
static const size_t UNIT_SIZE = FRAMES_PER_UNIT * BYTES_PER_FRAME;
static const size_t RTP_PACKET_SMALL_SIZE = 492;
static const size_t RTP_PACKET_LARGE_SIZE = 1388;


PcmMediaAssembler::PcmMediaAssembler(sp< List< sp<Buffer> > > queue, const sp<Message>& notifyAccessUnit) :
	mQueue(queue),
	mNotifyAccessUnit(notifyAccessUnit),
	mAccessUnit(NULL),
	mAccessUnitOffset(0),
	mStartStream(false),
	mLastRtpPacketSize(0),
	mLastSequence(ILLEGAL_SEQUENCE_NUMBER) {
	}

PcmMediaAssembler::~PcmMediaAssembler() {
}

void PcmMediaAssembler::newAccessUnit() {
	mAccessUnit = new Buffer(UNIT_SIZE);
	mAccessUnitOffset = 0;
}

void PcmMediaAssembler::appendToAccessUnit(sp<Buffer>& buffer) {
	if (0 == mAccessUnit.getPointer()) {
		return;
	}

	assert(buffer->size() % 4 == 0);
	assert(mAccessUnit->capacity() >= mAccessUnitOffset + buffer->size());

	uint16_t* source = (uint16_t*)(buffer->data());
	uint16_t* dest = (uint16_t*)(mAccessUnit->data() + mAccessUnitOffset);

	for (size_t i = 0; i < buffer->size(); i += 2) {
		*dest = (*source >> 8) | ((*source	<< 8) & 0xFF00);
		dest++;
		source++;
		mAccessUnitOffset += 2;
	}
}

void PcmMediaAssembler::accessUnitFinished() {
	if (0 == mAccessUnit.getPointer()) {
		return;
	}

	sp<Message> msg = mNotifyAccessUnit->dup();
	msg->obj = new sp<Buffer>(mAccessUnit);

	static uint64_t last = 0;
	uint64_t now = Clock::realTime();
	if (last != 0) {
		int64_t deltaInUs = (now-last) / 1000;
		if ((deltaInUs > 55000) || (deltaInUs < 45000)) {
			printf("irregular delta in pcmassembler: %lld\n", deltaInUs);
		}
	}
	last = now;

	assert(msg->sendToTarget());
}

bool PcmMediaAssembler::sequenceStarted() {
	return mLastSequence != ILLEGAL_SEQUENCE_NUMBER;
}

void PcmMediaAssembler::processMediaQueue() {
	if (mQueue->empty()) {
		return;
	}

	sp<Buffer> buffer = *mQueue->begin();
	mQueue->erase(mQueue->begin());

	uint16_t seqNum = 0;
	bool mark = false;
	assert(buffer->mBundle.fillUInt16("seqNum", &seqNum));
	assert(buffer->mBundle.fillBool("mark", &mark));

	if (sequenceStarted()) {
		if (mLastSequence+1 == seqNum) {
			appendToAccessUnit(buffer);
		} else {
			mAccessUnit = 0;
		}

		if (mark) {
			accessUnitFinished();
		}
	}

	if (mark) {
		newAccessUnit();
	}

	mLastSequence = seqNum;
}
