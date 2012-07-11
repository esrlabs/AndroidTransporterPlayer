#include "NetHandler.h"
#include "RPiPlayer.h"

using namespace android::os;

NetHandler::NetHandler() :
	mPlayer(NULL) {
}

void NetHandler::handleMessage(const sp<Message>& message) {
	switch (message->what) {
	case SETUP_MEDIA_SOURCE:
		mPlayer = (Handler*) message->obj;
		mRtspMediaSource = new RtspMediaSource(mPlayer);
		sp<Message> reply = mPlayer->obtainMessage(RPiPlayer::SET_DATA_SOURCE);
		reply->obj = mRtspMediaSource.getPointer();
		reply->sendToTarget();
		break;
	}
}
