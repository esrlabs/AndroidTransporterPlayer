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

#include "CsdUtils.h"
#include "BitReader.h"
#include "mindroid/lang/String.h"
#include "mindroid/util/Buffer.h"

using namespace mindroid;

sp<Buffer> CsdUtils::hexStringToByteArray(const sp<String>& hexString) {
	size_t byteArraySize = hexString->size() / 2;
	sp<Buffer> byteArray = new Buffer(byteArraySize);
	for(size_t i = 0; i < hexString->size(); i += 2) {
		byteArray->data()[i / 2] = (uint8_t) strtol(hexString->substr(i, i + 2)->c_str(), NULL, 16);
	}
	return byteArray;
}

sp<Buffer> CsdUtils::decodeBase64String(const sp<String>& string) {
    size_t size = string->size();
    if ((size % 4) != 0) {
        return NULL;
    }

    size_t padding = 0;
    if (size >= 1 && string->c_str()[size - 1] == '=') {
        padding = 1;

        if (size >= 2 && string->c_str()[size - 2] == '=') {
            padding = 2;
        }
    }

    size_t bufferSize = 3 * size / 4 - padding;
    sp<Buffer> buffer = new Buffer(bufferSize);
    uint8_t* rawBuffer = buffer->data();

    size_t j = 0;
    uint32_t accumulator = 0;
    for (size_t i = 0; i < size; ++i) {
        char c = string->c_str()[i];
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
            if (i < size - padding) {
                return NULL;
            }

            value = 0;
        }

        // Group every 4 base-64 characters together and realign them within 3 bytes (see http://www.kbcafe.com/articles/HowTo.Base64.pdf).
        accumulator = (accumulator << 6) | value;
        if (((i + 1) % 4) == 0) {
            rawBuffer[j++] = (accumulator >> 16);

            if (j < bufferSize) { rawBuffer[j++] = (accumulator >> 8) & 0xFF; }
            if (j < bufferSize) { rawBuffer[j++] = accumulator & 0xFF; }

            accumulator = 0;
        }
    }

    return buffer;
}

void CsdUtils::buildAvcCodecSpecificData(const sp<String>& strProfileId, const sp<String>& strSpropParamSet, sp<Buffer>* sps, sp<Buffer>* pps) {
	sp<Buffer> profileId = hexStringToByteArray(strProfileId);

	sp<Buffer> paramSets[8];
	size_t paramSetIndex = 0;
	size_t numSeqParameterSets = 0;
	size_t seqParameterSetSize = 0;
	size_t numPicParameterSets = 0;
	size_t picParameterSetSize = 0;

	size_t startPos = 0;
	for (;;) {
		ssize_t commaPos = strSpropParamSet->indexOf(",", startPos);
		size_t endPos = (commaPos < 0) ? strSpropParamSet->size() : commaPos;

		sp<String> strNalUnit = strSpropParamSet->substr(startPos, endPos);
		sp<Buffer> nalUnit = decodeBase64String(strNalUnit);
		uint8_t nalType = nalUnit->data()[0] & 0x1F;
		if (numSeqParameterSets == 0) {
			assert((unsigned)nalType == 7u);
		} else if (numPicParameterSets > 0) {
			assert((unsigned)nalType == 8u);
		}
		if (nalType == 7) {
			++numSeqParameterSets;
			seqParameterSetSize += nalUnit->size();
		} else  {
			assert((unsigned)nalType == 8u);
			++numPicParameterSets;
			picParameterSetSize += nalUnit->size();
		}

		paramSets[paramSetIndex] = nalUnit;
		paramSetIndex++;

		if (commaPos < 0 || paramSetIndex > 8) {
			break;
		}

		startPos = commaPos + 1;
	}

	assert(numSeqParameterSets >= 1 && numSeqParameterSets < 32);
	assert(numPicParameterSets >= 1 && numPicParameterSets < 256);

	// SPS
	*sps = new Buffer(1024);
	(*sps)->setRange(0, 0);
	for (size_t i = 0; i < numSeqParameterSets; ++i) {
		sp<Buffer> nalUnit = paramSets[i];
		memcpy((*sps)->data() + (*sps)->size(), "\x00\x00\x00\x01", 4);
		memcpy((*sps)->data() + (*sps)->size() + 4, nalUnit->data(), nalUnit->size());
		(*sps)->setRange(0, (*sps)->size() + 4 + nalUnit->size());

		if (i == 0) {
			int32_t width;
			int32_t height;
			getAvcDimensions(nalUnit, &width, &height);
		}
	}

	// PPS
	*pps = new Buffer(1024);
	(*pps)->setRange(0, 0);
	for (size_t i = 0; i < numPicParameterSets; ++i) {
		sp<Buffer> nalUnit = paramSets[i + numSeqParameterSets];
		memcpy((*pps)->data() + (*pps)->size(), "\x00\x00\x00\x01", 4);
		memcpy((*pps)->data() + (*pps)->size() + 4, nalUnit->data(), nalUnit->size());
		(*pps)->setRange(0, (*pps)->size() + 4 + nalUnit->size());
	}
}

void CsdUtils::getAvcDimensions(const sp<Buffer>& seqParamSet, int32_t* width, int32_t* height) {
	BitReader br(seqParamSet->data() + 1, seqParamSet->size() - 1);

    unsigned profile_idc = br.getBits(8);
    br.skipBits(16);
    parseUE(br); // seq_parameter_set_id

    unsigned chroma_format_idc = 1;  // 4:2:0 chroma format

    if (profile_idc == 100 || profile_idc == 110
            || profile_idc == 122 || profile_idc == 244
            || profile_idc == 44 || profile_idc == 83 || profile_idc == 86) {
        chroma_format_idc = parseUE(br);
        if (chroma_format_idc == 3) {
            br.skipBits(1); // residual_colour_transform_flag
        }
        parseUE(br); // bit_depth_luma_minus8
        parseUE(br); // bit_depth_chroma_minus8
        br.skipBits(1); // qpprime_y_zero_transform_bypass_flag
        br.getBits(1); // seq_scaling_matrix_present_flag
    }

    parseUE(br); // log2_max_frame_num_minus4
    unsigned pic_order_cnt_type = parseUE(br);

    if (pic_order_cnt_type == 0) {
        parseUE(br); // log2_max_pic_order_cnt_lsb_minus4
    } else if (pic_order_cnt_type == 1) {
        // offset_for_non_ref_pic, offset_for_top_to_bottom_field and
        // offset_for_ref_frame are technically se(v), but since we are
        // just skipping over them the midpoint does not matter.

        br.getBits(1); // delta_pic_order_always_zero_flag
        parseUE(br); // offset_for_non_ref_pic
        parseUE(br); // offset_for_top_to_bottom_field

        unsigned num_ref_frames_in_pic_order_cnt_cycle = parseUE(br);
        for (unsigned i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; ++i) {
            parseUE(br); // offset_for_ref_frame
        }
    }

    parseUE(br); // num_ref_frames
    br.getBits(1); // gaps_in_frame_num_value_allowed_flag

    unsigned pic_width_in_mbs_minus1 = parseUE(br);
    unsigned pic_height_in_map_units_minus1 = parseUE(br);
    unsigned frame_mbs_only_flag = br.getBits(1);

    *width = pic_width_in_mbs_minus1 * 16 + 16;
    *height = (2 - frame_mbs_only_flag) * (pic_height_in_map_units_minus1 * 16 + 16);

    if (!frame_mbs_only_flag) {
        br.getBits(1); // mb_adaptive_frame_field_flag
    }

    br.getBits(1); // direct_8x8_inference_flag

    if (br.getBits(1)) { // frame_cropping_flag
        unsigned frame_crop_left_offset = parseUE(br);
        unsigned frame_crop_right_offset = parseUE(br);
        unsigned frame_crop_top_offset = parseUE(br);
        unsigned frame_crop_bottom_offset = parseUE(br);

        unsigned cropUnitX, cropUnitY;
        if (chroma_format_idc == 0 /* monochrome */) {
            cropUnitX = 1;
            cropUnitY = 2 - frame_mbs_only_flag;
        } else {
            unsigned subWidthC = (chroma_format_idc == 3) ? 1 : 2;
            unsigned subHeightC = (chroma_format_idc == 1) ? 2 : 1;

            cropUnitX = subWidthC;
            cropUnitY = subHeightC * (2 - frame_mbs_only_flag);
        }

        *width -= (frame_crop_left_offset + frame_crop_right_offset) * cropUnitX;
        *height -= (frame_crop_top_offset + frame_crop_bottom_offset) * cropUnitY;
    }
}

unsigned CsdUtils::parseUE(BitReader& br) {
    unsigned numZeroes = 0;
    while (br.getBits(1) == 0) {
        ++numZeroes;
    }
    unsigned x = br.getBits(numZeroes);
    return x + (1u << numZeroes) - 1;
}
