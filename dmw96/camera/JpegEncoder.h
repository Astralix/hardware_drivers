/*
 * Copyright Samsung Electronics Co.,LTD.
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef ANDROID_JPEG_ENCODER_DSPG_H
#define ANDROID_JPEG_ENCODER_DSPG_H

#include "Exif.h"

#include "jpegencapi.h"
/* For SW/HW shared memory allocation */
#include "ewl.h"

#define TEMP_JPEG_FILENAME	"/tmp/tmp.jpg"
#define JPEG_MARKER_SIZE			2
#define JPEG_APP0_SIZE	30

namespace android {

typedef struct {
    char    *in_buf;
    char    *out_buf;
    uint32_t 			quality;
    uint32_t            width;
    uint32_t            height;
    uint32_t            data_size;
    uint32_t            file_size;
    uint32_t            set_framebuf;
    uint32_t            thumb_width;
    uint32_t            thumb_height;
    uint32_t            thumb_size;

} jpg_args;

typedef enum {
    JPEG_SET_ENCODE_WIDTH,
    JPEG_SET_ENCODE_HEIGHT,
    JPEG_SET_ENCODE_QUALITY,
    JPEG_SET_ENCODE_IN_FORMAT,
    JPEG_SET_SAMPING_MODE,
    JPEG_SET_THUMBNAIL_WIDTH,
    JPEG_SET_THUMBNAIL_HEIGHT,
    JPEG_SET_THUMBNAIL_SIZE,
    JPEG_SET_OUTPUT_BUF_SIZE
} jpeg_conf;


class JpegEncoder {
public:
    JpegEncoder();
    virtual ~JpegEncoder();

	uint32_t encode(uint32_t srcPhysAddr,uint32_t thumbPhysAddr);
	//int32_t encode(unsigned char *pSrcBuf); 

    uint32_t setConfig(jpeg_conf type, int32_t value);

    void clear();

	int makeExif (unsigned char *exifOut,
		                exif_attribute_t *exifInfo,
		                uint32_t *size,
		                uint8_t *thumbBuf,
						int32_t thumbSize);

	void getJpegSizes(int32_t *pThumbJpegSize,int32_t *pImageJpegSize);

	uint8_t *getOutputVirtAddr();

private:

	uint32_t AllocImgMem(JpegEncCfg *cfg);

	uint32_t CheckDataIntegrity();

	jpg_args mArgs;
	/* for software encoder
	void deinterleave(uint8_t* vuPlanar, uint8_t* uRows,
        uint8_t* vRows, int rowIndex, int width);*/

    inline void writeExifIfd(unsigned char **pCur,
                                 unsigned short tag,
                                 unsigned short type,
                                 unsigned int count,
                                 uint32_t value);
    inline void writeExifIfd(unsigned char **pCur,
                                 unsigned short tag,
                                 unsigned short type,
                                 unsigned int count,
                                 unsigned char *pValue);
    inline void writeExifIfd(unsigned char **pCur,
                                 unsigned short tag,
                                 unsigned short type,
                                 unsigned int count,
                                 rational_t *pValue,
                                 unsigned int *offset,
                                 unsigned char *start);
    inline void writeExifIfd(unsigned char **pCur,
                                 unsigned short tag,
                                 unsigned short type,
                                 unsigned int count,
                                 unsigned char *pValue,
                                 unsigned int *offset,
                                 unsigned char *start);


	int32_t mThumbSize;
	int32_t mJpegSize;
	EWLLinearMem_t *mOutBuf;

	JpegEncInst mEncoder;
};

};
#endif /* ANDROID_HARDWARE_EXIF_H */
