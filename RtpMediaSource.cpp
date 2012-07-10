#include "RtpMediaSource.h"
#include "RtpAvcAssembler.h"
#include "android/os/Message.h"
#include "RPiPlayer.h"

using namespace android::os;

RtpMediaSource::RtpMediaSource(MediaSourceType type, Handler& player) :
	mCondVar(mCondVarLock),
	mType(type),
	mPlayer(player) {
	mAssembler = new RtpAvcAssembler(this);
}

RtpMediaSource::~RtpMediaSource() {
}

void RtpMediaSource::handleMessage(Message& message) {
	switch (message.what) {
	case POLL_RTP_MEDIA_SOURCE:
		onPollMediaSource();
		break;
	}
}

void RtpMediaSource::pollMediaSource() {
	obtainMessage(POLL_RTP_MEDIA_SOURCE).sendToTarget();
}

void RtpMediaSource::onPollMediaSource() {
	pollMediaSource();
}

void RtpMediaSource::enqueueAccessUnit(const android::os::sp<Buffer>& accessUnit) {
	AutoLock autoLock(mCondVarLock);
	mAccessUnits.push_back(accessUnit);
	mCondVar.notify();

	mPlayer.obtainMessage(RPiPlayer::MEDIA_SOURCE_NOTIFY, mType, 0).sendToTarget();
}

int32_t RtpMediaSource::dequeueAccessUnit(android::os::sp<Buffer>* accessUnit) {
	AutoLock autoLock(mCondVarLock);

	while (mAccessUnits.empty()) {
		mCondVar.wait();
	}

	accessUnit = &(*mAccessUnits.begin());
	mAccessUnits.erase(mAccessUnits.begin());

	return 0;
}
