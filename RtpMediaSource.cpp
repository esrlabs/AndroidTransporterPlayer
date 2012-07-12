#include "RtpMediaSource.h"
#include "RtpAvcAssembler.h"
#include "android/os/Message.h"
#include "RPiPlayer.h"
#include "android/net/DatagramSocket.h"

using namespace android::os;

RtpMediaSource::RtpMediaSource(MediaSourceType type, const sp<Handler>& player) :
	mCondVar(mCondVarLock),
	mType(type),
	mPlayer(player) {
	mAssembler = new RtpAvcAssembler(this);
}

RtpMediaSource::~RtpMediaSource() {
}

void RtpMediaSource::run() {
	while (!isInterrupted()) {
		// receive stuff on RTP and RTCP port
	}
}
