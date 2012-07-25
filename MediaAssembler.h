#ifndef MEDIAASSEMBLER_H_
#define MEDIAASSEMBLER_H_

#include "mindroid/os/Ref.h"

class MediaAssembler :
	public mindroid::Ref
{
public:
	virtual void processMediaQueue() = 0;
};

#endif /* MEDIAASSEMBLER_H_ */
