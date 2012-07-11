#ifndef RTSPMEDIASOURCE_H_
#define RTSPMEDIASOURCE_H_

#include "android/os/Ref.h"
#include "android/os/Handler.h"
#include "MediaSourceType.h"
#include "RtpMediaSource.h"

class RtspMediaSource :
	public android::os::Handler {
public:
	RtspMediaSource(const android::os::sp<android::os::Handler>& player);
	virtual ~RtspMediaSource();

	void handleMessage(const android::os::sp<android::os::Message>& message);

	int32_t dequeueBuffer(MediaSourceType type , android::os::sp<Buffer>* accessUnit);

	android::os::sp<RtpMediaSource> getMediaSource(MediaSourceType type) {
	    return (type == AUDIO) ? mAudioTrack : mVideoTrack;
	}

private:
	const android::os::sp<android::os::Handler>& mPlayer;
	android::os::sp<RtpMediaSource> mAudioTrack;
	android::os::sp<RtpMediaSource> mVideoTrack;
};

#endif /* RTSPMEDIASOURCE_H_ */
