#ifndef RPIPLAYER_H_
#define RPIPLAYER_H_

#include "android/os/LooperThread.h"
#include "NetHandler.h"
#include "android/lang/String.h"
#include "android/util/List.h"
extern "C" {
#include "bcm_host.h"
#include "ilclient.h"
}

namespace android {
namespace util {
class Buffer;
}
}

using android::os::sp;

class RPiPlayer :
	public android::os::Handler
{
public:
	static const uint32_t NOTIFY_QUEUE_AUDIO_BUFFER = 1;
	static const uint32_t NOTIFY_QUEUE_VIDEO_BUFFER = 2;
	static const uint32_t NOTIFY_PLAY_AUDIO_BUFFER = 3;
	static const uint32_t NOTIFY_PLAY_VIDEO_BUFFER = 4;
	static const uint32_t STOP_MEDIA_SOURCE_DONE = 5;

	RPiPlayer();
	virtual ~RPiPlayer();
	void start(android::lang::String url);
	void stop();

	virtual void handleMessage(const sp<android::os::Message>& message);

private:
	bool setupMediaSource(const android::lang::String& url);
	void stopMediaSource();
	int initOMX();
	void finalizeOMX();
	void onPlayVideoBuffer(const sp<android::util::Buffer>& accessUnit);
	static void onEmptyBufferDone(void* args, COMPONENT_T* component);

	sp< android::os::LooperThread<NetHandler> > mNetLooper;
	android::util::List< sp<android::util::Buffer> > mVideoAccessUnits;

	TUNNEL_T mTunnel[4];
	COMPONENT_T* mComponentList[5];
	ILCLIENT_T* mClient;
	COMPONENT_T* mVideoDecoder;
	COMPONENT_T* mVideoScheduler;
	COMPONENT_T* mVideoRenderer;
	COMPONENT_T* mClock;
	bool mPortSettingsChanged;
	bool mFirstPacket;
};

#endif /* RPIPLAYER_H_ */
