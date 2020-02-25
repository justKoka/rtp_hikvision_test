#pragma once
extern "C"
{
#include "libavformat/avformat.h"
}
#include <iostream>
#include "hls-fmp4.h"
#include "hls-m3u.h"
#include "hls-param.h"
#include "../../libmov/include/mov-format.h"
#include "../../libmov/include/mpeg-ps.h"
#include <assert.h>
#include "../../librtp/include/rtp-payload-internal.h"
#include "../include/fdkaac_enc.h"

class hls_segmenter;

struct rtp_payload_test_t
{
	int payload;
	const char* encoding;
	int track; // number of track in fmp4
	void* encoder;
	void* decoder;
	size_t size;
	uint8_t *packet;
	hls_segmenter *hsegmenter; //parent hls_segmenter
	/*uint8_t packet[64 * 1024];*/
};

class hls_segmenter {
private:
	hls_fmp4_t* hls;
	hls_m3u_t m3u8;
	int64_t first_timestamp;
	//mp4 tracks
	int track_aac = -1;
	int track_264 = -1;
	int track_265 = -1;
	//video params
	int height;
	int width;
	uint8_t* extra_data;
	int extra_data_size;
	//audio params
	int channels;
	int bits_per_coded_sample;
	int sample_rate;
	//video/audio rtp encoder context
	rtp_payload_test_t *ctxVideo = NULL;
	rtp_payload_test_t *ctxAudio = NULL;
public:

	AacEncoder fdkaac_enc; // for internal usage
	int pack(int64_t timestamp, const void *packet, int size, const char *encoding, int keyframe); // for internal usage
	hls_segmenter();
	hls_segmenter(int duration, int playlist_capacity, int remaining);
	~hls_segmenter();
	int send_rtp(const void *packet, int size);
	int init(int duration, int playlist_capacity, int remaining);
	void init_video_stream(const char* encoding, uint8_t* sps, int sps_size);
	void init_audio_stream(int payload, const char* encoding, int channels, int bits_per_coded_sample, int sample_rate);
	std::string playlist();
	int destroy();
};


