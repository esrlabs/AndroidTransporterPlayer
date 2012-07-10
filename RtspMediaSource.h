#ifndef RTSPMEDIASOURCE_H_
#define RTSPMEDIASOURCE_H_

#include "android/os/Ref.h"
#include "android/os/Handler.h"
#include "MediaSourceType.h"
#include "RtpMediaSource.h"

class RtspMediaSource :
	public android::os::Ref,
	public android::os::Handler {
public:
	RtspMediaSource(android::os::Handler& player);
	virtual ~RtspMediaSource();

	void handleMessage(android::os::Message& message);

	int32_t dequeueBuffer(MediaSourceType type , android::os::sp<Buffer>* accessUnit);

	android::os::sp<RtpMediaSource> getMediaSource(MediaSourceType type) {
	    return (type == AUDIO) ? mAudioTrack : mVideoTrack;
	}

private:
	android::os::Handler& mPlayer;
	android::os::sp<RtpMediaSource> mAudioTrack;
	android::os::sp<RtpMediaSource> mVideoTrack;
};

#endif /* RTSPMEDIASOURCE_H_ */
