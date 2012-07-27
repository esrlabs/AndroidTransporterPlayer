#ifndef RTCP_H_
#define RTCP_H_

#include <stdint.h>
#include "mindroid/lang/String.h"

namespace mindroid {
class Buffer;
}
namespace rtcp {

class Rtcp {
public:
    enum RTCPType {
        SR = 200,   // Sender report, for transmission and reception statistics from participants that are active senders
        RR = 201,   // Receiver report, for reception statistics from participants that are not active senders
        SDES = 202, // Source description items, including CNAME
        BYE = 203,  // Indicates end of participation
        APP = 204   // Application specific functions
    };
    static const uint8_t VERSION = 2;

    struct SenderReport : public mindroid::Ref {
        uint8_t version;
        uint8_t padding;
        uint8_t rrcount;
        uint8_t packetType;
        uint16_t length;
        uint32_t ssrc;
        uint64_t ntpTimestamp;
        uint32_t rtpTimestamp;
        uint32_t senderPacketCount;
        uint32_t senderOctetCount;
    };

    static mindroid::sp<SenderReport> parseSenderReport(mindroid::sp<mindroid::Buffer> buffer);

    struct ReceiverReport : public mindroid::Ref {
        ReceiverReport(uint8_t rrc,
                       uint32_t ssrc,
                       uint32_t senderSrcc,
                       uint32_t totalLost,
                       uint32_t highestSeqNr)
            :   mReceptionReportCount(rrc),
                mSynchronizationSourceIdentifier(ssrc),
                mSenderSSRC(senderSrcc),
                mTotalLost(totalLost),
                mHighestSeqNr(highestSeqNr)
        {}

        void serialize(uint32_t* buffer, uint16_t length, int32_t lostSinceLastReport, uint32_t diffSeqNr);
        uint8_t mReceptionReportCount;
        uint32_t mLength;
        uint32_t mSynchronizationSourceIdentifier;
        uint32_t mSenderSSRC;
        uint32_t mTotalLost;
        uint32_t mHighestSeqNr;
        uint16_t completeLength() {
            return 8;//TODO ...this is for the basic case
        }
    };
    Rtcp ();

    virtual ~Rtcp ()
    {}

    void sendReceiverDescription();
private:

    uint32_t mTotalPacketsLost;
    mindroid::String mCname;

};
} //namespace rtcp


#endif // RTCP_H_
