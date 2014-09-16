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
//#define LOG_NDEBUG 0
#define LOG_TAG "DSPG_JpegEncoder"
#include <utils/Log.h>
#include <utils/Errors.h>

#include "JpegEncoder.h"
#include "EncJpegInstance.h"
//hardware jpeg encoder
#include "EncJpeg.h"

static const char ExifAsciiPrefix[] = { 0x41, 0x53, 0x43, 0x49, 0x49, 0x0, 0x0, 0x0 };

namespace android {

JpegEncoder::JpegEncoder()
{
	LOGV("%s:%d",__func__,__LINE__);
	mEncoder = NULL;
	mOutBuf = NULL;
	mThumbSize = 0;
	mJpegSize = 0;

	clear();
}

JpegEncoder::~JpegEncoder()
{
	LOGV("%s:%d",__func__,__LINE__);
	if (mOutBuf != NULL && mEncoder != NULL)
	{
		EWLFreeLinear(((jpegInstance_s *)mEncoder)->asic.ewl, mOutBuf);
		delete(mOutBuf);
		mOutBuf = NULL;
	}
	
	if (mEncoder != NULL)
	{
		int32_t ret = JpegEncRelease(mEncoder);
		if (ret != JPEGENC_OK)
		{
			/*
			There is not much to do for an error here,
			just a notification message. There has to be a fault
			somewhere else that caused a failure of the
			encoder release
			*/
			LOGE("Failed to release encoder: errno=%d",ret);
		}
		mEncoder = NULL;
	}
}	

void JpegEncoder::clear()
{
	memset(&mArgs,0,sizeof(jpg_args));
}

uint32_t JpegEncoder::setConfig(jpeg_conf type, int32_t value)
{

    uint32_t ret = NO_ERROR;

    switch (type) {
    case JPEG_SET_ENCODE_WIDTH:
        mArgs.width = value;
        break;

    case JPEG_SET_ENCODE_HEIGHT:
        mArgs.height = value;
        break;

    case JPEG_SET_ENCODE_QUALITY:
		if(value >= 100)
			mArgs.quality = 9;
		else if (value < 0)
			ret = UNKNOWN_ERROR;
		else
	        mArgs.quality = value / 10;
        break;

    case JPEG_SET_ENCODE_IN_FORMAT:
        break;

	case JPEG_SET_THUMBNAIL_WIDTH:
		mArgs.thumb_width = value;
		break;

	case JPEG_SET_THUMBNAIL_HEIGHT:
		mArgs.thumb_height = value;
		break;

	case JPEG_SET_THUMBNAIL_SIZE:
		mArgs.thumb_size = value;
		break;

	case JPEG_SET_OUTPUT_BUF_SIZE:
		mArgs.data_size = value;
		break;

    default:
        LOGE("Invalid Config type");
        ret = UNKNOWN_ERROR;
    }

    return ret;
}

uint32_t JpegEncoder::CheckDataIntegrity()
{
	LOGV("%s:%d",__func__,__LINE__);

	if (mArgs.width <= 0)
	{
		LOGE("Invalid source width: %d",mArgs.width);
		return UNKNOWN_ERROR;
	}

	if (mArgs.height <= 0)
	{
		LOGE("Invalid source height: %d",mArgs.height);
		return UNKNOWN_ERROR;
	}

	if (mArgs.quality >= 10)
	{
		LOGE("Invalid jpeg quality height: %d",mArgs.quality);
		return UNKNOWN_ERROR;
	}

	if (mArgs.data_size ==  0)
	{
		LOGE("Invalid jpeg size: %d",mArgs.data_size);
		return UNKNOWN_ERROR;
	}

	return NO_ERROR;
}


/*------------------------------------------------------------------------------

    AllocRes

    Allocation of the physical memories used by both SW and HW: 
    the input picture and the output stream buffer.

    NOTE! The implementation uses the EWL instance from the encoder
          for OS independence. This is not recommended in final environment 
          because the encoder will release the EWL instance in case of error.
          Instead, the memories should be allocated from the OS the same way
          as inside EWLMallocLinear().

------------------------------------------------------------------------------*/
uint32_t JpegEncoder::AllocImgMem(JpegEncCfg *cfg)
{
    uint32_t outbufSize;
    int32_t ret;

	LOGV("%s:%d",__func__,__LINE__);
    outbufSize = mArgs.width * mArgs.height * 2 + 8192;

    if(cfg->thumbnail == 1)
        outbufSize += mArgs.thumb_width * mArgs.thumb_height;

	if (mOutBuf != NULL)
	{
		EWLFreeLinear(((jpegInstance_s *)mEncoder)->asic.ewl, mOutBuf);
		delete mOutBuf;
	}
	 
	mOutBuf = new EWLLinearMem_t;

    ret = EWLMallocLinear(((jpegInstance_s *)mEncoder)->asic.ewl, outbufSize, 
                mOutBuf);
	
    if (ret != EWL_OK){
		delete mOutBuf;
		mOutBuf = NULL;
		LOGE("EWLMallocLinear failed: errno=%d",ret);
        return ret;
	}

    return NO_ERROR;
}


uint8_t *JpegEncoder::getOutputVirtAddr()
{
	if (mOutBuf == NULL)
		return NULL;

	return (uint8_t *)mOutBuf->virtualAddress;
}

uint32_t JpegEncoder::encode(uint32_t srcPhysAddr,uint32_t thumbPhysAddr)
{
	JpegEncRet ret;
	JpegEncCfg cfg;
	JpegEncIn encIn;
	JpegEncOut encOut;

	LOGV("%s:%d",__func__,__LINE__);
	if( CheckDataIntegrity() != NO_ERROR )
	{
		LOGE("Encoder is not configured");
		return UNKNOWN_ERROR;
	}
	
	memset(&cfg,0,sizeof(JpegEncCfg));

	/* Step 1: Initialize an encoder instance */
	cfg.qLevel = mArgs.quality;
	cfg.frameType = JPEGENC_YUV420_SEMIPLANAR;
	cfg.rotation = JPEGENC_ROTATE_0;
	cfg.markerType = JPEGENC_SINGLE_MARKER;
	cfg.unitsType = JPEGENC_NO_UNITS;
	cfg.codingType = JPEGENC_WHOLE_FRAME;

	encIn.frameHeader = 1;

	if( mArgs.thumb_width > 0 && mArgs.thumb_width > 0)
		cfg.thumbnail = 1;

	ret = JpegEncInit(&cfg, &mEncoder);
	if( ret != JPEGENC_OK )
	{
		LOGE("Failed to initialize jpeg encoder, errno: %d",ret);
		/* Handle here the error situation */
        return UNKNOWN_ERROR;
	}

	if( AllocImgMem(&cfg) != NO_ERROR )
	{
		LOGE("Failed to allocate phys memory");
		/* Handle here the error situation */
        return UNKNOWN_ERROR;
	}

	if (cfg.thumbnail == 1)
	{
		/*THUMB IMAGE*/
		cfg.cfgThumb.inputWidth = mArgs.thumb_width;
		cfg.cfgThumb.codingWidth = mArgs.thumb_width;
		cfg.cfgThumb.inputHeight = mArgs.thumb_height;
		cfg.cfgThumb.codingHeight = mArgs.thumb_height;
		cfg.xOffset = 0;
		cfg.yOffset = 0;

		ret = JpegEncSetThumbnailMode(mEncoder,&cfg);

		if (ret != JPEGENC_OK)
		{
			LOGE("Failed to initialize thumbnail mode, errno: %d",ret);
			/* Handle here the error situation */
		    return UNKNOWN_ERROR;
		}

		encIn.busLum = thumbPhysAddr;
		encIn.busCb = encIn.busLum + cfg.cfgThumb.inputWidth * cfg.cfgThumb.inputHeight;
	//	encIn.busCr; no required, we are using 4:2:0

		/* Setup encoder input */
		encIn.pOutBuf = (u8 *)mOutBuf->virtualAddress;

		encIn.busOutBuf = mOutBuf->busAddress;
		encIn.outBufSize = mOutBuf->size;

		ret = JpegEncEncode(mEncoder,&encIn,&encOut);

		if (ret != JPEGENC_FRAME_READY)
		{
			LOGE("JpegEncEncode failed on thumnail encode: %d",ret);
			/* Handle here the error situation */
		    return UNKNOWN_ERROR;
		}
		mThumbSize = encOut.jfifSize;
		
		LOGV("Thumbnail jpeg size=%d",mThumbSize);
	}

	/*MAIN IMAGE*/
	cfg.inputWidth = mArgs.width;
	cfg.codingWidth = mArgs.width;
	cfg.inputHeight = mArgs.height;
	cfg.codingHeight = mArgs.height;
	cfg.xOffset = 0;
	cfg.yOffset = 0;

	ret = JpegEncSetFullResolutionMode(mEncoder, &cfg);

	if( ret != JPEGENC_OK )
	{
		LOGE("JpegEncSetFullResolutionMode failed, errno: %d",ret);
		/* Handle here the error situation */
        return UNKNOWN_ERROR;
	}

	encIn.busLum = srcPhysAddr;
	encIn.busCb = encIn.busLum + cfg.inputWidth * cfg.inputHeight;
//	encIn.busCr; no required, we are using 4:2:0

    /* Setup encoder input */
    encIn.pOutBuf = (u8 *)mOutBuf->virtualAddress;
    encIn.busOutBuf = mOutBuf->busAddress;
    encIn.outBufSize = mOutBuf->size;


	ret = JpegEncEncode(mEncoder,&encIn,&encOut);

	if (ret != JPEGENC_FRAME_READY)
	{
		LOGE("JpegEncEncode failed on thumnail encode: %d",ret);
		/* Handle here the error situation */
	    return UNKNOWN_ERROR;
	}

	mJpegSize = encOut.jfifSize;
	LOGV("%s:%d",__func__,__LINE__);
	LOGV("Image jpeg size=%d",mJpegSize);
	
	return NO_ERROR;
}

void JpegEncoder::getJpegSizes(int32_t *pThumbJpegSize,int32_t *pImageJpegSize)
{
	*pThumbJpegSize = mThumbSize;
	*pImageJpegSize = mJpegSize;
}


#if 0
//BS::  SOFTWARE ENCODER
int32_t JpegEncoder::encode(unsigned char *src)
{
	int width = mArgs.width;
	int height = mArgs.height;
	int fileSize;

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    FILE *outfile = NULL;
    int ret = NO_ERROR;
    if ((outfile = fopen(TEMP_JPEG_FILENAME, "wb")) == NULL) 
    {  
        return UNKNOWN_ERROR;
    }    
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, outfile);

	//setup compression defaults
    cinfo.image_width = width; 
    cinfo.image_height = height;
    cinfo.input_components = 3;        
    cinfo.in_color_space = JCS_YCbCr;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, mArgs.quality, TRUE);
    cinfo.raw_data_in = TRUE;  
    
    {
        JSAMPARRAY pp[3];
        JSAMPROW *rpY = (JSAMPROW*)malloc(sizeof(JSAMPROW) * height);
        JSAMPROW *rpU = (JSAMPROW*)malloc(sizeof(JSAMPROW) * height);
        JSAMPROW *rpV = (JSAMPROW*)malloc(sizeof(JSAMPROW) * height);
        int k;
        cinfo.comp_info[0].h_samp_factor =
        cinfo.comp_info[0].v_samp_factor = 2;
        cinfo.comp_info[1].h_samp_factor =
        cinfo.comp_info[1].v_samp_factor =
        cinfo.comp_info[2].h_samp_factor =
        cinfo.comp_info[2].v_samp_factor = 1;
        jpeg_start_compress(&cinfo, TRUE);

        pp[0] = rpY;
        pp[1] = rpU;
        pp[2] = rpV;

		uint8_t* uRows = new uint8_t [8 * (width >> 1)];
		uint8_t* vRows = new uint8_t [8 * (width >> 1)];


		unsigned char *pUV = src + width*height;

		for (int line = 0; line < height; line += 2*DCTSIZE) {

			deinterleave(pUV, uRows, vRows, line, width);

	        // setup pointers in the JSAMPIMAGE array
            for (int i = 0; i < 2*DCTSIZE; i++) {
	            rpY[i] = src + width*(i + line);
		        // construct u row and v row
		        if ((i & 1) == 0) {
		            // height and width are both halved because of downsampling
		            int offset = (i >> 1) * (width >> 1);
		            rpU[i/2] = uRows + offset;
		            rpV[i/2] = vRows + offset;
		        }
            }
            jpeg_write_raw_data(&cinfo, pp,2*DCTSIZE);
        }
        jpeg_finish_compress(&cinfo);
        free(rpY);
        free(rpU);
        free(rpV);

		delete [] uRows;
		delete [] vRows;

    }
	fileSize = ftell(outfile);
    fclose(outfile);
    jpeg_destroy_compress(&cinfo);
    return fileSize;
}

//BS:: For Software encoder ONLY, TBR!!!
//Deinterleive UV samples to planar mode
void JpegEncoder::deinterleave(uint8_t* vuPlanar, uint8_t* uRows,
        uint8_t* vRows, int rowIndex, int width) 
{
    for (int row = 0; row < 8; ++row) {
        int offset = ((rowIndex >> 1) + row) * width;
        uint8_t* uv = vuPlanar + offset;
        for (int i = 0; i < (width >> 1); ++i) {
            int index = row * (width >> 1) + i;
            uRows[index] = *uv++;
            vRows[index] = *uv++;
        }
    }
}
#endif

int JpegEncoder::makeExif (unsigned char *exifOut,
                    exif_attribute_t *exifInfo,
                    unsigned int *size,
                    unsigned char *thumbBuf,
					int thumbSize)
{
    LOGD("makeExif E");

    unsigned char *pCur, *pApp1Start, *pIfdStart, *pGpsIfdPtr, *pNextIfdOffset;
    unsigned int tmp, LongerTagOffest = 0;
    pApp1Start = pCur = exifOut;

    //2 Exif Identifier Code & TIFF Header
    pCur += 4;  // Skip 4 Byte for APP1 marker and length
    unsigned char ExifIdentifierCode[6] = { 0x45, 0x78, 0x69, 0x66, 0x00, 0x00 };
    memcpy(pCur, ExifIdentifierCode, 6);
    pCur += 6;

    /* Byte Order - little endian, Offset of IFD - 0x00000008.H */
    unsigned char TiffHeader[8] = { 0x49, 0x49, 0x2A, 0x00, 0x08, 0x00, 0x00, 0x00 };
    memcpy(pCur, TiffHeader, 8);
    pIfdStart = pCur;
    pCur += 8;

    //2 0th IFD TIFF Tags
    if (exifInfo->enableGps)
        tmp = NUM_0TH_IFD_TIFF;
    else
        tmp = NUM_0TH_IFD_TIFF - 1;

    memcpy(pCur, &tmp, NUM_SIZE);
    pCur += NUM_SIZE;

    LongerTagOffest += 8 + NUM_SIZE + tmp*IFD_SIZE + OFFSET_SIZE;

    writeExifIfd(&pCur, EXIF_TAG_IMAGE_WIDTH, EXIF_TYPE_LONG,
                 1, exifInfo->width);
    writeExifIfd(&pCur, EXIF_TAG_IMAGE_HEIGHT, EXIF_TYPE_LONG,
                 1, exifInfo->height);
    writeExifIfd(&pCur, EXIF_TAG_MAKE, EXIF_TYPE_ASCII,
                 strlen((char *)exifInfo->maker) + 1, exifInfo->maker, &LongerTagOffest, pIfdStart);
    writeExifIfd(&pCur, EXIF_TAG_MODEL, EXIF_TYPE_ASCII,
                 strlen((char *)exifInfo->model) + 1, exifInfo->model, &LongerTagOffest, pIfdStart);
    writeExifIfd(&pCur, EXIF_TAG_ORIENTATION, EXIF_TYPE_SHORT,
                 1, exifInfo->orientation);
    writeExifIfd(&pCur, EXIF_TAG_SOFTWARE, EXIF_TYPE_ASCII,
                 strlen((char *)exifInfo->software) + 1, exifInfo->software, &LongerTagOffest, pIfdStart);
    writeExifIfd(&pCur, EXIF_TAG_DATE_TIME, EXIF_TYPE_ASCII,
                 20, exifInfo->date_time, &LongerTagOffest, pIfdStart);
    writeExifIfd(&pCur, EXIF_TAG_YCBCR_POSITIONING, EXIF_TYPE_SHORT,
                 1, exifInfo->ycbcr_positioning);
    writeExifIfd(&pCur, EXIF_TAG_EXIF_IFD_POINTER, EXIF_TYPE_LONG,
                 1, LongerTagOffest);
    if (exifInfo->enableGps) {
        pGpsIfdPtr = pCur;
        pCur += IFD_SIZE;   // Skip a ifd size for gps IFD pointer
    }

    pNextIfdOffset = pCur;  // Skip a offset size for next IFD offset
    pCur += OFFSET_SIZE;

    //2 0th IFD Exif Private Tags
    pCur = pIfdStart + LongerTagOffest;

    tmp = NUM_0TH_IFD_EXIF;
    memcpy(pCur, &tmp , NUM_SIZE);
    pCur += NUM_SIZE;

    LongerTagOffest += NUM_SIZE + NUM_0TH_IFD_EXIF*IFD_SIZE + OFFSET_SIZE;

    writeExifIfd(&pCur, EXIF_TAG_EXPOSURE_TIME, EXIF_TYPE_RATIONAL,
                 1, &exifInfo->exposure_time, &LongerTagOffest, pIfdStart);
    writeExifIfd(&pCur, EXIF_TAG_FNUMBER, EXIF_TYPE_RATIONAL,
                 1, &exifInfo->fnumber, &LongerTagOffest, pIfdStart);
    writeExifIfd(&pCur, EXIF_TAG_EXPOSURE_PROGRAM, EXIF_TYPE_SHORT,
                 1, exifInfo->exposure_program);
    writeExifIfd(&pCur, EXIF_TAG_ISO_SPEED_RATING, EXIF_TYPE_SHORT,
                 1, exifInfo->iso_speed_rating);
    writeExifIfd(&pCur, EXIF_TAG_EXIF_VERSION, EXIF_TYPE_UNDEFINED,
                 4, exifInfo->exif_version);
    writeExifIfd(&pCur, EXIF_TAG_DATE_TIME_ORG, EXIF_TYPE_ASCII,
                 20, exifInfo->date_time, &LongerTagOffest, pIfdStart);
    writeExifIfd(&pCur, EXIF_TAG_DATE_TIME_DIGITIZE, EXIF_TYPE_ASCII,
                 20, exifInfo->date_time, &LongerTagOffest, pIfdStart);
    writeExifIfd(&pCur, EXIF_TAG_SHUTTER_SPEED, EXIF_TYPE_SRATIONAL,
                 1, (rational_t *)&exifInfo->shutter_speed, &LongerTagOffest, pIfdStart);
    writeExifIfd(&pCur, EXIF_TAG_APERTURE, EXIF_TYPE_RATIONAL,
                 1, &exifInfo->aperture, &LongerTagOffest, pIfdStart);
    writeExifIfd(&pCur, EXIF_TAG_BRIGHTNESS, EXIF_TYPE_SRATIONAL,
                 1, (rational_t *)&exifInfo->brightness, &LongerTagOffest, pIfdStart);
    writeExifIfd(&pCur, EXIF_TAG_EXPOSURE_BIAS, EXIF_TYPE_SRATIONAL,
                 1, (rational_t *)&exifInfo->exposure_bias, &LongerTagOffest, pIfdStart);
    writeExifIfd(&pCur, EXIF_TAG_MAX_APERTURE, EXIF_TYPE_RATIONAL,
                 1, &exifInfo->max_aperture, &LongerTagOffest, pIfdStart);
    writeExifIfd(&pCur, EXIF_TAG_METERING_MODE, EXIF_TYPE_SHORT,
                 1, exifInfo->metering_mode);
    writeExifIfd(&pCur, EXIF_TAG_FLASH, EXIF_TYPE_SHORT,
                 1, exifInfo->flash);
    writeExifIfd(&pCur, EXIF_TAG_FOCAL_LENGTH, EXIF_TYPE_RATIONAL,
                 1, &exifInfo->focal_length, &LongerTagOffest, pIfdStart);
    char code[8] = { 0x00, 0x00, 0x00, 0x49, 0x49, 0x43, 0x53, 0x41 };
    int commentsLen = strlen((char *)exifInfo->user_comment) + 1;
    memmove(exifInfo->user_comment + sizeof(code), exifInfo->user_comment, commentsLen);
    memcpy(exifInfo->user_comment, code, sizeof(code));
    writeExifIfd(&pCur, EXIF_TAG_USER_COMMENT, EXIF_TYPE_UNDEFINED,
                 commentsLen + sizeof(code), exifInfo->user_comment, &LongerTagOffest, pIfdStart);
    writeExifIfd(&pCur, EXIF_TAG_COLOR_SPACE, EXIF_TYPE_SHORT,
                 1, exifInfo->color_space);
    writeExifIfd(&pCur, EXIF_TAG_PIXEL_X_DIMENSION, EXIF_TYPE_LONG,
                 1, exifInfo->width);
    writeExifIfd(&pCur, EXIF_TAG_PIXEL_Y_DIMENSION, EXIF_TYPE_LONG,
                 1, exifInfo->height);
    writeExifIfd(&pCur, EXIF_TAG_EXPOSURE_MODE, EXIF_TYPE_LONG,
                 1, exifInfo->exposure_mode);
    writeExifIfd(&pCur, EXIF_TAG_WHITE_BALANCE, EXIF_TYPE_LONG,
                 1, exifInfo->white_balance);
    writeExifIfd(&pCur, EXIF_TAG_SCENCE_CAPTURE_TYPE, EXIF_TYPE_LONG,
                 1, exifInfo->scene_capture_type);
    tmp = 0;
    memcpy(pCur, &tmp, OFFSET_SIZE); // next IFD offset
    pCur += OFFSET_SIZE;

    //2 0th IFD GPS Info Tags
    if (exifInfo->enableGps) {
        writeExifIfd(&pGpsIfdPtr, EXIF_TAG_GPS_IFD_POINTER, EXIF_TYPE_LONG,
                     1, LongerTagOffest); // GPS IFD pointer skipped on 0th IFD

        pCur = pIfdStart + LongerTagOffest;

        if (exifInfo->gps_processing_method[0] == 0) {
            // don't create GPS_PROCESSING_METHOD tag if there isn't any
            tmp = NUM_0TH_IFD_GPS - 1;
        } else {
            tmp = NUM_0TH_IFD_GPS;
        }
        memcpy(pCur, &tmp, NUM_SIZE);
        pCur += NUM_SIZE;

        LongerTagOffest += NUM_SIZE + tmp*IFD_SIZE + OFFSET_SIZE;

        writeExifIfd(&pCur, EXIF_TAG_GPS_VERSION_ID, EXIF_TYPE_BYTE,
                     4, exifInfo->gps_version_id);
        writeExifIfd(&pCur, EXIF_TAG_GPS_LATITUDE_REF, EXIF_TYPE_ASCII,
                     2, exifInfo->gps_latitude_ref);
        writeExifIfd(&pCur, EXIF_TAG_GPS_LATITUDE, EXIF_TYPE_RATIONAL,
                     3, exifInfo->gps_latitude, &LongerTagOffest, pIfdStart);
        writeExifIfd(&pCur, EXIF_TAG_GPS_LONGITUDE_REF, EXIF_TYPE_ASCII,
                     2, exifInfo->gps_longitude_ref);
        writeExifIfd(&pCur, EXIF_TAG_GPS_LONGITUDE, EXIF_TYPE_RATIONAL,
                     3, exifInfo->gps_longitude, &LongerTagOffest, pIfdStart);
        writeExifIfd(&pCur, EXIF_TAG_GPS_ALTITUDE_REF, EXIF_TYPE_BYTE,
                     1, exifInfo->gps_altitude_ref);
        writeExifIfd(&pCur, EXIF_TAG_GPS_ALTITUDE, EXIF_TYPE_RATIONAL,
                     1, &exifInfo->gps_altitude, &LongerTagOffest, pIfdStart);
        writeExifIfd(&pCur, EXIF_TAG_GPS_TIMESTAMP, EXIF_TYPE_RATIONAL,
                     3, exifInfo->gps_timestamp, &LongerTagOffest, pIfdStart);
        tmp = strlen((char*)exifInfo->gps_processing_method);
	if (tmp > 0) {
            if (tmp > 100) {
                tmp = 100;
            }
            unsigned char tmp_buf[100+sizeof(ExifAsciiPrefix)];
            memcpy(tmp_buf, ExifAsciiPrefix, sizeof(ExifAsciiPrefix));
            memcpy(&tmp_buf[sizeof(ExifAsciiPrefix)], exifInfo->gps_processing_method, tmp);
            writeExifIfd(&pCur, EXIF_TAG_GPS_PROCESSING_METHOD, EXIF_TYPE_UNDEFINED,
                         tmp+sizeof(ExifAsciiPrefix), tmp_buf, &LongerTagOffest, pIfdStart);
        }
        writeExifIfd(&pCur, EXIF_TAG_GPS_DATESTAMP, EXIF_TYPE_ASCII,
                     11, exifInfo->gps_datestamp, &LongerTagOffest, pIfdStart);
        tmp = 0;
        memcpy(pCur, &tmp, OFFSET_SIZE); // next IFD offset
        pCur += OFFSET_SIZE;
    }

    //2 1th IFD TIFF Tags
    if (exifInfo->enableThumb && (thumbBuf != NULL) && (thumbSize > 0)) {
        tmp = LongerTagOffest;
        memcpy(pNextIfdOffset, &tmp, OFFSET_SIZE);  // NEXT IFD offset skipped on 0th IFD

        pCur = pIfdStart + LongerTagOffest;

        tmp = NUM_1TH_IFD_TIFF;
        memcpy(pCur, &tmp, NUM_SIZE);
        pCur += NUM_SIZE;

        LongerTagOffest += NUM_SIZE + NUM_1TH_IFD_TIFF*IFD_SIZE + OFFSET_SIZE;

        writeExifIfd(&pCur, EXIF_TAG_IMAGE_WIDTH, EXIF_TYPE_LONG,
                     1, exifInfo->widthThumb);
        writeExifIfd(&pCur, EXIF_TAG_IMAGE_HEIGHT, EXIF_TYPE_LONG,
                     1, exifInfo->heightThumb);
        writeExifIfd(&pCur, EXIF_TAG_COMPRESSION_SCHEME, EXIF_TYPE_SHORT,
                     1, exifInfo->compression_scheme);
        writeExifIfd(&pCur, EXIF_TAG_ORIENTATION, EXIF_TYPE_SHORT,
                     1, exifInfo->orientation);
        writeExifIfd(&pCur, EXIF_TAG_X_RESOLUTION, EXIF_TYPE_RATIONAL,
                     1, &exifInfo->x_resolution, &LongerTagOffest, pIfdStart);
        writeExifIfd(&pCur, EXIF_TAG_Y_RESOLUTION, EXIF_TYPE_RATIONAL,
                     1, &exifInfo->y_resolution, &LongerTagOffest, pIfdStart);
        writeExifIfd(&pCur, EXIF_TAG_RESOLUTION_UNIT, EXIF_TYPE_SHORT,
                     1, exifInfo->resolution_unit);
        writeExifIfd(&pCur, EXIF_TAG_JPEG_INTERCHANGE_FORMAT, EXIF_TYPE_LONG,
                     1, LongerTagOffest);
        writeExifIfd(&pCur, EXIF_TAG_JPEG_INTERCHANGE_FORMAT_LEN, EXIF_TYPE_LONG,
                     1, thumbSize);

        tmp = 0;
        memcpy(pCur, &tmp, OFFSET_SIZE); // next IFD offset
        pCur += OFFSET_SIZE;

        memcpy(pIfdStart + LongerTagOffest,
               thumbBuf, thumbSize);
        LongerTagOffest += thumbSize;
    } else {
        tmp = 0;
        memcpy(pNextIfdOffset, &tmp, OFFSET_SIZE);  // NEXT IFD offset skipped on 0th IFD
    }

    unsigned char App1Marker[2] = { 0xff, 0xe1 };
    memcpy(pApp1Start, App1Marker, 2);
    pApp1Start += 2;

    *size = 10 + LongerTagOffest;
    tmp = *size - 2;    // APP1 Maker isn't counted
    unsigned char size_mm[2] = {(tmp >> 8) & 0xFF, tmp & 0xFF};
    memcpy(pApp1Start, size_mm, 2);

    LOGD("makeExif X");

    return NO_ERROR;
}

inline void JpegEncoder::writeExifIfd(unsigned char **pCur,
                                         unsigned short tag,
                                         unsigned short type,
                                         unsigned int count,
                                         uint32_t value)
{
    memcpy(*pCur, &tag, 2);
    *pCur += 2;
    memcpy(*pCur, &type, 2);
    *pCur += 2;
    memcpy(*pCur, &count, 4);
    *pCur += 4;
    memcpy(*pCur, &value, 4);
    *pCur += 4;
}

inline void JpegEncoder::writeExifIfd(unsigned char **pCur,
                                         unsigned short tag,
                                         unsigned short type,
                                         unsigned int count,
                                         unsigned char *pValue)
{
    char buf[4] = { 0,};

    memcpy(buf, pValue, count);
    memcpy(*pCur, &tag, 2);
    *pCur += 2;
    memcpy(*pCur, &type, 2);
    *pCur += 2;
    memcpy(*pCur, &count, 4);
    *pCur += 4;
    memcpy(*pCur, buf, 4);
    *pCur += 4;
}


inline void JpegEncoder::writeExifIfd(unsigned char **pCur,
                                         unsigned short tag,
                                         unsigned short type,
                                         unsigned int count,
                                         unsigned char *pValue,
                                         unsigned int *offset,
                                         unsigned char *start)
{
    memcpy(*pCur, &tag, 2);
    *pCur += 2;
    memcpy(*pCur, &type, 2);
    *pCur += 2;
    memcpy(*pCur, &count, 4);
    *pCur += 4;
    memcpy(*pCur, offset, 4);
    *pCur += 4;
    memcpy(start + *offset, pValue, count);
    *offset += count;
}

inline void JpegEncoder::writeExifIfd(unsigned char **pCur,
                                         unsigned short tag,
                                         unsigned short type,
                                         unsigned int count,
                                         rational_t *pValue,
                                         unsigned int *offset,
                                         unsigned char *start)
{
    memcpy(*pCur, &tag, 2);
    *pCur += 2;
    memcpy(*pCur, &type, 2);
    *pCur += 2;
    memcpy(*pCur, &count, 4);
    *pCur += 4;
    memcpy(*pCur, offset, 4);
    *pCur += 4;
    memcpy(start + *offset, pValue, 8 * count);
    *offset += 8 * count;
}


};

