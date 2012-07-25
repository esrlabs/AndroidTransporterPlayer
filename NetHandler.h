#ifndef NETHANDLER_H_
#define NETHANDLER_H_

#include "mindroid/os/Handler.h"
#include "mindroid/os/LooperThread.h"
#include "RtspMediaSource.h"
#include "RtpMediaSource.h"

class RPiPlayer;

using mindroid::sp;

class NetHandler :
	public mindroid::Handler
{
public:
	static const uint32_t START_MEDIA_SOURCE = 0;
	static const uint32_t STOP_MEDIA_SOURCE = 1;
	static const uint32_t START_VIDEO_TRACK = 2;
	static const uint32_t STOP_VIDEO_TRACK = 3;
	static const uint32_t START_AUDIO_TRACK = 4;
	static const uint32_t STOP_AUDIO_TRACK = 5;

	NetHandler();
	~NetHandler();
	virtual void handleMessage(const sp<mindroid::Message>& message);

private:
	static const uint16_t RTP_AUDIO_SOURCE_PORT = 56096;
	static const uint16_t RTP_VIDEO_SOURCE_PORT = 56098;
	static const uint8_t PCM_AUDIO_TYPE = 10;
	static const uint8_t AVC_VIDEO_TYPE = 96;

	sp<RPiPlayer> mPlayer;
	sp<RtspMediaSource> mRtspMediaSource;
	sp<RtpMediaSource> mRtpAudioSource;
	sp<RtpMediaSource> mRtpVideoSource;
};

#endif /* NETHANDLER_H_ */
