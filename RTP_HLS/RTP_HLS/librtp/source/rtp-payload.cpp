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

void* rtp_payload_decode_create(int payload, const char* name, struct rtp_payload_t *handler, void* cbparam)
{
	struct rtp_payload_delegate_t* ctx;
	ctx = (rtp_payload_delegate_t*)calloc(1, sizeof(*ctx));
	if (ctx)
	{
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
	if (payload >= 96 && encoding)
	{
		if (0 == strcasecmp(encoding, "H264"))
		{
			// H.264 video (MPEG-4 Part 10) (RFC 6184)
			codec->decoder = rtp_h264_decode();
		}
		else if (0 == strcasecmp(encoding, "H265") || 0 == strcasecmp(encoding, "HEVC"))
		{
			// H.265 video (HEVC) (RFC 7798)
			codec->decoder = rtp_h265_decode();
		}
		else if (0 == strcasecmp(encoding, "mpeg4-generic"))
		{
			/// RFC3640 RTP Payload Format for Transport of MPEG-4 Elementary Streams
			/// 4.1. MIME Type Registration (p27)
			codec->decoder = rtp_mpeg4_generic_decode();
		}
		else if (0 == strcasecmp(encoding, "MP4A-LATM"))
		{
			// RFC6416 RTP Payload Format for MPEG-4 Audio/Visual Streams
			// 6. RTP Packetization of MPEG-4 Audio Bitstreams (p15)
			// 7.3 Media Type Registration for MPEG-4 Audio (p21)
			codec->decoder = rtp_mp4a_latm_decode();
		}
	}
	else {
		switch (payload)
		{
		case RTP_PAYLOAD_PCMU: // ITU-T G.711 PCM u-Law audio 64 kbit/s (RFC 3551)
		case RTP_PAYLOAD_PCMA: // ITU-T G.711 PCM A-Law audio 64 kbit/s (RFC 3551)
		case RTP_PAYLOAD_G722: // ITU-T G.722 audio 64 kbit/s (RFC 3551)
		case RTP_PAYLOAD_G729: // ITU-T G.729 and G.729a audio 8 kbit/s (RFC 3551)
			codec->decoder = rtp_common_decode();
			break;
		default:
			return -1; // not support
		}
	}
	return 0;
}