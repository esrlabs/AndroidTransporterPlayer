#include "gtest/gtest.h"
#include "rtcp/Rtcp.h"
#include <stdio.h>

using namespace audio;

class RtcpTest : public ::testing::Test
{

public:
	virtual void SetUp()
	{
	}
	 
	virtual void TearDown()
    {
    }
};

TEST_F(RtcpTest, serialize)
{
    Rtcp rtcp;
    int receptionCount = 23;
    int ssrc = 22;
    int senderSsrc = 44;
    int totalLost = 3;
    int highestSeqNr = 3456;
        
    Rtcp::ReceiverReport receiverReport(receptionCount, ssrc, senderSsrc, totalLost, highestSeqNr);
    uint32_t buffer[100];
    receiverReport.serialize(buffer, 100, 0, 100);
//        31,30,29,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1
// header |V=2 |P |    RC        |   PT=RR=201           |             length                  |
    uint32_t header = buffer[0];
    printf("header: 0x%X\n", header);
	EXPECT_EQ(2, (header >> 30) & 0b11) << "version";
	EXPECT_EQ(0, (header >> 29) & 0b1) << "padding";
	EXPECT_EQ(receptionCount, (header >> 24) & 0b11111) << "reception count";
	EXPECT_EQ(Rtcp::RR, (header >> 16) & 0xFF) << "RTCP type";
	EXPECT_EQ(0, (buffer[3] >> 24) & 0xFF) << "fraction lost";
	EXPECT_EQ(totalLost, (buffer[3]) & 0xFFFFFF) << "total lost";
	EXPECT_EQ(highestSeqNr, (buffer[4])) << "highest sequence number";
}

