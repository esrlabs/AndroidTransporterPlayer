#include "RtspMediaSource.h"
#include "RtspSocket.h"
#include "Bundle.h"
#include "android/os/Message.h"
#include "android/os/Handler.h"
#include <stdio.h>

using namespace android::os;
using namespace android::lang;
using namespace android::net;

RtspMediaSource::RtspMediaSource() :
	mCSeq(1),
	mState(NO_MEDIA_SOURCE) {
}

RtspMediaSource::~RtspMediaSource() {
}

void RtspMediaSource::start(const String& url, const sp<Message>& reply) {
	mUrl = url;
	mReply = reply;
	Thread::start();
}

void RtspMediaSource::stop() {
	interrupt();
	join();
}

void RtspMediaSource::describeService(const sp<Message>& reply) {
	mReply = reply;
	mState = DESCRIBE_SERVICE;
	String describeMessage = String::format("DESCRIBE rtsp://%s:%s/%s RTSP/1.0\r\nCSeq: %d\r\n\r\n", mHost.c_str(), mPort.c_str(), mServiceDesc.c_str(), mCSeq++);
	mSocket->write(describeMessage.c_str(), describeMessage.size());
}

void RtspMediaSource::setupTrack(const sp<Message>& reply) {
	mReply = reply;
	mState = SETUP_TRACK;
	String setupMessage = String::format("SETUP rtsp://%s:%s/%s/trackID=0 RTSP/1.0\r\nCSeq: %d\r\nTransport: RTP/AVP;unicast;client_port=56098-56099\r\n\r\n",
			mHost.c_str(), mPort.c_str(), mServiceDesc.c_str(), mCSeq++);
	mSocket->write(setupMessage.c_str(), setupMessage.size());
}

void RtspMediaSource::playTrack(const sp<android::os::Message>& reply) {
	mReply = reply;
	mState = PLAY_TRACK;
	String playMessage = String::format("PLAY rtsp://%s:%s/%s/trackID=0 RTSP/1.0\r\nCSeq: %d\r\nRange: npt=0.000-\r\nSession: %s\r\n\r\n",
			mHost.c_str(), mPort.c_str(), mServiceDesc.c_str(), mCSeq++, mSessionId.c_str());
	mSocket->write(playMessage.c_str(), playMessage.size());
}

void RtspMediaSource::setupMediaSource(sp<Message> reply) {
	if (!mUrl.startsWith("rtsp://")) {
		reply->arg1 = -1;
		reply->sendToTarget();
	}

	String mediaSource = mUrl.substr(strlen("rtsp://"));
	ssize_t separatorIndex = mediaSource.indexOf("/");
	if  (separatorIndex > 0) {
		mServiceDesc = mediaSource.substr(separatorIndex + 1);
		mediaSource = mediaSource.substr(0, separatorIndex);
	} else {
		reply->arg1 = -1;
		reply->sendToTarget();
	}

	separatorIndex = mediaSource.indexOf(":");
	if (separatorIndex > 0) {
		mHost = mediaSource.substr(0, separatorIndex);
		mPort = mediaSource.substr(separatorIndex + 1);
	} else {
		mHost = mediaSource;
		mPort = "9000";
	}

	mSocket = new RtspSocket();
	if (!mSocket->connect(mHost.c_str(), atoi(mPort.c_str()))) {
		reply->arg1 = -1;
		reply->sendToTarget();
	}
	reply->arg1 = 0;
	reply->sendToTarget();
	mState = SETUP_MEDIA_SOURCE_DONE;
}

void RtspMediaSource::run() {
	setSchedulingParams(SCHED_OTHER, -16);

	setupMediaSource(mReply);
	int32_t resultCode = mReply->arg1;
	mReply = NULL;
	if (resultCode != 0) {
		return;
	}

	while (!isInterrupted()) {
		RtspHeader* rtspHeader = mSocket->readPacket();
		if (rtspHeader != NULL) {
			uint32_t contentLength;
			RtspHeader::iterator itr = rtspHeader->begin();
			while (itr != rtspHeader->end()) {
				printf("%s: %s\n", itr->first.c_str(), itr->second.c_str());
				if (itr->first == "Content-Length") {
					contentLength = atoi(itr->second.c_str());
					uint8_t data[contentLength];
					mSocket->readFully(data, contentLength);
					printf("%s\n", String((char*)data, contentLength).c_str());
				} else if (itr->first == "Session") {
					mSessionId = itr->second;
				}
				++itr;
			}
			delete rtspHeader;
		}

		sp<Message> reply = mReply;
		mReply = NULL;

		if (mState == DESCRIBE_SERVICE) {
			reply->arg1 = 0;
			mState = DESCRIBE_SERVICE_DONE;
			reply->sendToTarget();
		} else if (mState == SETUP_TRACK) {
			reply->arg1 = 0;
			mState = SETUP_TRACK_DONE;
			reply->sendToTarget();
		} else if (mState == PLAY_TRACK) {
			reply->arg1 = 0;
			mState = PLAY_TRACK_DONE;
			reply->sendToTarget();
		}
	}
}
