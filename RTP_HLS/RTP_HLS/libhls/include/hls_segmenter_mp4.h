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
	void* encoder;
	void* decoder;
	size_t size;
	uint8_t *packet;
	hls_segmenter *hsegmenter; //parent hls_segmenter
	uint32_t first_timestamp = 0;
};

class hls_segmenter {
private:
	hls_fmp4_t* hls;
	hls_m3u_t m3u8;
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
	rtp_payload_test_t ctxVideo;
	rtp_payload_test_t ctxAudio;
	//video/audio stream init flag
	bool audioStream = false;
	bool videoStream = false;
public:
	AacEncoder fdkaac_enc; // for internal usage
	int pack(int64_t timestamp, const void *packet, int size, const char *encoding, int keyframe); // for internal usage
	hls_segmenter();
	hls_segmenter(int duration, int playlist_capasity, int playlist_remaining, std::string url);
	~hls_segmenter();
	int send_rtp(uint8_t *packet, int size);
	int write_init_segment(std::string filename);
	void init_video_stream(const char* encoding, int payload, uint8_t* sps, int sps_size);
	void init_audio_stream(const char* encoding, int payload, int channels, int bits_per_coded_sample, int sample_rate);
	std::vector<uint8_t> get_segment(std::string name);
	std::vector<uint8_t> get_init_file();
	std::string get_playlist_data();
	int destroy();
};


