#pragma once
#include "../../libhls/include/hls_segmenter_mp4.h"

unsigned int ReadBit();
unsigned int ReadBits(int n);
unsigned int ReadExponentialGolombCode();
unsigned int ReadSE();
void Parse(hls_segmenter *hls, const unsigned char * pStart, unsigned short nLen);
/* Note that Parse(const unsigned char * pStart, unsigned short nLen) takes a h264 SPS nalu with an offset of 5 
( remove the '0 0 0 1' prefix and the nal_unit_type byte ) */