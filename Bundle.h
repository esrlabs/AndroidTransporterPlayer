#ifndef BUNDLE_H_
#define BUNDLE_H_

#include "android/lang/String.h"

namespace android {
namespace os {
class Handler;
class Message;
}
}

class Bundle {
public:
	Bundle(android::os::sp<android::os::Handler> arg1, android::lang::String arg2, android::os::sp<android::os::Message> reply);
	virtual ~Bundle();

	android::os::sp<android::os::Handler> arg1;
	android::lang::String arg2;
	android::os::sp<android::os::Message> reply;
};

#endif /* BUNDLE_H_ */
