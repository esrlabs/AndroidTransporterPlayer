#ifndef ANDROID_NET_SERVERSOCKET_H_
#define ANDROID_NET_SERVERSOCKET_H_

#include "android/os/Utils.h"
#include "android/net/Socket.h"
#include "android/os/Ref.h"
#include <sys/types.h>
#include <sys/socket.h>

namespace android {
namespace net {

class ServerSocket :
	public android::os::Ref
{
public:
	ServerSocket();
	ServerSocket(const uint16_t port);
	ServerSocket(const uint16_t port, const uint32_t backlog);
	virtual ~ServerSocket();
	bool bind(const uint16_t port);
	bool bind(const uint16_t port, const uint32_t backlog);
	android::os::sp<Socket> accept();
	void close();
	bool isBound() const { return mIsBound; }
	bool isClosed() const { return mIsClosed; }

private:
	static const uint16_t DEFAULT_BACKLOG = 10;

	int32_t mServerSocket;
	bool mIsBound;
	bool mIsClosed;

	NO_COPY_CTOR_AND_ASSIGNMENT_OPERATOR(ServerSocket)
};

} /* namespace net */
} /* namespace android */

#endif /* ANDROID_NET_SERVERSOCKET_H_ */
