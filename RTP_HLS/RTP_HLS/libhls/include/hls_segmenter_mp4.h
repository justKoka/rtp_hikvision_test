#pragma once
extern "C"
{
#include "libavformat/avformat.h"
}
#include <iostream>
#include "hls-fmp4.h"
#include "hls-m3u8.h"
#include "hls-m3u.h"
#include "hls-param.h"
#include "../../libmov/include/mov-format.h"
#include "../../libmov/include/mpeg-ps.h"
#include <assert.h>

enum codec {
	h264,
	h265,
	aac
};

struct hls_segmenter {
	hls_fmp4_t* hls;
	hls_m3u_t* m3u8;
	int64_t first_timestamp;
	//mp4 tracks
	int track_aac = -1;
	int track_264 = -1;
	int track_265 = -1;
	//video params
	int height;
	int width;
	//audio params
	int channels;
	int bits_per_coded_sample;
	int sample_rate;
	//video/audio params (sps's)
	uint8_t* extra_data;
	int extra_data_size;
};

int hls_segmenter_mp4_send_packet(hls_segmenter* hls_segmenter, int64_t timestamp, int track, const void *packet, int size, const char *encoding, int keyframe, int nalu_type);
int hls_segmenter_mp4_init(hls_segmenter* hls_segmenter, uint8_t* sps, int sps_size, int duration /* one segment duration in seconds */, int nb_streams, int codec[]);
int hls_segmenter_mp4_destroy();