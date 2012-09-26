/*
 * Copyright (C) 2010 The Android Open Source Project
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
 *
 * Additions and refactorings by E.S.R.Labs GmbH
 */

#ifndef UTILS_H_
#define UTILS_H_

#include "mindroid/os/Ref.h"

namespace mindroid {
class String;
class Buffer;
}

using mindroid::sp;

class Utils {
public:
	static sp<mindroid::Buffer> hexStringToByteArray(const mindroid::String& hexString);
};

#endif /* UTILS_H_ */
