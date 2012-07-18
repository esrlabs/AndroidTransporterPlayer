#ifndef RTSPMEDIASOURCE_H_
#define RTSPMEDIASOURCE_H_

#include "android/os/Thread.h"
#include "android/lang/String.h"
#include "android/util/List.h"

class RtspSocket;
namespace android {
namespace os {
class Message;
class Handler;
}
}

using android::os::sp;

class RtspMediaSource :
	public android::os::Thread {
public:
	RtspMediaSource();
	virtual ~RtspMediaSource();

	virtual void run();

	bool start(const android::lang::String& url,
			const sp<android::os::Message>& reply);
	void stop();
	void describeService(const sp<android::os::Message>& reply);
	void setupAudioTrack(uint16_t port, const sp<android::os::Message>& reply);
	void playAudioTrack(const sp<android::os::Message>& reply);
	void setupVideoTrack(uint16_t port, const sp<android::os::Message>& reply);
	void playVideoTrack(const sp<android::os::Message>& reply);

private:
	enum State {
		NO_MEDIA_SOURCE,
		SETUP_MEDIA_SOURCE_DONE,
		DESCRIBE_SERVICE,
		DESCRIBE_SERVICE_DONE,
		SETUP_AUDIO_TRACK,
		SETUP_AUDIO_TRACK_DONE,
		PLAY_AUDIO_TRACK,
		PLAY_AUDIO_TRACK_DONE,
		SETUP_VIDEO_TRACK,
		SETUP_VIDEO_TRACK_DONE,
		PLAY_VIDEO_TRACK,
		PLAY_VIDEO_TRACK_DONE
	};

	void start();
	bool setupMediaSource(sp<android::os::Message> reply);

	android::lang::String mUrl;
	android::lang::String mHost;
	android::lang::String mPort;
	android::lang::String mServiceDesc;
	android::lang::String mAudioMediaSourceUrl;
	android::lang::String mVideoMediaSourceUrl;
	sp<RtspSocket> mSocket;
	uint32_t mCSeq;
	State mState;
	sp<android::os::Message> mReply;
	android::lang::String mSessionId;
};

#endif /* RTSPMEDIASOURCE_H_ */
