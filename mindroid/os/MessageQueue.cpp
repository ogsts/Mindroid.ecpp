/*
 * Copyright (C) 2011 Daniel Himmelein
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

#include <pthread.h>
#include "mindroid/os/MessageQueue.h"
#include "mindroid/os/Message.h"
#include "mindroid/os/Clock.h"
#include "mindroid/os/Lock.h"
#include "mindroid/os/Runnable.h"

namespace mindroid {

MessageQueue::MessageQueue() :
	mHeadMessage(NULL),
	mCondVar(mCondVarLock),
	mLockMessageQueue(false) {
}

MessageQueue::~MessageQueue() {
}

bool MessageQueue::enqueueMessage(Message& message, uint64_t execTimestamp) {
	AutoLock autoLock(mCondVarLock);
	if (mLockMessageQueue) {
		return false;
	}
	{
		AutoLock autoLock(message.mLock);
		if (message.mExecTimestamp != 0) {
			return false;
		}
		message.mExecTimestamp = execTimestamp;
	}
	if (message.mHandler == NULL) {
		mLockMessageQueue = true;
	}
	Message* curMessage = mHeadMessage;
	if (curMessage == NULL || execTimestamp < curMessage->mExecTimestamp) {
		message.mNextMessage = curMessage;
		mHeadMessage = &message;
		mCondVar.notify();
	} else {
		Message* prevMessage = NULL;
		while (curMessage != NULL && curMessage->mExecTimestamp <= execTimestamp) {
			prevMessage = curMessage;
			curMessage = curMessage->mNextMessage;
		}
		message.mNextMessage = prevMessage->mNextMessage;
		prevMessage->mNextMessage = &message;
		mCondVar.notify();
	}
	return true;
}

Message& MessageQueue::dequeueMessage() {
	while (true) {
		AutoLock autoLock(mCondVarLock);
		uint64_t now = Clock::monotonicTime();
		Message* message = getNextMessage(now);
		if (message != NULL) {
			return *message;
		}

		if (mHeadMessage != NULL) {
			if (mHeadMessage->mExecTimestamp - now > 0) {
				timespec absExecTimestamp;
				absExecTimestamp.tv_sec = mHeadMessage->mExecTimestamp / 1000000000LL;
				absExecTimestamp.tv_nsec = mHeadMessage->mExecTimestamp % 1000000000LL;
				mCondVar.wait(absExecTimestamp);
			}
		} else {
			mCondVar.wait();
		}
	}
}

Message* MessageQueue::getNextMessage(uint64_t now) {
	Message* message = mHeadMessage;
	if (message != NULL) {
		if (now >= message->mExecTimestamp) {
			mHeadMessage = message->mNextMessage;
			return message;
		}
	}
	return NULL;
}

bool MessageQueue::removeMessages(Handler* handler) {
	if (handler == NULL) {
		return false;
	}

	bool foundMessage = false;

	mCondVarLock.lock();

	Message* curMessage = mHeadMessage;
	// remove all matching messages at the front of the message queue.
	while (curMessage != NULL && curMessage->mHandler == handler) {
		foundMessage = true;
		Message* nextMessage = curMessage->mNextMessage;
		mHeadMessage = nextMessage;
		curMessage->recycle();
		curMessage = nextMessage;
	}

	// remove all matching messages after the front of the message queue.
	while (curMessage != NULL) {
		Message* nextMessage = curMessage->mNextMessage;
		if (nextMessage != NULL) {
			if (nextMessage->mHandler == handler) {
				foundMessage = true;
				Message* nextButOneMessage = nextMessage->mNextMessage;
				nextMessage->recycle();
				curMessage->mNextMessage = nextButOneMessage;
				continue;
			}
		}
		curMessage = nextMessage;
	}

	mCondVarLock.unlock();

	return foundMessage;
}

bool MessageQueue::removeMessage(Handler* handler, Message* message) {
	if (handler == NULL || message == NULL) {
		return false;
	}

	bool foundMessage = false;

	mCondVarLock.lock();

	Message* curMessage = mHeadMessage;
	// remove a matching message at the front of the message queue.
	if (curMessage != NULL && curMessage->mHandler == handler && curMessage == message) {
		foundMessage = true;
		Message* nextMessage = curMessage->mNextMessage;
		mHeadMessage = nextMessage;
		curMessage->recycle();
		curMessage = nextMessage;
	}

	// remove a matching message after the front of the message queue.
	if (!foundMessage) {
		while (curMessage != NULL) {
			Message* nextMessage = curMessage->mNextMessage;
			if (nextMessage != NULL) {
				if (nextMessage->mHandler == handler && nextMessage == message) {
					foundMessage = true;
					Message* nextButOneMessage = nextMessage->mNextMessage;
					nextMessage->recycle();
					curMessage->mNextMessage = nextButOneMessage;
					break;
				}
			}
			curMessage = nextMessage;
		}
	}

	mCondVarLock.unlock();

	return foundMessage;
}

bool MessageQueue::removeMessages(Handler* handler, int16_t what) {
	if (handler == NULL) {
		return false;
	}

	bool foundMessage = false;

	mCondVarLock.lock();

	Message* curMessage = mHeadMessage;
	// remove all matching messages at the front of the message queue.
	while (curMessage != NULL && curMessage->mHandler == handler && curMessage->what == what) {
		foundMessage = true;
		Message* nextMessage = curMessage->mNextMessage;
		mHeadMessage = nextMessage;
		curMessage->recycle();
		curMessage = nextMessage;
	}

	// remove all matching messages after the front of the message queue.
	while (curMessage != NULL) {
		Message* nextMessage = curMessage->mNextMessage;
		if (nextMessage != NULL) {
			if (nextMessage->mHandler == handler && nextMessage->what == what) {
				foundMessage = true;
				Message* nextButOneMessage = nextMessage->mNextMessage;
				nextMessage->recycle();
				curMessage->mNextMessage = nextButOneMessage;
				continue;
			}
		}
		curMessage = nextMessage;
	}

	mCondVarLock.unlock();

	return foundMessage;
}

} /* namespace mindroid */
