#ifndef RTSPMEDIASOURCE_H_
#define RTSPMEDIASOURCE_H_

#include "android/os/Thread.h"
#include "android/lang/String.h"

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

	void start(const android::lang::String& url,
			const sp<android::os::Message>& reply);
	void stop();
	void describeService(const sp<android::os::Message>& reply);
	void setupTrack(const sp<android::os::Message>& reply);
	void playTrack(const sp<android::os::Message>& reply);

private:
	enum State {
		NO_MEDIA_SOURCE,
		SETUP_MEDIA_SOURCE_DONE,
		DESCRIBE_SERVICE,
		DESCRIBE_SERVICE_DONE,
		SETUP_TRACK,
		SETUP_TRACK_DONE,
		PLAY_TRACK,
		PLAY_TRACK_DONE
	};

	void start();
	void setupMediaSource(sp<android::os::Message> reply);

	android::lang::String mUrl;
	android::lang::String mHost;
	android::lang::String mPort;
	android::lang::String mServiceDesc;
	sp<RtspSocket> mSocket;
	uint32_t mCSeq;
	State mState;
	sp<android::os::Message> mReply;
	android::lang::String mSessionId;
};

#endif /* RTSPMEDIASOURCE_H_ */
