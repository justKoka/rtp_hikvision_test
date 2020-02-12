#include "../include/rtp-payload.h"
#include "../include/rtp-profile.h"
#include "../include/rtp-packet.h"
#include "../include/rtp-payload-internal.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define TS_PACKET_SIZE 188

#if defined(OS_WINDOWS)
#define strcasecmp _stricmp
#endif

struct rtp_payload_delegate_t
{
	struct rtp_payload_encode_t* encoder;
	struct rtp_payload_decode_t* decoder;
	void* packer;
};

/// @return 0-ok, <0-error
static int rtp_payload_find(int payload, const char* encoding, struct rtp_payload_delegate_t* codec);

//void* rtp_payload_encode_create(int payload, const char* name, uint16_t seq, uint32_t ssrc, struct rtp_payload_t *handler, void* cbparam)
//{
//	int size;
//	struct rtp_payload_delegate_t* ctx;
//
//	ctx = (rtp_payload_delegate_t*)calloc(1, sizeof(*ctx));
//	if (ctx)
//	{
//		size = rtp_packet_getsize();
//		if (rtp_payload_find(payload, name, ctx) < 0
//			|| NULL == (ctx->packer = ctx->encoder->create(size, (uint8_t)payload, seq, ssrc, handler, cbparam)))
//		{
//			free(ctx);
//			return NULL;
//		}
//	}
//	return ctx;
//}

void rtp_payload_encode_destroy(void* encoder)
{
	struct rtp_payload_delegate_t* ctx;
	ctx = (struct rtp_payload_delegate_t*)encoder;
	ctx->encoder->destroy(ctx->packer);
	free(ctx);
}

void rtp_payload_encode_getinfo(void* encoder, uint16_t* seq, uint32_t* timestamp)
{
	struct rtp_payload_delegate_t* ctx;
	ctx = (struct rtp_payload_delegate_t*)encoder;
	ctx->encoder->get_info(ctx->packer, seq, timestamp);
}

int rtp_payload_encode_input(void* encoder, const void* data, int bytes, uint32_t timestamp)
{
	struct rtp_payload_delegate_t* ctx;
	ctx = (struct rtp_payload_delegate_t*)encoder;
	return ctx->encoder->input(ctx->packer, data, bytes, timestamp);
}

void* rtp_payload_decode_create(int payload, const char* name, struct rtp_payload_t *handler, void* cbparam)
{
	struct rtp_payload_delegate_t* ctx;
	ctx = (rtp_payload_delegate_t*)calloc(1, sizeof(*ctx));
	if (ctx)
	{
		//ctx->encoder = rtp_h264_encode();
		/*ctx->decoder = rtp_h264_decode();*/
		if (rtp_payload_find(payload, name, ctx) < 0
			|| NULL == (ctx->packer = ctx->decoder->create(handler, cbparam)))
		{
			free(ctx);
			return NULL;
		}
	}
	return ctx;
}

void rtp_payload_decode_destroy(void* decoder)
{
	struct rtp_payload_delegate_t* ctx;
	ctx = (struct rtp_payload_delegate_t*)decoder;
	ctx->decoder->destroy(ctx->packer);
	free(ctx);
}

int rtp_payload_decode_input(void* decoder, const void* packet, int bytes)
{
	struct rtp_payload_delegate_t* ctx;
	ctx = (struct rtp_payload_delegate_t*)decoder;
	return ctx->decoder->input(ctx->packer, packet, bytes);
}

// Default max packet size (1500, minus allowance for IP, UDP, UMTP headers)
// (Also, make it a multiple of 4 bytes, just in case that matters.)
//static int s_max_packet_size = 1456; // from Live555 MultiFrameRTPSink.cpp RTP_PAYLOAD_MAX_SIZE
//static size_t s_max_packet_size = 576; // UNIX Network Programming by W. Richard Stevens
static int s_max_packet_size = 1434; // from VLC

void rtp_packet_setsize(int bytes)
{
	s_max_packet_size = bytes < 564 ? 564 : bytes;
}

int rtp_packet_getsize()
{
	return s_max_packet_size;
}

static int rtp_payload_find(int payload, const char* encoding, struct rtp_payload_delegate_t* codec)
{
	assert(payload >= 0 && payload <= 127);
	if (payload >= 96 && encoding)
	{
		if (0 == strcasecmp(encoding, "H264"))
		{
			// H.264 video (MPEG-4 Part 10) (RFC 6184)
			/*codec->encoder = rtp_h264_encode();*/
			codec->decoder = rtp_h264_decode();
		}
		else if (0 == strcasecmp(encoding, "H265") || 0 == strcasecmp(encoding, "HEVC"))
		{
			// H.265 video (HEVC) (RFC 7798)
			/*codec->encoder = rtp_h265_encode();*/
			codec->decoder = rtp_h265_decode();
		}
	}
	else {
		switch (payload)
		{
		case RTP_PAYLOAD_PCMU: // ITU-T G.711 PCM u-Law audio 64 kbit/s (RFC 3551)
		case RTP_PAYLOAD_PCMA: // ITU-T G.711 PCM A-Law audio 64 kbit/s (RFC 3551)
		case RTP_PAYLOAD_G722: // ITU-T G.722 audio 64 kbit/s (RFC 3551)
		case RTP_PAYLOAD_G729: // ITU-T G.729 and G.729a audio 8 kbit/s (RFC 3551)
			//codec->encoder = rtp_common_encode();
			codec->decoder = rtp_common_decode();
			break;

		//case RTP_PAYLOAD_MPA: // MPEG-1 or MPEG-2 audio only (RFC 3551, RFC 2250)
		//case RTP_PAYLOAD_MPV: // MPEG-1 and MPEG-2 video (RFC 2250)
		//	codec->encoder = rtp_mpeg1or2es_encode();
		//	codec->decoder = rtp_mpeg1or2es_decode();
		//	break;

		//case RTP_PAYLOAD_MP2T: // MPEG-2 transport stream (RFC 2250)
		//	codec->encoder = rtp_ts_encode();
		//	codec->decoder = rtp_ts_decode();
		//	break;

		//case RTP_PAYLOAD_JPEG:
		//case RTP_PAYLOAD_H263:
		//	return -1; // TODO
		//	break;
		default:
			return -1; // not support
		}
	}
	return 0;
}