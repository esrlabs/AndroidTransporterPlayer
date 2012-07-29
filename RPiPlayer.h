#ifndef RPIPLAYER_H_
#define RPIPLAYER_H_

#include "mindroid/os/LooperThread.h"
#include "NetHandler.h"
#include "mindroid/lang/String.h"
#include "mindroid/util/List.h"
extern "C" {
#include "bcm_host.h"
#include "ilclient.h"
}

namespace mindroid {
class Buffer;
}

using mindroid::sp;

class RPiPlayer :
	public mindroid::Handler
{
public:
	static const uint32_t NOTIFY_QUEUE_AUDIO_BUFFER = 1;
	static const uint32_t NOTIFY_QUEUE_VIDEO_BUFFER = 2;
	static const uint32_t NOTIFY_PLAY_AUDIO_BUFFER = 3;
	static const uint32_t NOTIFY_PLAY_VIDEO_BUFFER = 4;
	static const uint32_t STOP_MEDIA_SOURCE_DONE = 5;
	static const uint32_t NOTIFY_EMPTY_OMX_BUFFER = 8;

	RPiPlayer();
	virtual ~RPiPlayer();
	bool start(mindroid::String url);
	void stop();

	virtual void handleMessage(const sp<mindroid::Message>& message);

private:
	bool startMediaSource(const mindroid::String& url);
	void stopMediaSource();
	int initOMXAudio();
	bool setAudioSink(const char* sinkName);
	void finalizeOMXAudio();
	int initOMXVideo();
	void finalizeOMXVideo();
	void onPlayAudioBuffer();
	void onFillInputBuffers();
	void onPlayVideoBuffer(const sp<mindroid::Buffer>& buffer);
	static void onEmptyBufferDone(void* args, COMPONENT_T* component);

	sp< mindroid::LooperThread<NetHandler> > mNetLooper;
	sp< mindroid::List< sp<mindroid::Buffer> > > mAudioBuffers;
	sp< mindroid::List< sp<mindroid::Buffer> > > mVideoBuffers;

	// Audio
	ILCLIENT_T* mAudioClient;
	COMPONENT_T* mAudioRenderer;
	COMPONENT_T* mAudioComponentList[2];
	OMX_BUFFERHEADERTYPE* mAudioBuffer;
	bool mFirstPacketAudio;

	// Video
	ILCLIENT_T* mVideoClient;
	TUNNEL_T mTunnel[4];
	COMPONENT_T* mVideoComponentList[5];
	COMPONENT_T* mVideoDecoder;
	COMPONENT_T* mVideoScheduler;
	COMPONENT_T* mVideoRenderer;
	COMPONENT_T* mClock;
	bool mPortSettingsChanged;
	bool mFirstPacketVideo;

	sp< mindroid::List< OMX_BUFFERHEADERTYPE* > > mFilledOmxInputBuffers;
	sp< mindroid::List< OMX_BUFFERHEADERTYPE* > > mEmptyOmxInputBuffers;
	uint32_t getSamplesInOmx();

};

#endif /* RPIPLAYER_H_ */
