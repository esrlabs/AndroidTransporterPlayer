#ifndef RTPMEDIASOURCE_H_
#define RTPMEDIASOURCE_H_

#include "android/os/Ref.h"
#include "android/os/Lock.h"
#include "android/os/CondVar.h"
#include "android/os/Handler.h"
#include "List.h"
#include "Buffer.h"
#include "MediaSourceType.h"

class RtpAvcAssembler;

class RtpMediaSource :
	public android::os::Ref,
	public android::os::Handler {
public:
	static const int32_t POLL_RTP_MEDIA_SOURCE = 0;

	RtpMediaSource(MediaSourceType type, android::os::Handler& player);
	virtual ~RtpMediaSource();

	virtual void handleMessage(android::os::Message& message);

	void pollMediaSource();
	void onPollMediaSource();

	void enqueueAccessUnit(const android::os::sp<Buffer>& accessUnit);
	int32_t dequeueAccessUnit(android::os::sp<Buffer>* accessUnit);

	bool empty() { return mAccessUnits.empty(); }

private:
	android::os::Lock mCondVarLock;
	android::os::CondVar mCondVar;
	MediaSourceType mType;
	android::os::Handler& mPlayer;
	List< android::os::sp<Buffer> > mAccessUnits;
	android::os::sp<RtpAvcAssembler> mAssembler;

	NO_COPY_CTOR_AND_ASSIGNMENT_OPERATOR(RtpMediaSource)
};

#endif /* RTPMEDIASOURCE_H_ */
