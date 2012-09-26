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

#include "Utils.h"
#include "mindroid/lang/String.h"
#include "mindroid/util/Buffer.h"

using namespace mindroid;

sp<Buffer> Utils::hexStringToByteArray(const String& hexString) {
	size_t byteArraySize = hexString.size() / 2;
	sp<Buffer> byteArray = new Buffer(byteArraySize);
	for(size_t i = 0; i < hexString.size(); i += 2) {
		byteArray->data()[i / 2] = (uint8_t) strtol(hexString.substr(i, i + 2), NULL, 16);
	}
	return byteArray;
}
