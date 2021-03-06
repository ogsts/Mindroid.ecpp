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

#include "mindroid/os/SystemClock.h"
#include "mindroid/util/concurrent/locks/Lock.h"

namespace mindroid {

uint64_t SystemClock::sMonotonicTime = 0;

uint64_t SystemClock::monotonicTime() {
	AutoLock autoLock;
	return sMonotonicTime;
}

uint64_t SystemClock::realTime() {
	return monotonicTime();
}

void SystemClock::tick(uint32_t duration) {
	AutoLock autoLock;
	sMonotonicTime += duration;
}

} /* namespace mindroid */
