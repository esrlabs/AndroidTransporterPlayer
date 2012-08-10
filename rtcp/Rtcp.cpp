#include "rtcp/Rtcp.h"
#include "Bitstream.h"
#include "mindroid/util/Buffer.h"
#include "mindroid/os/Message.h"
#include "mindroid/os/Clock.h"
#include <stdio.h>

using namespace mindroid;

namespace rtcp {

static const uint32_t kSourceID = 0xdeadbeef;

Rtcp::Rtcp(sp<Message> notifyRtcpSR) :
    mBaseSeq(0),
    mMaxSeq(0),
    mSeqWrap(0),
    mTotalPacketsLost(0),
    mReceivedPackets(0),
    mCumulativeLost(0),
    mAvgRtcpSize(8*4), // set to expected packet size we will first send as receiver
    mNotifyRtcpSR(notifyRtcpSR)
{
}

//         0                   1                   2                   3
//         0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// header |V=2|P|    RC   |   PT=SR=200   |             length            |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                         SSRC of sender                        |
//        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// sender |              NTP timestamp, most significant word             |
// info   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |             NTP timestamp, least significant word             |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                         RTP timestamp                         |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                     sender's packet count                     |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                      sender's octet count                     |
//        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//

static uint16_t u16at(const uint8_t *data) {
    return data[0] << 8 | data[1];
}

static uint32_t u32at(const uint8_t *data) {
    return u16at(data) << 16 | u16at(&data[2]);
}

static uint64_t u64at(const uint8_t *data) {
    return (uint64_t)(u32at(data)) << 32 | u32at(&data[4]);
}
void Rtcp::onPacketReceived(mindroid::sp<mindroid::Buffer> data) {
    uint8_t* buffer = data->data();
    size_t packetSize = data->size();
    updateAvg(packetSize);
    uint32_t const minReportSize = 7*4;
    uint32_t const reportBlockSize = 6*4;
    if (packetSize < minReportSize) {
        return;
    }

    BitstreamReader bs(buffer, packetSize);
    uint8_t version = bs.get<2>();
    if (version != 2) {
        return;
    }
    uint8_t padding = bs.get<1>();
    uint8_t rrcount = bs.get<5>();
    if (rrcount) {
        printf("senderreport contains information from more sources, will be discarded\n");
    }
    if (packetSize < minReportSize + reportBlockSize*rrcount) {
        return;
    }
    uint8_t packetType = bs.get<8>();

    switch (packetType) {
        case RTCP_SR: {
            parseSR(bs, padding, rrcount);
            break;
        }
        case RTCP_RR:  
        case RTCP_SDES:
        case RTCP_APP:
            break;
        case RTCP_BYE: {
            parseBYE(bs, padding, rrcount);
            break;
        }
        default: {
            printf("Unknown RTCP packet type %d\n", packetType);
            break;
        }
    }
}

void Rtcp::parseSR(BitstreamReader& bs, uint8_t padding, uint8_t rrcount) {
    const uint8_t PACKET_SIZE = 8 * 4; // size of RR packet with one source in bytes
    if (PACKET_SIZE > bs.size()) {
        printf("RTCP buffer too small to accomodate RR.\n");
        return;
    }
    SenderReport* sr = new SenderReport();
    sr->version = 2;
    sr->padding = padding;
    sr->rrcount = rrcount;

    sr->length = bs.get<16>();
    sr->ssrc = bs.get<32>();
    sr->ntpTimestamp = bs.get<64>();
    sr->rtpTimestamp = bs.get<32>();
    sr->senderPacketCount = bs.get<32>();
    sr->senderOctetCount = bs.get<32>();

    sp<Message> msg = mNotifyRtcpSR->dup();
    sp<Bundle> bundle = msg->metaData();
    sp<Buffer> buffer(new Buffer(PACKET_SIZE));
    bundle->putObject("RTCP-SR-Packet", buffer);
    msg->sendToTarget();
}

void Rtcp::parseBYE(BitstreamReader& bs, uint8_t padding, uint8_t rrcount) {
}

//         0                   1                   2                   3
//         0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// header |V=2|P|    RC   |   PT=RR=201   |             length            |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                     SSRC of packet sender                     |
//        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// report |                 SSRC_1 (SSRC of first source)                 |
// block  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   1    | fraction lost |       cumulative number of packets lost       |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |           extended highest sequence number received           |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                      interarrival jitter                      |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                         last SR (LSR)                         |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                   delay since last SR (DLSR)                  |
//        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// report |                 SSRC_2 (SSRC of second source)                |
// block  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   2    :                               ...                             :
//        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//        |                  profile-specific extensions                  |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
void Rtcp::addReceiverReport(const sp<Buffer>& buffer,
                             uint32_t ssrc_1,
                             uint64_t lastNTPTime,
                             uint64_t lastNTPTimeUpdateUs,
                             uint32_t receivedPackets,
                             uint16_t highestSeqNrReceived) {
    static bool firstPacketReceived = false;
    uint32_t packetsExpectedPrior = mPacketsExpected;
    uint32_t mReceivedPacketsPrior = mReceivedPackets;
    mReceivedPackets = receivedPackets;
    mPacketsExpected = packetsExpected();
    uint8_t fractionLost = calcFractionLost(mPacketsExpected, packetsExpectedPrior, mReceivedPackets, mReceivedPacketsPrior);
    uint32_t cumulativeLost = getCumulativeLost(mPacketsExpected, packetsExpectedPrior, mReceivedPackets, mReceivedPacketsPrior);
    if (!firstPacketReceived){
        firstPacketReceived = true;
        mBaseSeq = highestSeqNrReceived;
    }
    if (mMaxSeq > highestSeqNrReceived) {
        //wrap around
        ++mSeqWrap;
    }
    mMaxSeq = highestSeqNrReceived;
    const uint8_t PACKET_SIZE = 8 * 4; // size of RR packet with one source in bytes
    if (PACKET_SIZE > buffer->size()) {
        printf("RTCP buffer too small to accomodate RR.\n");
        return;
    }

    updateAvg(PACKET_SIZE);
    uint8_t *data = buffer->data();
    BitstreamWriter bs(data, buffer->size());
    bs.put<2>(0b10);// version
    bs.put<1>(0);// no padding
    bs.put<5>(1);// number of reception report blocks
    bs.put<8>(Rtcp::RTCP_RR);
    bs.put<16>(8 - 1); //length of this RTCP packet in 32-bit words minus one
    bs.put<32>(kSourceID);
    bs.put<32>(ssrc_1);
    bs.put<8>(fractionLost);
    bs.put<24>(cumulativeLost);
    bs.put<32>(maxPacketReceived());
    uint32_t interarrivalJitter = 0;//TODO
    bs.put<32>(interarrivalJitter);
    uint32_t LRS = 0;
    uint32_t DLRS = 0;
    if (lastNTPTime != 0) {
        // middle 32 bits out of 64 in the previous NTP timestamp from last sender reports, data, headerLength
        LRS = (lastNTPTime >> 16) & 0xffffffff;
        DLRS = (uint32_t)((Clock::realTime() - lastNTPTimeUpdateUs) * 65536.0 / 1E6);
    }
    bs.put<32>(LRS);
    bs.put<32>(DLRS);
    buffer->setRange(buffer->offset() + PACKET_SIZE, buffer->size() - PACKET_SIZE);
}


} //namespace rtcp

