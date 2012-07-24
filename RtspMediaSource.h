#ifndef RTSPMEDIASOURCE_H_
#define RTSPMEDIASOURCE_H_

#include "android/os/Handler.h"
#include "android/os/Thread.h"
#include "android/os/Lock.h"
#include "android/lang/String.h"
#include "android/util/List.h"
#include "RtspSocket.h"
#include <map>

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
	static const uint32_t START_VIDEO_TRACK = 0;
	static const uint32_t DESCRIBE_MEDIA_SOURCE = 1;
	static const uint32_t SETUP_VIDEO_TRACK = 2;
	static const uint32_t PLAY_VIDEO_TRACK = 3;

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
		NetReceiver(const sp<RtspMediaSource>& mediaSource);
		virtual void run();
		void stop();

	private:
		sp<RtspMediaSource> mMediaSource;
		sp<RtspSocket> mSocket;

		NO_COPY_CTOR_AND_ASSIGNMENT_OPERATOR(NetReceiver)
	};

	sp<RtspSocket> getSocket() { return mSocket; }
	void describeMediaSource();
	void onDescribeMediaSource(const sp<android::util::Buffer>& desc);
	void setupAudioTrack(uint16_t port);
	void playAudioTrack();
	void setupVideoTrack(uint16_t port);
	void playVideoTrack();
	void setPendingRequest(uint32_t id, const sp<android::os::Message>& message);
	sp<android::os::Message> getPendingRequest(uint32_t id);
	sp<android::os::Message> removePendingRequest(uint32_t id);

	sp<android::os::Handler> mNetHandler;
	sp<NetReceiver> mNetReceiver;
	sp<RtspSocket> mSocket;
	android::lang::String mHost;
	android::lang::String mPort;
	android::lang::String mSdpFile;
	android::lang::String mAudioMediaSource;
	android::lang::String mAudioSessionId;
	android::lang::String mVideoMediaSource;
	android::lang::String mVideoSessionId;
	uint32_t mCSeq;

	android::os::Lock mLock;
	std::map< uint32_t, sp<android::os::Message> > mPendingRequests;

	friend class NetReceiver;
};

#endif /* RTSPMEDIASOURCE_H_ */
