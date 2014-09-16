#!/bin/bash

echo "Testing H.264 encoding"
mount_point=./testdata_encoder ./test_h264.sh all &> h264_encoder.log
echo "H.264 Testing Done!"

echo "Testing MPEG4 and H263 encoding"
mount_point=./testdata_encoder ./test_mpeg4.sh all &> mpeg4_encoder.log
echo "MPEG-4/H.263 Testing Done!"

echo "Testing JPEG encoding"
mount_point=./testdata_encoder ./test_jpeg.sh all &> jpeg_encoder.log
echo "JPEG Testing Done!"

