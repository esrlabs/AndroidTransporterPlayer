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
	static const uint32_t MEDIA_SOURCE_NOTIFY = 1;
	static const uint32_t STOP_MEDIA_SOURCE_DONE = 2;

	RPiPlayer();
	virtual ~RPiPlayer();
	void start(android::lang::String url);
	void stop();

	virtual void handleMessage(const sp<android::os::Message>& message);

	bool setupMediaSource(const android::lang::String& url);
	void stopMediaSource();

private:
	int initOMX();
	void finalizeOMX();
	void onMediaSourceNotify(const sp<Buffer>& accessUnit);

	sp< android::os::LooperThread<NetHandler> > mNetLooper;

	TUNNEL_T mTunnel[4];
	COMPONENT_T* mComponentList[5];
	ILCLIENT_T* mClient;
	COMPONENT_T* mVideoDecoder;
	COMPONENT_T* mVideoScheduler;
	COMPONENT_T* mVideoRenderer;
	COMPONENT_T* mClock;
	OMX_BUFFERHEADERTYPE* mBuffer;
	unsigned int mBufferFillSize;
	bool mPortSettingsChanged;
	bool mFirstPacket;
};

#endif /* RPIPLAYER_H_ */
