#ifndef NETHANDLER_H_
#define NETHANDLER_H_

#include "android/os/Handler.h"
#include "RtspMediaSource.h"

class NetHandler :
	public android::os::Handler
{
public:
	static const uint32_t SETUP_MEDIA_SOURCE = 0;

	NetHandler();
	virtual void handleMessage(const android::os::sp<android::os::Message>& message);

private:
	android::os::sp<RtspMediaSource> mRtspMediaSource;
	android::os::sp<Handler> mPlayer;
};

#endif /* NETHANDLER_H_ */
