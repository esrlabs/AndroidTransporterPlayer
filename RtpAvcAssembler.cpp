#include "RtpAvcAssembler.h"
#include "RtpMediaSource.h"

using namespace android::os;

RtpAvcAssembler::RtpAvcAssembler(sp<RtpMediaSource> mediaSource) :
	mMediaSource(mediaSource) {
}

RtpAvcAssembler::~RtpAvcAssembler() {
}
