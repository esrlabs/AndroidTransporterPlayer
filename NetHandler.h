#ifndef NETHANDLER_H_
#define NETHANDLER_H_

#include "android/os/Handler.h"
#include "android/os/LooperThread.h"
#include "RtspMediaSource.h"

class NetHandler :
	public android::os::Handler
{
public:
	static const uint32_t SETUP_MEDIA_SOURCE = 0;
	static const uint32_t SETUP_MEDIA_SOURCE_DONE = 1;

	NetHandler();
	virtual void handleMessage(const android::os::sp<android::os::Message>& message);

private:
	android::os::sp<Handler> mPlayer;
	android::os::sp< android::os::LooperThread<RtspMediaSource> > mRtspMediaSourceLooper;
	android::os::sp<RtspMediaSource> mRtspMediaSource;
};

#endif /* NETHANDLER_H_ */
