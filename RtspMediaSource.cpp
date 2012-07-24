#include "RtspMediaSource.h"
#include "RtspSocket.h"
#include "NetHandler.h"
#include "android/os/Message.h"
#include "android/os/Handler.h"
#include "android/util/Buffer.h"
#include <stdio.h>

using namespace android::os;
using namespace android::lang;
using namespace android::util;
using namespace android::net;

RtspMediaSource::RtspMediaSource(const sp<android::os::Handler>& netHandler) :
		mNetHandler(netHandler),
		mCSeq(1),
		mState(NO_MEDIA_SOURCE) {
}

RtspMediaSource::~RtspMediaSource() {
}

bool RtspMediaSource::start(const android::lang::String& url) {
	if (!url.startsWith("rtsp://")) {
		return false;
	}

	String mediaSource = url.substr(strlen("rtsp://"));
	ssize_t separatorIndex = mediaSource.indexOf("/");
	if  (separatorIndex > 0) {
		mServiceDesc = mediaSource.substr(separatorIndex + 1);
		mediaSource = mediaSource.substr(0, separatorIndex);
	} else {
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
		return false;
	}

	mNetReceiver = new NetReceiver(mSocket, obtainMessage(PROCESS_RTSP_PACKET));
	if (!mNetReceiver->start()) {
		return false;
	}

	describeMediaSource();

	return true;
}

void RtspMediaSource::stop() {
	mNetHandler = NULL;
	mNetReceiver->stop();
}

void RtspMediaSource::handleMessage(const sp<android::os::Message>& message) {
	switch (message->what) {
	case PROCESS_RTSP_PACKET: {
		RtspHeader* rtspHeader = (RtspHeader*) message->obj;
		sp<Bundle> bundle = message->getData();
		sp<Buffer> content;
		if (bundle != NULL) {
			content = bundle->getObject<Buffer>("Content");
		}

		RtspHeader::iterator itr = rtspHeader->find(String("Cseq"));
		if (itr != rtspHeader->end()) {
			int seqNum = atoi(itr->second.c_str());
			if (mMessageMappings[seqNum] != NULL) {
				if (mMessageMappings[seqNum] == "DESCRIBE") {
					printf("DESCRIBE\n");
					onDescribeMediaSource(content);
					sp<Message> msg = mNetHandler->obtainMessage(NetHandler::START_VIDEO_TRACK);
					msg->arg1 = 96;
					msg->sendToTarget();
				} else if (mMessageMappings[seqNum] == "SETUP") {
					mSessionId = (*rtspHeader)[String("Session")];
					playVideoTrack();
				} else if (mMessageMappings[seqNum] == "PLAY") {
				}
			}
		}

		delete rtspHeader;
		break;
	}
	case START_VIDEO_TRACK: {
		setupVideoTrack(message->arg1);
		break;
	}
	}
}

void RtspMediaSource::describeMediaSource() {
	mMessageMappings[mCSeq] = "DESCRIBE";
	String describeMessage = String::format("DESCRIBE rtsp://%s:%s/%s RTSP/1.0\r\nCSeq: %d\r\n\r\n",
			mHost.c_str(), mPort.c_str(), mServiceDesc.c_str(), mCSeq++);
	mSocket->write(describeMessage.c_str(), describeMessage.size());
}

void RtspMediaSource::onDescribeMediaSource(const sp<Buffer>& desc) {
	String mediaSourceDesc((char*)desc->data(), desc->size());
	List<String> lines = mediaSourceDesc.split("\n");
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

void RtspMediaSource::setupAudioTrack(uint16_t port) {
	String setupMessage = String::format("SETUP %s RTSP/1.0\r\nCSeq: %d\r\nTransport: RTP/AVP;unicast;client_port=%d-%d\r\n\r\n",
			mAudioMediaSourceUrl.c_str(), mCSeq++, port, port + 1);
	mSocket->write(setupMessage.c_str(), setupMessage.size());
}

void RtspMediaSource::playAudioTrack() {
	String playMessage = String::format("PLAY %s RTSP/1.0\r\nCSeq: %d\r\nRange: npt=0.000-\r\nSession: %s\r\n\r\n",
			mAudioMediaSourceUrl.c_str(), mCSeq++, mSessionId.c_str());
	mSocket->write(playMessage.c_str(), playMessage.size());
}

void RtspMediaSource::setupVideoTrack(uint16_t port) {
	mMessageMappings[mCSeq] = "SETUP";
	String setupMessage = String::format("SETUP %s RTSP/1.0\r\nCSeq: %d\r\nTransport: RTP/AVP;unicast;client_port=%d-%d\r\n\r\n",
			mVideoMediaSourceUrl.c_str(), mCSeq++, port, port + 1);
	printf("SETUP: %s\n", setupMessage.c_str());
	mSocket->write(setupMessage.c_str(), setupMessage.size());
}

void RtspMediaSource::playVideoTrack() {
	mMessageMappings[mCSeq] = "PLAY";
	String playMessage = String::format("PLAY %s RTSP/1.0\r\nCSeq: %d\r\nRange: npt=0.000-\r\nSession: %s\r\n\r\n",
			mVideoMediaSourceUrl.c_str(), mCSeq++, mSessionId.c_str());
	printf("PLAY: %s\n", playMessage.c_str());
	mSocket->write(playMessage.c_str(), playMessage.size());
}

RtspMediaSource::NetReceiver::NetReceiver(const sp<RtspSocket>& socket, const sp<Message>& reply) :
		mSocket(socket),
		mReply(reply) {
}

void RtspMediaSource::NetReceiver::run() {
	setSchedulingParams(SCHED_OTHER, -16);

	while (!isInterrupted()) {
		RtspHeader* rtspHeader = mSocket->readPacketHeader();
		if (rtspHeader != NULL) {
			sp<Message> reply = mReply->dup();
			reply->obj = rtspHeader;

			RtspHeader::iterator itr = rtspHeader->begin();
			while (itr != rtspHeader->end()) {
				if (itr->first == "Content-Length") {
					size_t contentLength = atoi(itr->second.c_str());
					if (contentLength > 0) {
						sp<Buffer> buffer = new Buffer(contentLength);
						mSocket->readFully(buffer->data(), contentLength);
						buffer->setRange(0, contentLength);
						sp<Bundle> bundle = new Bundle();
						bundle->putObject("Content", buffer);
						reply->setData(bundle);
					}
				}
				++itr;
			}

			reply->sendToTarget();
		}
	}
}

void RtspMediaSource::NetReceiver::stop() {
	interrupt();
	mSocket->close();
	join();
}
