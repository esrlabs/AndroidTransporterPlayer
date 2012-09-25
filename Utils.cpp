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

sp<Buffer> Utils::decodeBase64(const String& base64String) {
    if ((base64String.size() % 4) != 0) {
        return NULL;
    }

    size_t n = base64String.size();
    size_t padding = 0;
    if (n >= 1 && base64String.c_str()[n - 1] == '=') {
        padding = 1;

        if (n >= 2 && base64String.c_str()[n - 2] == '=') {
            padding = 2;
        }
    }

    size_t outLen = 3 * base64String.size() / 4 - padding;

    sp<Buffer> buffer = new Buffer(outLen);

    uint8_t *out = buffer->data();
    size_t j = 0;
    uint32_t accum = 0;
    for (size_t i = 0; i < n; ++i) {
        char c = base64String.c_str()[i];
        unsigned value;
        if (c >= 'A' && c <= 'Z') {
            value = c - 'A';
        } else if (c >= 'a' && c <= 'z') {
            value = 26 + c - 'a';
        } else if (c >= '0' && c <= '9') {
            value = 52 + c - '0';
        } else if (c == '+') {
            value = 62;
        } else if (c == '/') {
            value = 63;
        } else if (c != '=') {
            return NULL;
        } else {
            if (i < n - padding) {
                return NULL;
            }

            value = 0;
        }

        accum = (accum << 6) | value;

        if (((i + 1) % 4) == 0) {
            out[j++] = (accum >> 16);

            if (j < outLen) { out[j++] = (accum >> 8) & 0xff; }
            if (j < outLen) { out[j++] = accum & 0xff; }

            accum = 0;
        }
    }

    return buffer;
}
