#ifndef RPIPLAYER_H_
#define RPIPLAYER_H_

#include "android/os/LooperThread.h"
#include "NetHandler.h"

class RPiPlayer :
	public android::os::Handler
{
public:
	static const int32_t SET_DATA_SOURCE = 0;
	static const int32_t MEDIA_SOURCE_NOTIFY = 1;

	RPiPlayer();
	virtual ~RPiPlayer();
	void start();

	virtual void handleMessage(const android::os::sp<android::os::Message>& message);

	bool setupMediaSource();

private:
	android::os::sp< android::os::LooperThread<NetHandler> > mNetLooper;
	android::os::sp<RtspMediaSource> mRtspMediaSource;
};

#endif /* RPIPLAYER_H_ */
