#include "../include/hls_segmenter_mp4.h"
#include "../../librtp/include/sps_parse.h"
#include "../include/ualaw_to_linear.h"

static char s_packet[2 * 1024 * 1024];

static void* rtp_alloc(void* /*param*/, int bytes)
{
	//static uint8_t buffer[2 * 1024 * 1024 + 4] = { 0, 0, 0, 1, };
	//assert(bytes <= sizeof(buffer) - 4);
	//return buffer + 4;
}

static void rtp_free(void* /*param*/, void * /*packet*/)
{
}

static void rtp_decode_packet(void* param, const void *packet, int bytes, uint32_t timestamp, int flags) {
	struct rtp_payload_test_t* ctx = (struct rtp_payload_test_t*)param;

	static uint8_t buffer[128 * 1024];
	if (bytes + 4 > sizeof(buffer) || (flags & 0x01)) {
		std::cerr << "something is wrong with decoding rtp packet" << std::endl;
		return;
	}
	size_t size = 0;
	if (ctx->payload < 96) {
		static char pcm[2 * 1024];
		if (ctx->payload == 0) // pcmu
			MuLaw_Sequence_Decode((int8_t*)packet, bytes, (int16_t*)pcm);
		else if (ctx->payload == 8) //pcma
			ALaw_Sequence_Decode((int8_t*)packet, bytes, (int16_t*)pcm);
		int nbAac = 128 * 1024;
		ctx->hsegmenter->fdkaac_enc.aacenc_encode(pcm, bytes, ctx->hsegmenter->fdkaac_enc.aacenc_frame_size(), (char*)buffer, nbAac);
		size = nbAac;
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
	ctx->hsegmenter->pack(timestamp, buffer, size, ctx->encoding, flags);
}

//static void rtp_decode_packet(void* param, const void *packet, int bytes, uint32_t timestamp, int flags)
//{
//	static const uint8_t start_code[4] = { 0, 0, 0, 1 };
//	struct rtp_payload_test_t* ctx = (struct rtp_payload_test_t*)param;
//
//	static uint8_t buffer[2 * 1024 * 1024];
//	assert(bytes + 4 < sizeof(buffer));
//	assert(!(flags & 0x01)); //check if RTP_PACKET_LOST flag is set
//
//	size_t size = 0;
//	if (0 == strcmp("H264", ctx->encoding) || 0 == strcmp("H265", ctx->encoding))
//	{
//		memcpy(buffer, start_code, sizeof(start_code));
//		size += sizeof(start_code);
//	}
//	else if (0 == strcasecmp("mpeg4-generic", ctx->encoding))
//	{
//		int len = bytes + 7;
//		uint8_t profile = 2;
//		uint8_t sampling_frequency_index = 4;
//		uint8_t channel_configuration = 2;
//		buffer[0] = 0xFF; /* 12-syncword */
//		buffer[1] = 0xF0 /* 12-syncword */ | (0 << 3)/*1-ID*/ | (0x00 << 2) /*2-layer*/ | 0x01 /*1-protection_absent*/;
//		buffer[2] = ((profile - 1) << 6) | ((sampling_frequency_index & 0x0F) << 2) | ((channel_configuration >> 2) & 0x01);
//		buffer[3] = ((channel_configuration & 0x03) << 6) | ((len >> 11) & 0x03); /*0-original_copy*/ /*0-home*/ /*0-copyright_identification_bit*/ /*0-copyright_identification_start*/
//		buffer[4] = (uint8_t)(len >> 3);
//		buffer[5] = ((len & 0x07) << 5) | 0x1F;
//		buffer[6] = 0xFC | ((len / 1024) & 0x03);
//		size = 7;
//	}
//	memcpy(buffer + size, packet, bytes);
//	size += bytes;
//
//	//send to fmp4 segmenter
//	int keyframe = (flags & 0x02) ? 1 : 0; //check if RTP_I_FRAME flag is set
//	ctx->hsegmenter->hls_segmenter_mp4_send_packet(timestamp, buffer, size, ctx->encoding, flags);
//}

static int hls_fmp4_segment(void* m3u, const void* data, size_t bytes, int64_t, int64_t dts, int64_t duration) {
	static int i = 0;
	static char name[128] = { 0 };
	snprintf(name, sizeof(name), "hls_%d.mp4", ++i);
	FILE* fp = fopen(name, "wb");
	fwrite(data, 1, bytes, fp);
	fclose(fp);
	hls_m3u_t* m3u8 = (hls_m3u_t*)m3u;
	return m3u8->add_segment(duration, name);
}

static int hls_fmp4_init_segment(hls_fmp4_t* hls, hls_m3u_t* m3u) // todo: change to m3u
{
	int bytes = hls_fmp4_init_segment(hls, s_packet, sizeof(s_packet));
	FILE* fp=fopen("main.mp4", "wb");
	m3u->set_x_map("main.mp4");
	fwrite(s_packet, 1, bytes, fp);
	fclose(fp);
	return bytes;
}

//static int hls_init_segment(hls_fmp4_t* hls, hls_m3u8_t* m3u) // todo: change to m3u
//{
//	int bytes = hls_fmp4_init_segment(hls, s_packet, sizeof(s_packet));
//
//	FILE* fp = fopen("HLS/hls_0.mp4", "wb");
//
//	fwrite(s_packet, 1, bytes, fp);
//	fclose(fp);
//
//	return hls_m3u8_set_x_map(m3u, "hls_0.mp4");
//}
//
//static void ffmpeg_init()
//{
//	avcodec_register_all();
//	av_register_all();
//	avformat_network_init();
//}
//
//static AVFormatContext* ffmpeg_open(const char* url)
//{
//	int r;
//	AVFormatContext* ic;
//	AVDictionary* opt = NULL;
//	ic = avformat_alloc_context();
//	if (NULL == ic)
//	{
//		printf("%s(%s): avformat_alloc_context failed.\n", __FUNCTION__, url);
//		return NULL;
//	}
//
//	//if (!av_dict_get(ff->opt, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE)) {
//	//	av_dict_set(&ff->opt, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE);
//	//	scan_all_pmts_set = 1;
//	//}
//
//	r = avformat_open_input(&ic, url, NULL, &opt);
//	if (0 != r)
//	{
//		printf("%s: avformat_open_input(%s) => %d\n", __FUNCTION__, url, r);
//		return NULL;
//	}
//
//	//if (scan_all_pmts_set)
//	//	av_dict_set(&format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE);
//
//	//ff->ic->probesize = 100 * 1024;
//	//ff->ic->max_analyze_duration = 5 * AV_TIME_BASE;
//
//	/* If not enough info to get the stream parameters, we decode the
//	first frames to get it. (used in mpeg case for example) */
//	r = avformat_find_stream_info(ic, NULL/*&opt*/);
//	if (r < 0) {
//		printf("%s(%s): could not find codec parameters\n", __FUNCTION__, url);
//		return NULL;
//	}
//
//	av_dict_free(&opt);
//	return ic;
//}
//
//
//
//static int hls_segment(void* m3u8, const void* data, size_t bytes, int64_t /*pts*/, int64_t dts, int64_t duration)
//{
//	static int i = 0;
//	static char name[128] = { 0 };
//	snprintf(name, sizeof(name), "HLS/hls_%d.mp4", ++i);
//	FILE* fp = fopen(name, "wb");
//	fwrite(data, 1, bytes, fp);
//	fclose(fp);
//
//	return hls_m3u8_add((hls_m3u8_t*)m3u8, name, dts, duration, 0);
//}
//
//void hls_segmenter_fmp4_test(const char* file)
//{
//	ffmpeg_init();
//
//	AVPacket pkt;
//	memset(&pkt, 0, sizeof(pkt));
//	av_init_packet(&pkt);
//
//	AVFormatContext* ic = ffmpeg_open(file);
//	hls_m3u8_t* m3u = hls_m3u8_create(0, 7);
//	hls_fmp4_t* hls = hls_fmp4_create(HLS_DURATION * 1000, hls_segment, m3u);
//
//	int track_aac = -1;
//	int track_264 = -1;
//	int track_265 = -1;
//	for (unsigned int i = 0; i < ic->nb_streams; i++)
//	{
//		AVStream* st = ic->streams[i];
//		if (AV_CODEC_ID_AAC == st->codecpar->codec_id)
//			track_aac = hls_fmp4_add_audio(hls, MOV_OBJECT_AAC, st->codecpar->channels, st->codecpar->bits_per_coded_sample, st->codecpar->sample_rate, st->codecpar->extradata, st->codecpar->extradata_size);
//		else if (AV_CODEC_ID_H264 == st->codecpar->codec_id)
//			track_264 = hls_fmp4_add_video(hls, MOV_OBJECT_H264, st->codecpar->width, st->codecpar->height, st->codecpar->extradata, st->codecpar->extradata_size);
//		else if (AV_CODEC_ID_H265 == st->codecpar->codec_id)
//			track_265 = hls_fmp4_add_video(hls, MOV_OBJECT_HEVC, st->codecpar->width, st->codecpar->height, st->codecpar->extradata, st->codecpar->extradata_size);
//	}
//
//	// write init segment
//	hls_init_segment(hls, m3u);
//
//	int r = av_read_frame(ic, &pkt);
//	while (0 == r)
//	{
//		AVStream* st = ic->streams[pkt.stream_index];
//		int64_t pts = (int64_t)(pkt.pts * av_q2d(st->time_base) * 1000);
//		int64_t dts = (int64_t)(pkt.dts * av_q2d(st->time_base) * 1000);
//		if (AV_CODEC_ID_AAC == st->codecpar->codec_id)
//		{
//			//printf("[A] pts: %08lld, dts: %08lld\n", pts, dts);
//			hls_fmp4_input(hls, track_aac, pkt.data, pkt.size, pts, dts, 0);
//		}
//		else if (AV_CODEC_ID_H264 == st->codecpar->codec_id)
//		{
//			//printf("[V] pts: %08lld, dts: %08lld\n", pts, dts);
//			hls_fmp4_input(hls, track_264, pkt.data, pkt.size, pts, dts, (pkt.flags & AV_PKT_FLAG_KEY) ? MOV_AV_FLAG_KEYFREAME : 0);
//		}
//		else if (AV_CODEC_ID_H265 == st->codecpar->codec_id)
//		{
//			//printf("[V] pts: %08lld, dts: %08lld\n", pts, dts);
//			hls_fmp4_input(hls, track_265, pkt.data, pkt.size, pts, dts, (pkt.flags & AV_PKT_FLAG_KEY) ? MOV_AV_FLAG_KEYFREAME : 0);
//		}
//		else
//		{
//			assert(0);
//		}
//
//		av_packet_unref(&pkt);
//		r = av_read_frame(ic, &pkt);
//	}
//
//	avformat_close_input(&ic);
//	avformat_free_context(ic);
//	hls_fmp4_destroy(hls);
//
//	// write m3u8 file
//	hls_m3u8_playlist(m3u, 1, s_packet, sizeof(s_packet));
//	hls_m3u8_destroy(m3u);
//
//	FILE* fp = fopen("HLS/playlist.m3u8", "wb");
//	auto nbytes = fwrite(s_packet, 1, strlen(s_packet), fp);
//	fclose(fp);
//}
//
//int main() {
//	hls_segmenter_fmp4_test("rtsp://admin2:456redko@192.168.17.106/onvif-media/media.amp?profile=arthur_h264&sessiontimeout=60&streamtype=unicast");
//	return 0;
//}

#pragma pack(push, 1)
struct rtpHeaderSecondByte {
	uint8_t M : 1;
	uint8_t PT : 7;
};
#pragma pack(pop)

int hls_segmenter::send_rtp(const void * packet, int size)
{
	rtpHeaderSecondByte *rtpHeader2byte = (rtpHeaderSecondByte *)((uint8_t*)packet + 1);
	if (ctxVideo != NULL) {
		if (rtpHeader2byte->PT == ctxVideo->payload)
			return rtp_payload_decode_input(ctxVideo->decoder, packet, size);
	}
	else if (ctxAudio != NULL) {
		if (rtpHeader2byte->PT == ctxAudio->payload)
			return rtp_payload_decode_input(ctxAudio->decoder, packet, size); // return 0 if packet discard, 1 - sucsess
		else
			return -1; // stream with this payload number is uninitialized
	}
	else
		return -2; // no streams initialized
}

int hls_segmenter::pack(int64_t timestamp, const void *packet, int size, const char *encoding, int keyframe = 0) {
		double time_base = 1 / 90000; // standart of h264 is 90KHz
		int64_t pts, dts;
		//pts = (int64_t)((timestamp - first_timestamp) * time_base * 1000);
		pts = (int64_t)(timestamp * time_base * 1000);
		dts = pts;
		if (0 == strcmp("AAC", encoding))
		{
			//printf("[A] pts: %08lld, dts: %08lld\n", pts, dts);
			return hls_fmp4_input(hls, track_aac, packet, size, pts, dts, 0);
		}
		else if (0 == strcmp("H264", encoding))
		{
			//printf("[V] pts: %08lld, dts: %08lld\n", pts, dts);
			return hls_fmp4_input(hls, track_264, packet, size, pts, dts, (keyframe == 1) ? MOV_AV_FLAG_KEYFREAME : 0);
		}
		else if (0 == strcmp("H265", encoding))
		{
			//printf("[V] pts: %08lld, dts: %08lld\n", pts, dts);
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

hls_segmenter::hls_segmenter(int duration, int playlist_capacity = 20, int remaining = 6)
{
	init(duration, playlist_capacity, remaining);
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

int hls_segmenter::init(int duration, int playlist_capasity = 20, int playlist_remaining = 6) {
	hls_m3u_t m3u(7, 0, duration, playlist_capasity, playlist_remaining, "playlist.m3u8");
	this->hls = hls_fmp4_create(duration * 1000, hls_fmp4_segment, NULL);
	this->m3u8 = m3u;
	return hls_fmp4_init_segment(hls, &m3u);
}

void hls_segmenter::init_video_stream(const char* encoding, uint8_t * sps, int sps_size)
{
	constexpr int NALU_offset = 5;
	std::pair<int, int> width_height_pair = Parse(sps + NALU_offset, sps_size - NALU_offset); //width and height
	width = width_height_pair.first;
	height = width_height_pair.second;
	extra_data = sps + NALU_offset;
	extra_data_size = sps_size;
	if (0 == strcmp("H264", encoding))
		track_264 = hls_fmp4_add_video(hls, MOV_OBJECT_H264, width, height, extra_data, extra_data_size);
	else if (0 == strcmp("H265", encoding) || 0 == strcmp("HEVC", encoding))
		track_265 = hls_fmp4_add_video(hls, MOV_OBJECT_HEVC, width, height, extra_data, extra_data_size);
	struct rtp_payload_test_t Video;
	Video.payload = 96;
	Video.encoding = encoding;
	struct rtp_payload_t handler;
	handler.alloc = rtp_alloc;
	handler.free = rtp_free;
	handler.packet = rtp_decode_packet;
	Video.decoder = rtp_payload_decode_create(Video.payload, Video.encoding, &handler, &Video);
	ctxVideo = &Video;
	this->ctxVideo->hsegmenter = this;
}

void hls_segmenter::init_audio_stream(int payload, const char* encoding, int channels, int bits_per_coded_sample, int sample_rate)
{
	this->channels = channels;
	this->bits_per_coded_sample = bits_per_coded_sample;
	this->sample_rate = sample_rate;
	track_aac = hls_fmp4_add_audio(hls, MOV_OBJECT_AAC, channels, bits_per_coded_sample, sample_rate, NULL, NULL);
	struct rtp_payload_test_t ctxAudio;
	ctxAudio.payload = payload;
	ctxAudio.encoding = encoding;
	struct rtp_payload_t handler;
	handler.alloc = rtp_alloc;
	handler.free = rtp_free;
	handler.packet = rtp_decode_packet;
	ctxAudio.decoder = rtp_payload_decode_create(ctxAudio.payload, ctxAudio.encoding, &handler, &ctxAudio);
	this->ctxAudio = &ctxAudio;
	this->ctxAudio->hsegmenter = this;
	if (payload < 96)
		fdkaac_enc.aacenc_init(2, channels, sample_rate, 64000); // 2 - audio object type - aac (lc), 64kbit - pcmu/pcma standard
}

std::string hls_segmenter::playlist()
{
	return m3u8.playlist_request();
}

int hls_segmenter::destroy() {
	//freeing all resourses
	m3u8.set_x_endlist();
	fdkaac_enc.aacenc_close();
	return 0;
}