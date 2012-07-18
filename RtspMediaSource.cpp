#include "RtspMediaSource.h"
#include "RtspSocket.h"
#include "Buffer.h"
#include "Bundle.h"
#include "android/os/Message.h"
#include "android/os/Handler.h"
#include <stdio.h>

using namespace android::os;
using namespace android::lang;
using namespace android::util;
using namespace android::net;

RtspMediaSource::RtspMediaSource() :
		mCSeq(1),
		mState(NO_MEDIA_SOURCE) {
}

RtspMediaSource::~RtspMediaSource() {
}

bool RtspMediaSource::start(const String& url, const sp<Message>& reply) {
	mUrl = url;
	mReply = reply;
	return Thread::start();
}

void RtspMediaSource::stop() {
	interrupt();
	mSocket->close();
	join();
}

void RtspMediaSource::describeService(const sp<Message>& reply) {
	mReply = reply;
	assert(mReply != NULL);
	mState = DESCRIBE_SERVICE;
	String describeMessage = String::format("DESCRIBE rtsp://%s:%s/%s RTSP/1.0\r\nCSeq: %d\r\n\r\n",
			mHost.c_str(), mPort.c_str(), mServiceDesc.c_str(), mCSeq++);
	mSocket->write(describeMessage.c_str(), describeMessage.size());
}

void RtspMediaSource::setupAudioTrack(uint16_t port, const sp<Message>& reply) {
	mReply = reply;
	mState = SETUP_AUDIO_TRACK;
	String setupMessage = String::format("SETUP %s RTSP/1.0\r\nCSeq: %d\r\nTransport: RTP/AVP;unicast;client_port=%d-%d\r\n\r\n",
			mAudioMediaSourceUrl.c_str(), mCSeq++, port, port + 1);
	mSocket->write(setupMessage.c_str(), setupMessage.size());
}

void RtspMediaSource::playAudioTrack(const sp<android::os::Message>& reply) {
	mReply = reply;
	mState = PLAY_AUDIO_TRACK;
	String playMessage = String::format("PLAY %s RTSP/1.0\r\nCSeq: %d\r\nRange: npt=0.000-\r\nSession: %s\r\n\r\n",
			mAudioMediaSourceUrl.c_str(), mCSeq++, mSessionId.c_str());
	mSocket->write(playMessage.c_str(), playMessage.size());
}

void RtspMediaSource::setupVideoTrack(uint16_t port, const sp<Message>& reply) {
	mReply = reply;
	mState = SETUP_VIDEO_TRACK;
	String setupMessage = String::format("SETUP %s RTSP/1.0\r\nCSeq: %d\r\nTransport: RTP/AVP;unicast;client_port=%d-%d\r\n\r\n",
			mVideoMediaSourceUrl.c_str(), mCSeq++, port, port + 1);
	mSocket->write(setupMessage.c_str(), setupMessage.size());
}

void RtspMediaSource::playVideoTrack(const sp<android::os::Message>& reply) {
	mReply = reply;
	mState = PLAY_VIDEO_TRACK;
	String playMessage = String::format("PLAY %s RTSP/1.0\r\nCSeq: %d\r\nRange: npt=0.000-\r\nSession: %s\r\n\r\n",
			mVideoMediaSourceUrl.c_str(), mCSeq++, mSessionId.c_str());
	mSocket->write(playMessage.c_str(), playMessage.size());
}

bool RtspMediaSource::setupMediaSource(sp<Message> reply) {
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
		return false;
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
		return false;
	}
	mState = SETUP_MEDIA_SOURCE_DONE;
	reply->arg1 = 0;
	reply->sendToTarget();
	return true;
}

void RtspMediaSource::run() {
	setSchedulingParams(SCHED_OTHER, -16);

	if (!setupMediaSource(mReply)) {
		return;
	}

	while (!isInterrupted()) {
		RtspHeader* rtspHeader = mSocket->readPacketHeader();
		if (rtspHeader != NULL) {
			RtspHeader::iterator itr = rtspHeader->begin();
			while (itr != rtspHeader->end()) {
				if (itr->first == "Content-Length") {
					size_t contentLength = atoi(itr->second.c_str());
					if (contentLength > 0) {
						sp<Buffer> buffer = new Buffer(contentLength);
						mSocket->readFully(buffer->data(), contentLength);
						buffer->setRange(0, contentLength);
						String sessionDesc((char*)buffer->data(), buffer->size());
						List<String> lines = sessionDesc.split("\n");
						List<String>::iterator itr = lines.begin();
						String mediaDesc;
						String audioMediaDesc;
						String videoMediaDesc;

						while (itr != lines.end()) {
							printf("%s\n", itr->c_str());
							if (itr->startsWith("m=")) {
								mediaDesc = *itr;
								if (mediaDesc.startsWith("m=audio 0 RTP/AVP 10")) {
									audioMediaDesc = mediaDesc;
								} else if (mediaDesc.startsWith("m=video 0 RTP/AVP 96")) {
									videoMediaDesc = mediaDesc;
								} else {
									videoMediaDesc = NULL;
								}
							} else if (itr->startsWith("a=")) {
								if (itr->startsWith("a=control:")) {
									if (!audioMediaDesc.isEmpty()) {
										mAudioMediaSourceUrl = itr->substr(String::size("a=control:")).trim();
									} else if (!videoMediaDesc.isEmpty()) {
										mVideoMediaSourceUrl = itr->substr(String::size("a=control:")).trim();
									}
								}
							}
							++itr;
						}
					}
				} else if (itr->first == "Session") {
					mSessionId = itr->second;
				}
				++itr;
			}
			delete rtspHeader;

			// TODO: make class state thread-safe
			sp<Message> reply = mReply;
			mReply = NULL;
			reply->arg1 = 0;
			reply->sendToTarget();
		}
	}
}
