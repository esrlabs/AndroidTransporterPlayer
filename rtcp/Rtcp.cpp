#include "rtcp/Rtcp.h"
#include "mindroid/util/Buffer.h"
#include <stdio.h>

using namespace mindroid;

namespace rtcp {

Rtcp::Rtcp() :
    mTotalPacketsLost(0)
{}

// 6.4.1 SR: Sender Report RTCP Packet
//
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
sp<Rtcp::SenderReport> Rtcp::parseSenderReport(mindroid::sp<mindroid::Buffer> data) {
    sp<Rtcp::SenderReport> result;
    uint8_t* buffer = data->data();
    size_t size = data->size();
    uint32_t const minReportSize = 7*4;
    uint32_t const reportBlockSize = 6*4;
    if (size < minReportSize) {
        return result;
    }
    SenderReport* sr = new SenderReport();
    sp<Rtcp::SenderReport> srCleanup(sr);
    int i = 0;
    sr->version = (buffer[0] >> 6) & 0b11;
    if (sr->version != 2) {
        return result;
    }
    sr->padding = (buffer[0] >> 5) & 0b1;
    uint8_t rrcount = buffer[0] & 0b11111;
    sr->rrcount = rrcount;
    if (size < minReportSize + reportBlockSize*rrcount) {
        return result;
    }
    sr->packetType = buffer[1];
    sr->length = buffer[2] << 8 | buffer[3];
    sr->ssrc = buffer[4] << 24 | buffer[5] << 16 | buffer[6] << 8 | buffer[7];
    sr->ntpTimestamp = static_cast<uint64_t>(buffer[8]) << 56
                       | static_cast<uint64_t>(buffer[9]) << 48
                       | static_cast<uint64_t>(buffer[10]) << 40
                       | static_cast<uint64_t>(buffer[11]) << 32
                       | static_cast<uint64_t>(buffer[12]) << 24
                       | static_cast<uint64_t>(buffer[13]) << 16
                       | static_cast<uint64_t>(buffer[14]) << 8
                       | static_cast<uint64_t>(buffer[15]);
    sr->rtpTimestamp = buffer[16] << 24 | buffer[17] << 16 | buffer[18] << 8 | buffer[19];
    sr->senderPacketCount = buffer[20] << 24 | buffer[21] << 16 | buffer[22] << 8 | buffer[23];
    sr->senderOctetCount = buffer[24] << 24 | buffer[25] << 16 | buffer[26] << 8 | buffer[27];
    // for (int i = 0; i < rrcount; i++)
    // {
    // uint32_t senderPacketCount = buffer[5];
    // }
    result = srCleanup;
    return result;
}

// 6.4.2 RR: Receiver Report RTCP Packet
//
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
void Rtcp::ReceiverReport::serialize(uint32_t* buffer, uint16_t length, int32_t lostSinceLastReport, uint32_t diffSeqNr) {
    memset(buffer, 0, 4*length);
    uint8_t padding = 0;
    uint32_t header = VERSION << 30
                      | padding << 29
                      | (mReceptionReportCount & 0b11111) << 24
                      | (uint8_t)RR << 16
                      | completeLength();
    buffer[0] = header;
    buffer[1] = mSenderSSRC;
    buffer[2] = mSynchronizationSourceIdentifier;
    uint8_t fractionLost = lostSinceLastReport < 0
                           ? 0
                           : (float)lostSinceLastReport/diffSeqNr * 256;
    buffer[3] = buffer[3] | fractionLost << 23;
    buffer[3] = buffer[3] | mTotalLost & 0x00FFFFFF;
    buffer[4] = mHighestSeqNr & 0x0000FFFF; //upper 16 not set TODO
}

void Rtcp::sendReceiverDescription() {
}

} //namespace rtcp

