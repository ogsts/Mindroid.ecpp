/*
 * Copyright (C) 2013 Daniel Himmelein
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MINDROID_TSSPSCCIRCULARBUFFER_H_
#define MINDROID_TSSPSCCIRCULARBUFFER_H_

#include <stdint.h>
#include <string.h>
#include "mindroid/os/AtomicInteger.h"
#include "mindroid/util/Utils.h"

namespace mindroid {

// Thread-safe (Lock-Free) Single-Producer Single-Consumer Circular Buffer: http://www.codeproject.com/Articles/43510/Lock-Free-Single-Producer-Single-Consumer-Circular
template<uint16_t SIZE>
class TsSpScCircularBuffer
{
public:
	TsSpScCircularBuffer() :
			mReadIndex(0),
			mWriteIndex(0) {
	}

	int32_t front(void* data, uint16_t size);
	int32_t pop(void* data, uint16_t size);
	bool push(const void* data, uint16_t size);
	uint16_t size() const;

	bool empty() const {
		return mReadIndex.get() == mWriteIndex.get();
	}

	bool full() const {
		return (mWriteIndex.get() + 1) % SIZE == mReadIndex.get();
	}

protected:
	void readData(uint16_t readIndex, uint8_t* data, uint16_t size);
	void writeData(uint16_t writeIndex, const uint8_t* data, uint16_t size);

private:
	uint8_t mBuffer[SIZE];
	AtomicInteger<uint16_t> mReadIndex;
	AtomicInteger<uint16_t> mWriteIndex;

	NO_COPY_CTOR_AND_ASSIGNMENT_OPERATOR(TsSpScCircularBuffer)
};

template<uint16_t SIZE>
int32_t TsSpScCircularBuffer<SIZE>::front(void* data, uint16_t size) {
	uint16_t readIndex = mReadIndex.get();
	uint16_t writeIndex = mWriteIndex.get();

	if (readIndex == writeIndex) {
		return 0;
	}

	uint16_t dataSize;
	readData(readIndex, (uint8_t*) &dataSize, 2);
	if (data != NULL) {
		if (size >= dataSize) {
			readData(readIndex + 2, (uint8_t*) data, dataSize);
			return dataSize;
		} else {
			return -dataSize;
		}
	} else {
		return dataSize;
	}
}

template<uint16_t SIZE>
int32_t TsSpScCircularBuffer<SIZE>::pop(void* data, uint16_t size) {
	uint16_t readIndex = mReadIndex.get();
	uint16_t writeIndex = mWriteIndex.get();

	if (readIndex == writeIndex) {
		return 0;
	}

	uint16_t dataSize;
	readData(readIndex, (uint8_t*) &dataSize, 2);
	if (data != NULL) {
		if (size >= dataSize) {
			uint16_t newReadIndex = (readIndex + dataSize + 2) % SIZE;
			readData(readIndex + 2, (uint8_t*) data, dataSize);
			mReadIndex.set(newReadIndex);
			return dataSize;
		} else {
			return -dataSize;
		}
	} else {
		uint16_t newReadIndex = (readIndex + dataSize + 2) % SIZE;
		mReadIndex.set(newReadIndex);
		return dataSize;
	}
}

template<uint16_t SIZE>
bool TsSpScCircularBuffer<SIZE>::push(const void* data, uint16_t size) {
	if ((data == NULL) || ((size + 2) >= SIZE)) {
		return false;
	}

	uint16_t readIndex = mReadIndex.get();
	uint16_t writeIndex = mWriteIndex.get();

	bool hasFreeSpace;
	if (writeIndex >= readIndex) {
		hasFreeSpace = (SIZE - (writeIndex - readIndex) > size);
	} else {
		hasFreeSpace = (readIndex - writeIndex > size);
	}

	if (hasFreeSpace) {
		uint16_t newWriteIndex = (writeIndex + size + 2) % SIZE;
		writeData(writeIndex, (uint8_t*) &size, 2);
		writeData(writeIndex + 2, (uint8_t*) data, size);
		mWriteIndex.set(newWriteIndex);
		return true;
	} else {
		return false;
	}
}

template<uint16_t SIZE>
uint16_t TsSpScCircularBuffer<SIZE>::size() const {
	uint16_t readIndex = mReadIndex.get();
	uint16_t writeIndex = mWriteIndex.get();

	if (writeIndex >= readIndex) {
		return (SIZE - (writeIndex - readIndex));
	} else {
		return (readIndex - writeIndex);
	}
}

template<uint16_t SIZE>
void TsSpScCircularBuffer<SIZE>::readData(uint16_t readIndex, uint8_t* data, uint16_t size) {
	if (readIndex + size < SIZE) {
		memcpy(data, mBuffer + readIndex, size);
	} else {
		uint16_t partialSize = (SIZE - readIndex);
		memcpy(data, mBuffer + readIndex, partialSize);
		memcpy(data + partialSize, mBuffer, size - partialSize);
	}
}

template<uint16_t SIZE>
void TsSpScCircularBuffer<SIZE>::writeData(uint16_t writeIndex, const uint8_t* data, uint16_t size) {
	if (writeIndex + size < SIZE) {
		memcpy(mBuffer + writeIndex, data, size);
	} else {
		uint16_t partialSize = (SIZE - writeIndex);
		memcpy(mBuffer + writeIndex, data, partialSize);
		memcpy(mBuffer, data + partialSize, size - partialSize);
	}
}

} /* namespace mindroid */

#endif /* MINDROID_TSSPSCCIRCULARBUFFER_H_ */