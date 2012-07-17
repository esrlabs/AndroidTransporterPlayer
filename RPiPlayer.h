#ifndef RPIPLAYER_H_
#define RPIPLAYER_H_

#include "android/os/LooperThread.h"
#include "NetHandler.h"
#include "android/lang/String.h"
extern "C" {
#include "bcm_host.h"
#include "ilclient.h"
}

using android::os::sp;

class RPiPlayer :
	public android::os::Handler
{
public:
	static const int32_t SET_RTSP_MEDIA_SOURCE = 0;
	static const int32_t MEDIA_SOURCE_NOTIFY = 1;

	RPiPlayer();
	virtual ~RPiPlayer();
	void start(android::lang::String url);
	void stop();

	virtual void handleMessage(const sp<android::os::Message>& message);

	bool setupMediaSource(const android::lang::String& url);

private:
	int initOMX();
	void finalizeOMX();
	void onMediaSourceNotify(const sp<Buffer>& accessUnit);

	sp< android::os::LooperThread<NetHandler> > mNetLooper;
	sp<RtspMediaSource> mRtspMediaSource;

	TUNNEL_T tunnel[4];
	COMPONENT_T* list[5];
	ILCLIENT_T* client;
	COMPONENT_T* video_decode;
	COMPONENT_T* video_scheduler;
	COMPONENT_T* video_render;
	COMPONENT_T* omx_clock;

	OMX_BUFFERHEADERTYPE* omxBuffer;
	unsigned int omxBufferFillSize;
	bool portSettingsChanged;
	bool firstPacket;
};

#endif /* RPIPLAYER_H_ */
