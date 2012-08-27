/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Refactorings by E.S.R.Labs GmbH
 */

#include "RtpMediaSource.h"
#include "mindroid/os/Handler.h"
#include "mindroid/net/DatagramSocket.h"
#include "mindroid/util/Buffer.h"
#include "MediaAssembler.h"
#include "rtcp/Rtcp.h"
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/resource.h>

using namespace mindroid;

RtpMediaSource::RtpMediaSource(uint16_t port) :
		mRtpPacketCounter(0),
		mHighestSeqNumber(0) {
	mQueue = new List< sp<Buffer> >();
	mNetReceiver = new NetReceiver(port, obtainMessage(NOTIFY_RTP_PACKET), obtainMessage(NOTIFY_RTCP_PACKET));
//	mRtcp = new Rtcp(obtainMessage(NOTIFY_RTCP_SR));
}

RtpMediaSource::~RtpMediaSource() {
}

bool RtpMediaSource::start(sp<MediaAssembler> mediaAssembler) {
	mMediaAssembler = mediaAssembler;
	return mNetReceiver->start();
}

void RtpMediaSource::stop() {
	mNetReceiver->stop();
	removeCallbacksAndMessages();
}

void RtpMediaSource::handleMessage(const sp<Message>& message) {
    switch (message->what) {
        case NOTIFY_RTP_PACKET: {
            sp<Bundle> bundle = message->metaData();
            sp<Buffer> buffer = bundle->getObject<Buffer>("RTP-Packet");
            if (parseRtpHeader(buffer) == 0) {
                processRtpPayload(buffer);
            }
            break;
        }
        case NOTIFY_RTCP_PACKET: {
//			sp<Bundle> bundle = message->metaData();
//			sp<Buffer> buffer = bundle->getObject<Buffer>("RTCP-Packet");
//			mRtcp->onPacketReceived(buffer);
            break;
        }
        case NOTIFY_RTCP_SR: {
//			sp<Bundle> bundle = message->metaData();
//			sp<Rtcp::SenderReport> sr = bundle->getObject<Rtcp::SenderReport>("RTCP-SR-Packet");
            break;
        }
        default:
            break;
    }
}

RtpMediaSource::NetReceiver::NetReceiver(uint16_t port, sp<Message> notifyRtpPacket, sp<Message> notifyRtcpPacket) :
		mNotifyRtpPacket(notifyRtpPacket),
		mNotifyRtcpPacket(notifyRtcpPacket) {
	mRtpSocket = new DatagramSocket(port);
	mRtcpSocket = new DatagramSocket(port + 1);
	// We saw some drops when working with standard buffer sizes, so give the sockets 256KB buffer.
	int size = 256 * 1024;
	setsockopt(mRtpSocket->getId(), SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));

	// This pipe is used to unblock the 'select' call when stopping the NetReceiver.
	pipe(mPipe);
	int flags = fcntl(mPipe[0], F_GETFL);
	flags |= O_NONBLOCK;
	fcntl(mPipe[0], F_SETFL, flags);
	flags = fcntl(mPipe[1], F_GETFL);
	flags |= O_NONBLOCK;
	fcntl(mPipe[1], F_SETFL, flags);
}

void RtpMediaSource::NetReceiver::stop() {
	interrupt();
	mRtpSocket->close();
	mRtcpSocket->close();
	write(mPipe[1], "X", 1);
	join();
	// mNotifyRtpPacket holds a (circular) dependency to the RtpMediaSource handler.
	mNotifyRtpPacket = NULL;
	mNotifyRtcpPacket = NULL;
}

void RtpMediaSource::NetReceiver::run() {
	setpriority(PRIO_PROCESS, 0, -16);

	while (!isInterrupted()) {
		fd_set sockets;
		FD_ZERO(&sockets);

		FD_SET(mRtpSocket->getId(), &sockets);
		FD_SET(mRtcpSocket->getId(), &sockets);
		FD_SET(mPipe[0], &sockets);

		int maxId = mRtpSocket->getId();
		if (mRtcpSocket->getId() > maxId) {
			maxId = mRtcpSocket->getId();
		}
		if (mPipe[0] > maxId) {
			maxId = mPipe[0];
		}

		int result = select(maxId + 1, &sockets, NULL, NULL, NULL);

		if (result > 0) {
			if (FD_ISSET(mRtpSocket->getId(), &sockets)) {
				sp<Buffer> buffer(new Buffer(MAX_UDP_PACKET_SIZE));
				ssize_t size = mRtpSocket->recv(buffer->data(), buffer->capacity());
				if (size > 0) {
					buffer->setRange(0, size);
					sp<Message> msg = mNotifyRtpPacket->dup();
					sp<Bundle> bundle = msg->metaData();
					bundle->putObject("RTP-Packet", buffer);
					msg->sendToTarget();
				}
			} else if (FD_ISSET(mRtcpSocket->getId(), &sockets)) {
				sp<Buffer> buffer(new Buffer(MAX_UDP_PACKET_SIZE));
				ssize_t size = mRtcpSocket->recv(buffer->data(), buffer->capacity());
				if (size > 0) {
					buffer->setRange(0, size);
					sp<Message> msg = mNotifyRtcpPacket->dup();
					sp<Bundle> bundle = msg->metaData();
					bundle->putObject("RTCP-Packet", buffer);
					msg->sendToTarget();
				}
			}
		}
	}
}

int RtpMediaSource::parseRtpHeader(const sp<Buffer>& buffer) {
	size_t size = buffer->size();
	const uint8_t* data = buffer->data();

	if (size < RTP_HEADER_SIZE) {
		return -1;
	}

	if ((data[0] >> 6) != 2) { // v2.0
		return -1;
	}

	if (data[0] & PADDING_BIT) {
		size_t paddingSize = data[size - 1];
		if (paddingSize + RTP_HEADER_SIZE > size) {
			return -1;
		}
		size -= paddingSize;
	}

	int numCSRCs = data[0] & 0x0F;
	size_t payloadOffset = 12 + 4 * numCSRCs;
	if (size < payloadOffset) {
		return -1;
	}

	if (data[0] & EXT_HEADER_BIT) {
		if (size < payloadOffset + 4) {
			return -1;
		}

		const uint8_t* extensionData = &data[payloadOffset];

		size_t extensionSize =
			4 * (extensionData[2] << 8 | extensionData[3]);

		if (size < payloadOffset + 4 + extensionSize) {
			return -1;
		}

		payloadOffset += 4 + extensionSize;
	}

	buffer->setRange(payloadOffset, size - payloadOffset);

	uint16_t seqNum = data[2] << 8 | data[3];
	uint32_t srcId = data[8] << 24 | data[9] << 16 | data[10] << 8 | data[11];
	uint32_t rtpTime = data[4] << 24 | data[5] << 16 | data[6] << 8 | data[7];

	buffer->setId(seqNum);
	buffer->metaData()->putUInt32("SSRC", srcId);
	buffer->metaData()->putUInt32("RTP-Time", rtpTime);
	buffer->metaData()->putUInt32("PT", data[1] & 0x7f);
	buffer->metaData()->putBool("M", data[1] & MARK_BIT);

	return 0;
}

void RtpMediaSource::processRtpPayload(const sp<Buffer>& buffer) {
	uint32_t seqNum = (uint32_t)buffer->getId();

	if (mRtpPacketCounter++ == 0) {
		mHighestSeqNumber = seqNum;
		mQueue->push_back(buffer);
		mMediaAssembler->processMediaQueue();
		return;
	}

	// Expand the sequence number to a 32 bit value.
	uint32_t seqNum1 = seqNum | (mHighestSeqNumber & 0xFFFF0000);
	uint32_t seqNum2 = seqNum | ((mHighestSeqNumber & 0xFFFF0000) + 0x10000);
	uint32_t seqNum3 = seqNum | ((mHighestSeqNumber & 0xFFFF0000) - 0x10000);
	uint32_t diffNum1 = seqNum1 > mHighestSeqNumber ? seqNum1 - mHighestSeqNumber : mHighestSeqNumber - seqNum1;
	uint32_t diffNum2 = seqNum2 > mHighestSeqNumber ? seqNum2 - mHighestSeqNumber : mHighestSeqNumber - seqNum2;
	uint32_t diffNum3 = seqNum3 > mHighestSeqNumber ? seqNum3 - mHighestSeqNumber : mHighestSeqNumber - seqNum3;

	if (diffNum1 < diffNum2) {
		if (diffNum1 < diffNum3) {
			seqNum = seqNum1;
		} else {
			seqNum = seqNum3;
		}
	} else if (diffNum2 < diffNum3) {
		seqNum = seqNum2;
	} else {
		seqNum = seqNum3;
	}

	if (seqNum > mHighestSeqNumber) {
		mHighestSeqNumber = seqNum;
	}

	buffer->setId(seqNum);

	List< sp<Buffer> >::iterator itr = mQueue->begin();
	while (itr != mQueue->end() && (uint32_t)(*itr)->getId() < seqNum) {
		++itr;
	}

	if (itr != mQueue->end() && (uint32_t)(*itr)->getId() == seqNum) {
		return;
	}

	mQueue->insert(itr, buffer);
	mMediaAssembler->processMediaQueue();
}
