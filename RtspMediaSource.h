#ifndef RTSPMEDIASOURCE_H_
#define RTSPMEDIASOURCE_H_

#include "android/os/Thread.h"
#include "android/os/Handler.h"
#include "MediaSourceType.h"
#include "RtpMediaSource.h"
#include "android/lang/String.h"
#include "RtspSocket.h"

class RtspMediaSource :
	public android::os::Handler {
public:
	RtspMediaSource();
	virtual ~RtspMediaSource();

	void setupMediaSource(const android::os::sp<android::os::Handler>& netHandler,
			const android::lang::String& url,
			const android::os::sp<android::os::Message>& reply);
	void describeService();

	void handleMessage(const android::os::sp<android::os::Message>& message);

private:
	static const uint32_t SETUP_MEDIA_SOURCE = 0;
	static const uint32_t DESCRIBE_SERVICE = 1;

	android::os::sp<android::os::Handler> mNetHandler;
	android::os::sp<RtpMediaSource> mAudioTrack;
	android::os::sp<RtpMediaSource> mVideoTrack;
	android::lang::String mUrl;
	android::lang::String mHost;
	android::lang::String mPort;
	android::os::sp<RtspSocket> mSocket;
};

#endif /* RTSPMEDIASOURCE_H_ */
