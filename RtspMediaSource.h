#ifndef RTSPMEDIASOURCE_H_
#define RTSPMEDIASOURCE_H_

#include "android/os/Ref.h"
#include "android/os/Handler.h"
#include "MediaSourceType.h"
#include "RtpMediaSource.h"
#include "android/lang/String.h"

class RtspSocket;

class RtspMediaSource :
	public android::os::Handler {
public:
	RtspMediaSource(const android::os::sp<android::os::Handler>& player, const android::lang::String& url);
	virtual ~RtspMediaSource();

	void start(android::os::sp<android::os::Message> reply);
	void stop();

	void handleMessage(const android::os::sp<android::os::Message>& message);

	int32_t dequeueBuffer(MediaSourceType type , android::os::sp<Buffer>* accessUnit);

	android::os::sp<RtpMediaSource> getMediaSource(MediaSourceType type) {
	    return (type == AUDIO) ? mAudioTrack : mVideoTrack;
	}

	void describe();

private:
	const android::os::sp<android::os::Handler>& mPlayer;
	android::os::sp<RtpMediaSource> mAudioTrack;
	android::os::sp<RtpMediaSource> mVideoTrack;
	android::lang::String mUrl;
	android::lang::String mHost;
	android::lang::String mPort;
	android::os::sp<RtspSocket> mSocket;
};

#endif /* RTSPMEDIASOURCE_H_ */
