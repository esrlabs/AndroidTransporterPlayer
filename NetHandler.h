#ifndef NETHANDLER_H_
#define NETHANDLER_H_

#include "android/os/Handler.h"
#include "android/os/LooperThread.h"
#include "RtspMediaSource.h"
#include "RtpMediaSource.h"

using android::os::sp;

class NetHandler :
	public android::os::Handler
{
public:
	static const uint32_t SETUP_MEDIA_SOURCE = 0;
	static const uint32_t SETUP_MEDIA_SOURCE_DONE = 1;
	static const uint32_t DESCRIBE_SERVICE_DONE = 2;
	static const uint32_t SETUP_TRACK_DONE = 3;
	static const uint32_t PLAY_TRACK_DONE = 4;
	static const uint32_t STOP_MEDIA_SOURCE = 5;

	NetHandler();
	~NetHandler();
	virtual void handleMessage(const sp<android::os::Message>& message);

private:
	static const uint16_t RTP_VIDEO_SOURCE_PORT = 56098;

	sp<Handler> mPlayer;
	sp<RtspMediaSource> mRtspMediaSource;
	sp<RtpMediaSource> mRtpVideoSource;
};

#endif /* NETHANDLER_H_ */
