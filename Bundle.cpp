#include "Bundle.h"
#include "android/os/Handler.h"

using namespace android::os;
using namespace android::lang;

Bundle::Bundle(sp<Handler> arg1, String arg2) :
	arg1(arg1),
	arg2(arg2) {
}

Bundle::~Bundle() {
}
