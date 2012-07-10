#ifndef RTPAVCASSEMBLER_H_
#define RTPAVCASSEMBLER_H_

#include "android/os/Ref.h"

class RtpMediaSource;

class RtpAvcAssembler :
	public android::os::Ref {
public:
	RtpAvcAssembler(android::os::sp<RtpMediaSource> mediaSource);
	virtual ~RtpAvcAssembler();

private:
	android::os::sp<RtpMediaSource> mMediaSource;
};

#endif /* RTPAVCASSEMBLER_H_ */
