#include "Buffer.h"
#include <stdlib.h>
#include <assert.h>

Buffer::Buffer(size_t capacity) :
	mData(malloc(capacity)),
	mCapacity(capacity),
	mOffset(0),
	mSize(capacity),
	mMetaData(0) {
}

Buffer::Buffer(void* data, size_t capacity) :
	mData(data),
	mCapacity(capacity),
	mOffset(0),
	mSize(capacity),
	mMetaData(0) {
}

Buffer::~Buffer() {
	if (mData != NULL) {
		free(mData);
		mData = NULL;
	}
}

void Buffer::setRange(size_t offset, size_t size) {
    assert(offset <= mCapacity);
    assert(offset + size <= mCapacity);

    mOffset = offset;
    mSize = size;
}
