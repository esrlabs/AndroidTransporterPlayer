#ifndef MEDIAASSEMBLER_H_
#define MEDIAASSEMBLER_H_

#include "android/os/Ref.h"

class MediaAssembler :
	public android::os::Ref
{
public:
	virtual void processMediaQueue() = 0;
};

#endif /* MEDIAASSEMBLER_H_ */
