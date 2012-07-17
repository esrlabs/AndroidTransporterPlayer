#include "RtpMediaSource.h"
#include "android/os/Handler.h"
#include "android/net/DatagramSocket.h"
#include "Buffer.h"
#include "RtpAvcAssembler.h"
#include <stdio.h>
#include <sys/socket.h>

using namespace android::os;
using namespace android::net;

RtpMediaSource::RtpMediaSource() :
	mRtpPacketCounter(0),
	mHighestSeqNumber(0) {
}

RtpMediaSource::~RtpMediaSource() {
}

void RtpMediaSource::run() {
	setSchedulingParams(SCHED_OTHER, -16);

	while (!isInterrupted()) {
		fd_set sockets;
		FD_ZERO(&sockets);

		FD_SET(mRtpSocket->getSocketId(), &sockets);
		FD_SET(mRtcpSocket->getSocketId(), &sockets);

		int maxSocketId = mRtpSocket->getSocketId();
		if (mRtcpSocket->getSocketId() > maxSocketId) {
			maxSocketId = mRtcpSocket->getSocketId();
		}

		int result = select(maxSocketId + 1, &sockets, NULL, NULL, NULL);

		if (result > 0) {
			if (FD_ISSET(mRtpSocket->getSocketId(), &sockets)) {
				processMediaData(mRtpSocket);
			} else if (FD_ISSET(mRtcpSocket->getSocketId(), &sockets)) {
				processMediaData(mRtcpSocket);
			}
		}
	}
}

void RtpMediaSource::start(uint16_t port,
		const sp<android::os::Message>& accessUnitNotifyMessage) {
	mAssembler = new RtpAvcAssembler(accessUnitNotifyMessage);
	mRtpSocket = new DatagramSocket(port);
	mRtcpSocket = new DatagramSocket(port + 1);
//	int size = 1024 * 1024;
//	setsockopt(mRtpSocket->getSocketId(), SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
	Thread::start();
}

void RtpMediaSource::stop() {
	interrupt();
	join();
}

bool RtpMediaSource::processMediaData(sp<DatagramSocket> socket) {
    sp<Buffer> buffer(new Buffer(65536));

    ssize_t size = socket->recv(buffer->data(), buffer->capacity());
    if (size > 0) {
    	buffer->setRange(0, size);
    	if (socket == mRtpSocket) {
    		if (parseRtpHeader(buffer) == 0) {
    			processRtpPayload(buffer);
    		}
    	}
    }

    return size > 0;
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
    buffer->setMetaData(seqNum);

    return 0;
}

void RtpMediaSource::processRtpPayload(const sp<Buffer>& buffer) {
	uint32_t seqNum = (uint32_t)buffer->getMetaData();

	if (mRtpPacketCounter++ == 0) {
		mHighestSeqNumber = seqNum;
		mQueue.push_back(buffer);
		mAssembler->processMediaData(mQueue);
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

	buffer->setMetaData(seqNum);

//	printf("RtpMediaSource::processRtpPayload buffer with seqNum %d\n", (uint32_t)buffer->getMetaData());

	List< sp<Buffer> >::iterator itr = mQueue.begin();
	while (itr != mQueue.end() && (uint32_t)(*itr)->getMetaData() < seqNum) {
		++itr;
	}

	if (itr != mQueue.end() && (uint32_t)(*itr)->getMetaData() == seqNum) {
		return;
	}

	mQueue.insert(itr, buffer);

	mAssembler->processMediaData(mQueue);
}
