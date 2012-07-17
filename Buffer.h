#ifndef BUFFER_H_
#define BUFFER_H_

#include <sys/types.h>
#include <stdint.h>
#include "android/os/Ref.h"

class Buffer :
	public android::os::Ref {
public:
	Buffer(size_t capacity);
	Buffer(void* data, size_t capacity);
	virtual ~Buffer();

	uint8_t* base() { return (uint8_t *)mData; }
	uint8_t* data() { return (uint8_t *)mData + mOffset; }
	size_t capacity() const { return mCapacity; }
	size_t size() const { return mSize; }
	size_t offset() const { return mOffset; }
	void setRange(size_t offset, size_t size);

	void setMetaData(int32_t metaData) { mMetaData = metaData; }
	int32_t getMetaData() const { return mMetaData; }

private:
	void* mData;
	size_t mCapacity;
	size_t mOffset;
	size_t mSize;
	int32_t mMetaData;
};

#endif /* BUFFER_H_ */
