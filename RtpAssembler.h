#ifndef RTPASSEMBLER_H_
#define RTPASSEMBLER_H_

#include "android/os/Ref.h"

class RtpAssembler :
	public android::os::Ref
{
public:
	virtual void processMediaData() = 0;
};

#endif /* RTPASSEMBLER_H_ */
