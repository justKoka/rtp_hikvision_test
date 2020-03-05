#include "../include/hls_segmenter_mp4.h"
#include "../../librtp/include/sps_parse.h"
#include "../include/ualaw_to_linear.h"

static char s_packet[2 * 1024 * 1024];

static void* rtp_alloc(void* /*param*/, int bytes)
{
}

static void rtp_free(void* /*param*/, void * /*packet*/)
{
}

static void rtp_decode_packet(void* param, const void *packet, int bytes, uint32_t timestamp, int flags) {
	rtp_payload_test_t* ctx = (struct rtp_payload_test_t*)param;

	uint8_t buffer[1024 * 1024];
	if (flags & 0x01) {
		printf("rtp packet lost with timestamp %u\n", timestamp);
		return;
	}
	size_t size = 0;
	if (ctx->payload < 96) {
		static char pcm[16 * 1024];
		if (ctx->payload == 0) // pcmu
			MuLaw_Sequence_Decode((int8_t*)packet, bytes, (int16_t*)pcm);
		else if (ctx->payload == 8) //pcma
			ALaw_Sequence_Decode((int8_t*)packet, bytes, (int16_t*)pcm);
		int nbAac = ctx->hsegmenter->fdkaac_enc.aacenc_max_output_buffer_size();
		ctx->hsegmenter->fdkaac_enc.aacenc_encode(pcm, bytes * 2, bytes, (char*)buffer, nbAac);
		if (nbAac > 0) {
			size = nbAac;
		}
		else
			return;
	}
	else if (0 == strcasecmp("mpeg4-generic", ctx->encoding))
	{
		int len = bytes + 7;
		uint8_t profile = 2;
		uint8_t sampling_frequency_index = 4;
		uint8_t channel_configuration = 2;
		buffer[0] = 0xFF; /* 12-syncword */
		buffer[1] = 0xF0 /* 12-syncword */ | (0 << 3)/*1-ID*/ | (0x00 << 2) /*2-layer*/ | 0x01 /*1-protection_absent*/;
		buffer[2] = ((profile - 1) << 6) | ((sampling_frequency_index & 0x0F) << 2) | ((channel_configuration >> 2) & 0x01);
		buffer[3] = ((channel_configuration & 0x03) << 6) | ((len >> 11) & 0x03); /*0-original_copy*/ /*0-home*/ /*0-copyright_identification_bit*/ /*0-copyright_identification_start*/
		buffer[4] = (uint8_t)(len >> 3);
		buffer[5] = ((len & 0x07) << 5) | 0x1F;
		buffer[6] = 0xFC | ((len / 1024) & 0x03);
		size = 7;
		memcpy(buffer + size, packet, bytes);
		size += bytes;
	}
	else {
		int keyframe = (flags & 0x02) ? 1 : 0; //check if RTP_I_FRAME flag is set
		memcpy(buffer + size, packet, bytes);
		size += bytes;
	}
	if (ctx->first_timestamp == 0) {
		ctx->first_timestamp = timestamp;
	}
	uint32_t clock = timestamp - ctx->first_timestamp;
	ctx->hsegmenter->pack(clock, buffer, size, ctx->encoding, flags);
}

static int hls_fmp4_segment(void* m3u, const void* data, size_t bytes, int64_t, int64_t dts, int64_t duration) {
	static int i = 0;
	char name[128] = { 0 };
	hls_m3u_t* m3u8 = (hls_m3u_t*)m3u;
	snprintf(name, sizeof(name), "hls_%d.mp4", ++i);
	double extinf = (double)duration / 1000;
	m3u8->add_segment(extinf, name, data, bytes);
	return 0;
}

static int hls_init_segment(hls_fmp4_t* hls, hls_m3u_t* m3u, std::string filename)
{
	int bytes = hls_fmp4_init_segment(hls, s_packet, sizeof(s_packet));
	m3u->set_x_map(filename, s_packet, bytes);
	return bytes;
}

#pragma pack(push, 1)
struct rtpHeaderSecondByte {
	uint8_t PT : 7;
	uint8_t M : 1;
};
#pragma pack(pop)

int hls_segmenter::send_rtp(uint8_t * packet, int size)
{
	rtpHeaderSecondByte *rtpHeader2byte = (rtpHeaderSecondByte *)((uint8_t*)packet + 1);
	if (videoStream) {
		if (rtpHeader2byte->PT == ctxVideo.payload)
		{
			ctxVideo.packet = packet;
			ctxVideo.size = size;
			return rtp_payload_decode_input(ctxVideo.decoder, ctxVideo.packet, ctxVideo.size);
		}
	}
	if (audioStream) {
		if (rtpHeader2byte->PT == ctxAudio.payload)
		{
			ctxAudio.packet = packet;
			ctxAudio.size = size;
			return rtp_payload_decode_input(ctxAudio.decoder, ctxAudio.packet, ctxAudio.size); // return 0 if packet discard, 1 - sucsess
		}
		else
			return -1; // stream with this payload number is uninitialized
	}
	return -2; // no streams initialized
}

int hls_segmenter::pack(int64_t timestamp, const void *packet, int size, const char *encoding, int keyframe = 0) {
		int64_t pts, dts;
		double time_base;
		if (0 == strcasecmp("mpeg4-generic", encoding) || 0 == strcasecmp("pcmu", encoding) || 0 == strcasecmp("pcma", encoding))
		{
			time_base = 1.0 / sample_rate;
			pts = (int64_t)(timestamp * time_base * 1000);
			dts = pts;
			return hls_fmp4_input(hls, track_aac, packet, size, pts, dts, 0);
		}
		else if (0 == strcmp("H264", encoding))
		{
			time_base = 1.0 / 90000;
			pts = (int64_t)(timestamp * time_base * 1000);
			dts = pts;
			return hls_fmp4_input(hls, track_264, packet, size, pts, dts, (keyframe == 1) ? MOV_AV_FLAG_KEYFREAME : 0);
		}
		else if (0 == strcmp("H265", encoding) || 0 == strcmp("HEVC", encoding))
		{
			time_base = 1.0 / 90000;
			pts = (int64_t)(timestamp * time_base * 1000);
			dts = pts;
			return hls_fmp4_input(hls, track_265, packet, size, pts, dts, (keyframe == 1) ? MOV_AV_FLAG_KEYFREAME : 0);
		}
		else
		{
			return -1;
		}
}

hls_segmenter::hls_segmenter()
{
}

hls_segmenter::hls_segmenter(int duration, int playlist_capasity, int playlist_remaining, std::string url)
{
	hls_m3u_t m3u(7, 0, duration, playlist_capasity, playlist_remaining, url);
	this->m3u8 = m3u;
	this->hls = hls_fmp4_create(duration * 1000, hls_fmp4_segment, &m3u8);
}

hls_segmenter::~hls_segmenter()
{
	destroy();
}

/*
duration - duration of one fmp4 segment
playlist_capasity  - max number of segments in playlist
playlist_remaining - number of last segments, remaining after clearing playlist (playlist is cleared when number of segments >= capacity)
*/

int hls_segmenter::write_init_segment(std::string filename) {
	
	return hls_init_segment(hls, &m3u8, filename);
}

void hls_segmenter::init_video_stream(const char* encoding, int payload, uint8_t * sps, int sps_size)
{
	constexpr int NALU_offset = 1;
	std::pair<int, int> width_height_pair = Parse(sps + NALU_offset, sps_size - NALU_offset); //width and height
	width = width_height_pair.first;
	height = width_height_pair.second;
	extra_data = sps + NALU_offset;
	extra_data_size = sps_size - NALU_offset;
	if (0 == strcmp("H264", encoding))
		track_264 = hls_fmp4_add_video(hls, MOV_OBJECT_H264, width, height, extra_data, extra_data_size);
	else if (0 == strcmp("H265", encoding) || 0 == strcmp("HEVC", encoding))
		track_265 = hls_fmp4_add_video(hls, MOV_OBJECT_HEVC, width, height, extra_data, extra_data_size);
	ctxVideo.payload = payload;
	ctxVideo.encoding = encoding;
	rtp_payload_t handler;
	handler.alloc = rtp_alloc;
	handler.free = rtp_free;
	handler.packet = rtp_decode_packet;
	ctxVideo.hsegmenter = this;
	ctxVideo.decoder = rtp_payload_decode_create(ctxVideo.payload, ctxVideo.encoding, &handler, &ctxVideo);
	videoStream = true;
}

void hls_segmenter::init_audio_stream(const char* encoding, int payload, int channels, int bits_per_coded_sample, int sample_rate)
{
	this->channels = channels;
	this->bits_per_coded_sample = bits_per_coded_sample;
	this->sample_rate = sample_rate;
	track_aac = hls_fmp4_add_audio(hls, MOV_OBJECT_AAC, channels, bits_per_coded_sample, sample_rate, NULL, NULL);
	ctxAudio.payload = payload;
	ctxAudio.encoding = encoding;
	rtp_payload_t handler;
	handler.alloc = rtp_alloc;
	handler.free = rtp_free;
	handler.packet = rtp_decode_packet;
	ctxAudio.hsegmenter = this;
	ctxAudio.decoder = rtp_payload_decode_create(ctxAudio.payload, ctxAudio.encoding, &handler, &ctxAudio);
	audioStream = true;
	if (payload < 96)
		fdkaac_enc.aacenc_init(2, channels, sample_rate, bits_per_coded_sample * sample_rate); // 2 - audio object type - aac (lc)
}

std::vector<uint8_t> hls_segmenter::get_segment(std::string name)
{
	return m3u8.get_segment(name);
}

std::vector<uint8_t> hls_segmenter::get_init_file()
{
	return m3u8.get_init_file();
}

std::string hls_segmenter::get_playlist_data()
{
	return m3u8.playlist_data();
}

int hls_segmenter::set_playlist_capacity(int number_of_segments, int remaining)
{
	return m3u8.set_playlist_capacity(number_of_segments, remaining);
}

int hls_segmenter::change_segment_duration(int duration)
{
	hls_m3u_t m3u(7, 0, duration, m3u8.get_capacity(), m3u8.get_remaining(), m3u8.get_url());
	this->m3u8 = m3u;
	this->hls = hls_fmp4_create(duration * 1000, hls_fmp4_segment, &m3u8);
	return 0;
}

int hls_segmenter::destroy() {
	//freeing all resourses
	m3u8.set_x_endlist();
	if (audioStream) {
		if (ctxAudio.payload < 96)
		fdkaac_enc.aacenc_close();
	}
	return 0;
}