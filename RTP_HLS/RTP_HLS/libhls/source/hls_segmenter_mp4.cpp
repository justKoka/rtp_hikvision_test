#include "../include/hls_segmenter_mp4.h"
#include "../../librtp/include/sps_parse.h"

static char s_packet[2 * 1024 * 1024];

static void* rtp_alloc(void* /*param*/, int bytes)
{
	static uint8_t buffer[2 * 1024 * 1024 + 4] = { 0, 0, 0, 1, };
	assert(bytes <= sizeof(buffer) - 4);
	return buffer + 4;
}

static void ffmpeg_init()
{
	avcodec_register_all();
	av_register_all();
	avformat_network_init();
}

static AVFormatContext* ffmpeg_open(const char* url)
{
	int r;
	AVFormatContext* ic;
	AVDictionary* opt = NULL;
	ic = avformat_alloc_context();
	if (NULL == ic)
	{
		printf("%s(%s): avformat_alloc_context failed.\n", __FUNCTION__, url);
		return NULL;
	}

	//if (!av_dict_get(ff->opt, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE)) {
	//	av_dict_set(&ff->opt, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE);
	//	scan_all_pmts_set = 1;
	//}

	r = avformat_open_input(&ic, url, NULL, &opt);
	if (0 != r)
	{
		printf("%s: avformat_open_input(%s) => %d\n", __FUNCTION__, url, r);
		return NULL;
	}

	//if (scan_all_pmts_set)
	//	av_dict_set(&format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE);

	//ff->ic->probesize = 100 * 1024;
	//ff->ic->max_analyze_duration = 5 * AV_TIME_BASE;

	/* If not enough info to get the stream parameters, we decode the
	first frames to get it. (used in mpeg case for example) */
	r = avformat_find_stream_info(ic, NULL/*&opt*/);
	if (r < 0) {
		printf("%s(%s): could not find codec parameters\n", __FUNCTION__, url);
		return NULL;
	}

	av_dict_free(&opt);
	return ic;
}

static int hls_init_segment(hls_fmp4_t* hls, hls_m3u8_t* m3u)
{
	int bytes = hls_fmp4_init_segment(hls, s_packet, sizeof(s_packet));

	FILE* fp = fopen("HLS/hls_0.mp4", "wb");

	fwrite(s_packet, 1, bytes, fp);
	fclose(fp);

	return hls_m3u8_set_x_map(m3u, "hls_0.mp4");
}

static int hls_segment(void* m3u8, const void* data, size_t bytes, int64_t /*pts*/, int64_t dts, int64_t duration)
{
	static int i = 0;
	static char name[128] = { 0 };
	snprintf(name, sizeof(name), "HLS/hls_%d.mp4", ++i);
	FILE* fp = fopen(name, "wb");
	fwrite(data, 1, bytes, fp);
	fclose(fp);

	return hls_m3u8_add((hls_m3u8_t*)m3u8, name, dts, duration, 0);
}

static int hls_ts_segment(void* m3u, const void* data, size_t bytes, int64_t, int64_t dts, int64_t duration) {
	static int i = 0;
	static char name[128] = { 0 };
	snprintf(name, sizeof(name), "HLS/hls_%d.mp4", ++i);
	FILE* fp = fopen(name, "wb");
	fwrite(data, 1, bytes, fp);
	fclose(fp);
	return add_segment((hls_m3u_t*)m3u, duration, name);
}

void hls_segmenter_fmp4_test(const char* file)
{
	ffmpeg_init();

	AVPacket pkt;
	memset(&pkt, 0, sizeof(pkt));
	av_init_packet(&pkt);

	AVFormatContext* ic = ffmpeg_open(file);
	hls_m3u8_t* m3u = hls_m3u8_create(0, 7);
	hls_fmp4_t* hls = hls_fmp4_create(HLS_DURATION * 1000, hls_segment, m3u);

	int track_aac = -1;
	int track_264 = -1;
	int track_265 = -1;
	for (unsigned int i = 0; i < ic->nb_streams; i++)
	{
		AVStream* st = ic->streams[i];
		if (AV_CODEC_ID_AAC == st->codecpar->codec_id)
			track_aac = hls_fmp4_add_audio(hls, MOV_OBJECT_AAC, st->codecpar->channels, st->codecpar->bits_per_coded_sample, st->codecpar->sample_rate, st->codecpar->extradata, st->codecpar->extradata_size);
		else if (AV_CODEC_ID_H264 == st->codecpar->codec_id)
			track_264 = hls_fmp4_add_video(hls, MOV_OBJECT_H264, st->codecpar->width, st->codecpar->height, st->codecpar->extradata, st->codecpar->extradata_size);
		else if (AV_CODEC_ID_H265 == st->codecpar->codec_id)
			track_265 = hls_fmp4_add_video(hls, MOV_OBJECT_HEVC, st->codecpar->width, st->codecpar->height, st->codecpar->extradata, st->codecpar->extradata_size);
	}

	// write init segment
	hls_init_segment(hls, m3u);

	int r = av_read_frame(ic, &pkt);
	while (0 == r)
	{
		AVStream* st = ic->streams[pkt.stream_index];
		int64_t pts = (int64_t)(pkt.pts * av_q2d(st->time_base) * 1000);
		int64_t dts = (int64_t)(pkt.dts * av_q2d(st->time_base) * 1000);
		if (AV_CODEC_ID_AAC == st->codecpar->codec_id)
		{
			//printf("[A] pts: %08lld, dts: %08lld\n", pts, dts);
			hls_fmp4_input(hls, track_aac, pkt.data, pkt.size, pts, dts, 0);
		}
		else if (AV_CODEC_ID_H264 == st->codecpar->codec_id)
		{
			//printf("[V] pts: %08lld, dts: %08lld\n", pts, dts);
			hls_fmp4_input(hls, track_264, pkt.data, pkt.size, pts, dts, (pkt.flags & AV_PKT_FLAG_KEY) ? MOV_AV_FLAG_KEYFREAME : 0);
		}
		else if (AV_CODEC_ID_H265 == st->codecpar->codec_id)
		{
			//printf("[V] pts: %08lld, dts: %08lld\n", pts, dts);
			hls_fmp4_input(hls, track_265, pkt.data, pkt.size, pts, dts, (pkt.flags & AV_PKT_FLAG_KEY) ? MOV_AV_FLAG_KEYFREAME : 0);
		}
		else
		{
			assert(0);
		}

		av_packet_unref(&pkt);
		r = av_read_frame(ic, &pkt);
	}

	avformat_close_input(&ic);
	avformat_free_context(ic);
	hls_fmp4_destroy(hls);

	// write m3u8 file
	hls_m3u8_playlist(m3u, 1, s_packet, sizeof(s_packet));
	hls_m3u8_destroy(m3u);

	FILE* fp = fopen("HLS/playlist.m3u8", "wb");
	auto nbytes = fwrite(s_packet, 1, strlen(s_packet), fp);
	fclose(fp);
}

int main() {
	hls_segmenter_fmp4_test("rtsp://admin2:456redko@192.168.17.106/onvif-media/media.amp?profile=arthur_h264&sessiontimeout=60&streamtype=unicast");
	return 0;
}

int hls_segmenter_mp4_send_packet(hls_segmenter* hls_segmenter, int64_t timestamp, int track, const void *packet, int size, const char *encoding, int keyframe) {
		double time_base = 1 / 90000; // standart of h264 is 90KHz
		int64_t pts, dts;
		pts = (int64_t)((timestamp - hls_segmenter->first_timestamp) * time_base * 1000);
		//pts = (int64_t)(timestamp * time_base * 1000);
		dts = pts;
		if (0 == strcmp("AAC", encoding))
		{
			//printf("[A] pts: %08lld, dts: %08lld\n", pts, dts);
			return hls_fmp4_input(hls_segmenter->hls, track, packet, size, pts, dts, 0);
		}
		else if (0 == strcmp("H264", encoding))
		{
			//printf("[V] pts: %08lld, dts: %08lld\n", pts, dts);
			return hls_fmp4_input(hls_segmenter->hls, track, packet, size, pts, dts, (!keyframe) ? MOV_AV_FLAG_KEYFREAME : 0);
		}
		else if (0 == strcmp("H265", encoding))
		{
			//printf("[V] pts: %08lld, dts: %08lld\n", pts, dts);
			return hls_fmp4_input(hls_segmenter->hls, track, packet, size, pts, dts, (!keyframe) ? MOV_AV_FLAG_KEYFREAME : 0);
		}
		else
		{
			assert(0);
		}
}

int hls_segmenter_mp4_init(hls_segmenter* hls_segmenter, uint8_t* sps, int sps_size, int duration /* one segment duration in seconds */, int nb_streams, codec codecs[]) {
	hls_m3u_t m3u(7, 0, duration, 12, 6, "playlist.m3u8");
	//hls_fmp4_t* hls = hls_fmp4_create(HLS_DURATION * 1000, hls_segment, m3u);
	hls_fmp4_t* hls = hls_fmp4_create(duration * 1000, hls_segment, NULL);
	
	constexpr int NALU_offset = 5;
	Parse(hls_segmenter, sps + NALU_offset, sps_size - NALU_offset); //width and height
	hls_segmenter->extra_data = sps + NALU_offset;
	hls_segmenter->extra_data_size = sps_size;

	//fetching audio params

	struct rtp_payload_test_t ctxVideo;
	ctxVideo.payload = 96;
	ctxVideo.encoding = "H264";
	struct rtp_payload_t handler;
	handler.alloc = rtp_alloc;
	handler.free = rtp_free;
	handler.packet = rtp_decode_packet;
	ctx.decoder = rtp_payload_decode_create(ctx.payload, ctx.encoding, &handler, &ctx);

	for (unsigned int i = 0; i < nb_streams; i++)
	{
		if (0 == strcmp("AAC", codecs[i].encoding))
			hls_segmenter->track_aac = hls_fmp4_add_audio(hls, MOV_OBJECT_AAC, hls_segmenter->channels, hls_segmenter->bits_per_coded_sample, hls_segmenter->sample_rate, hls_segmenter->extra_data, hls_segmenter->extra_data_size);
		else if (0 == strcmp("H264", codecs[i].encoding))
			hls_segmenter->track_264 = hls_fmp4_add_video(hls, MOV_OBJECT_H264, hls_segmenter->width, hls_segmenter->height, hls_segmenter->extra_data, hls_segmenter->extra_data_size);
		else if (0 == strcmp("H265", codecs[i].encoding))
			hls_segmenter->track_265 = hls_fmp4_add_video(hls, MOV_OBJECT_HEVC, hls_segmenter->width, hls_segmenter->height, hls_segmenter->extra_data, hls_segmenter->extra_data_size);
	}
	
	hls_segmenter->hls = hls;
	hls_segmenter->m3u8 = &m3u;
}

int hls_segmenter_mp4_destroy() {
	//set #EXT-X-ENDLIST in the end of m3u8	
}