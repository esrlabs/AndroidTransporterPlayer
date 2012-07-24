#ifndef RTSPMEDIASOURCE_H_
#define RTSPMEDIASOURCE_H_

#include "android/os/Handler.h"
#include "android/os/Thread.h"
#include "android/lang/String.h"
#include "android/util/List.h"
#include <map>

class RtspSocket;
namespace android {
namespace os {
class Message;
}
namespace util {
class Buffer;
}
}

using android::os::sp;

class RtspMediaSource :
	public android::os::Handler {
public:
	static const uint32_t PROCESS_RTSP_PACKET = 0;
	static const uint32_t START_VIDEO_TRACK = 1;

	RtspMediaSource(const sp<android::os::Handler>& netHandler);
	virtual ~RtspMediaSource();

	bool start(const android::lang::String& url);
	void stop();

	virtual void handleMessage(const sp<android::os::Message>& message);

private:

	class NetReceiver :
		public android::os::Thread
	{
	public:
		NetReceiver(const sp<RtspSocket>& socket, const sp<android::os::Message>& reply);
		virtual void run();
		void stop();

	private:
		sp<RtspSocket> mSocket;
		sp<android::os::Message> mReply;

		NO_COPY_CTOR_AND_ASSIGNMENT_OPERATOR(NetReceiver)
	};

	enum State {
		NO_MEDIA_SOURCE,
		DESCRIBE_MEDIA_SOURCE,
		SETUP_AUDIO_TRACK,
		PLAY_AUDIO_TRACK,
		SETUP_VIDEO_TRACK,
		PLAY_VIDEO_TRACK,
	};

	void describeMediaSource();
	void onDescribeMediaSource(const sp<android::util::Buffer>& desc);
	void setupAudioTrack(uint16_t port);
	void playAudioTrack();
	void setupVideoTrack(uint16_t port);
	void playVideoTrack();

	sp<android::os::Handler> mNetHandler;
	sp<NetReceiver> mNetReceiver;
	sp<RtspSocket> mSocket;
	android::lang::String mHost;
	android::lang::String mPort;
	android::lang::String mServiceDesc;
	android::lang::String mAudioMediaSourceUrl;
	android::lang::String mVideoMediaSourceUrl;

	uint32_t mCSeq;
	State mState;
	android::lang::String mSessionId;
	std::map<uint32_t, android::lang::String> mMessageMappings;
};

#endif /* RTSPMEDIASOURCE_H_ */
