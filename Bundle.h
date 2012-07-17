#ifndef BUNDLE_H_
#define BUNDLE_H_

#include "android/os/Ref.h"
#include "android/lang/String.h"

using android::os::sp;

namespace android {
namespace os {
class Handler;
class Message;
}
}

class Bundle {
public:
	Bundle(sp<android::os::Handler> arg1, android::lang::String arg2, sp<android::os::Message> reply);
	virtual ~Bundle();

	sp<android::os::Handler> arg1;
	android::lang::String arg2;
	sp<android::os::Message> reply;
};

#endif /* BUNDLE_H_ */
