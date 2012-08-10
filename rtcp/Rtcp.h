#ifndef RTCP_H_
#define RTCP_H_

#include <stdint.h>
#include "mindroid/lang/String.h"
#include "mindroid/os/Ref.h"
#include <stdio.h>

class BitstreamReader;
namespace mindroid {
class Buffer;
class Message;
}
namespace rtcp {

class RtcpSessionState {
public:
    RtcpSessionState ()
        : mTp(0),
          mTn(0),
          mPmembers(0),
          mSenders(0),
          mWeSent(false),
          mInitial(true)
    {}

private:

    uint64_t mTp;// the last time an RTCP packet was transmitted;
    uint64_t mTn;// the next scheduled transmission time of an RTCP packet;
    uint16_t mPmembers;// the estimated number of session members at the time tn was last recomputed;
    uint16_t mMembers;// the most current estimate for the number of session members;
    uint16_t mSenders;// the most current estimate for the number of senders in the session;
    uint32_t mRtcpBw;// The target RTCP bandwidth
    bool mWeSent;// true if the application has sent data since the 2nd previous RTCP report was transmitted.
    bool mInitial;// true if the application has not yet sent an RTCP packet.
};

class Rtcp :
		public mindroid::Ref
{
public:
    enum RTCPType {
        RTCP_SR = 200,   // Sender report
        RTCP_RR = 201,   // Receiver report
        RTCP_SDES = 202, // Source description items, including CNAME
        RTCP_BYE = 203,  // Indicates end of participation
        RTCP_APP = 204   // Application specific functions
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

    void onPacketReceived(mindroid::sp<mindroid::Buffer> buffer);

    Rtcp(mindroid::sp<mindroid::Message> notifyRtcpSR);

    virtual ~Rtcp ()
    {}

    void addReceiverReport(const mindroid::sp<mindroid::Buffer>& buffer,
                           uint32_t ssrc_1,
                           uint64_t lastNTPTime,
                           uint64_t lastNTPTimeUpdateUs,
                           uint32_t receivedPackets,
                           uint16_t highestSeqNrReceived);
private:

    void parseSR(BitstreamReader& bs, uint8_t padding, uint8_t rrcount);
    void parseBYE(BitstreamReader& bs, uint8_t padding, uint8_t rrcount);

    void updateAvg(uint32_t packetSize) {
        mAvgRtcpSize = (1/16) * (float)packetSize + (15/16) * mAvgRtcpSize;
    }

    uint8_t getCumulativeLost(uint32_t expected, uint32_t expectedPrior, uint32_t received, uint32_t receivedPrior) {
        uint32_t expectedInInterval = expected - expectedPrior;
        uint32_t receivedInInterval = received - receivedPrior;
        uint32_t lost = expectedInInterval - receivedInInterval;
        mCumulativeLost += lost;
        return mCumulativeLost;
    }

    // fraction of RTP data packets from source SSRC_n lost since the previous SR packet was sent
    uint8_t calcFractionLost(uint32_t expected, uint32_t expectedPrior, uint32_t received, uint32_t receivedPrior) const
    {
        uint32_t expectedInInterval = expected - expectedPrior;
        uint32_t receivedInInterval = received - receivedPrior;
        uint32_t lost = expectedInInterval - receivedInInterval;
        return (expectedInInterval == 0 || lost <= 0)
            ? 0
            : (lost << 8)/expectedInInterval;
    }
    
    uint32_t maxPacketReceived() {
        return mSeqWrap << 16 | mMaxSeq;
    }

    uint32_t packetsExpected() {
        return maxPacketReceived() - mBaseSeq + 1;
    }

    int32_t packetsLost() {
        return packetsExpected() - mReceivedPackets;
    }

    uint16_t mBaseSeq;
    uint16_t mMaxSeq;
    uint16_t mSeqWrap;
    uint32_t mTotalPacketsLost;
    uint32_t mReceivedPackets;
    uint32_t mPacketsExpected;
    uint32_t mCumulativeLost;
    float mAvgRtcpSize;
    mindroid::String mCname;
    mindroid::sp<mindroid::Message> mNotifyRtcpSR;

	NO_COPY_CTOR_AND_ASSIGNMENT_OPERATOR(Rtcp)

};
} //namespace rtcp


#endif // RTCP_H_
