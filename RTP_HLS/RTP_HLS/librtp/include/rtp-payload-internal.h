#pragma once
#include "rtp-payload.h"
#include "rtp-packet.h"
#include "rtp-util.h"

struct rtp_payload_encode_t
{
	/// create RTP packer
	/// @param[in] size maximum RTP packet payload size(don't include RTP header)
	/// @param[in] payload RTP header PT filed (see more about rtp-profile.h)
	/// @param[in] seq RTP header sequence number filed
	/// @param[in] ssrc RTP header SSRC filed
	/// @param[in] handler user-defined callback
	/// @param[in] cbparam user-defined parameter
	/// @return RTP packer
	void* (*create)(int size, uint8_t payload, uint16_t seq, uint32_t ssrc, struct rtp_payload_t *handler, void* cbparam);
	/// destroy RTP Packer
	void(*destroy)(void* packer);

	void(*get_info)(void* packer, uint16_t* seq, uint32_t* timestamp);

	/// PS/H.264 Elementary Stream to RTP Packet
	/// @param[in] packer
	/// @param[in] data stream data
	/// @param[in] bytes stream length in bytes
	/// @param[in] time stream UTC time
	/// @return 0-ok, ENOMEM-alloc failed, <0-failed
	int(*input)(void* packer, const void* data, int bytes, uint32_t time);
};

struct rtp_payload_decode_t
{
	void* (*create)(struct rtp_payload_t *handler, void* param);
	void(*destroy)(void* packer);

	/// RTP packet to PS/H.264 Elementary Stream
	/// @param[in] decoder RTP packet unpackers
	/// @param[in] packet RTP packet
	/// @param[in] bytes RTP packet length in bytes
	/// @param[in] time stream UTC time
	/// @return 1-packet handled, 0-packet discard, <0-failed
	int(*input)(void* decoder, const void* packet, int bytes);
};

struct rtp_payload_encode_t *rtp_ts_encode();
struct rtp_payload_encode_t *rtp_vp8_encode();
struct rtp_payload_encode_t *rtp_vp9_encode();
struct rtp_payload_encode_t *rtp_h264_encode();
struct rtp_payload_encode_t *rtp_h265_encode();
struct rtp_payload_encode_t *rtp_common_encode();
struct rtp_payload_encode_t *rtp_mp4v_es_encode();
struct rtp_payload_encode_t *rtp_mp4a_latm_encode();
struct rtp_payload_encode_t *rtp_mpeg4_generic_encode();
struct rtp_payload_encode_t *rtp_mpeg1or2es_encode();

struct rtp_payload_decode_t *rtp_ts_decode();
struct rtp_payload_decode_t *rtp_vp8_decode();
struct rtp_payload_decode_t *rtp_vp9_decode();
struct rtp_payload_decode_t *rtp_h264_decode();
struct rtp_payload_decode_t *rtp_h265_decode();
struct rtp_payload_decode_t *rtp_common_decode();
struct rtp_payload_decode_t *rtp_mp4v_es_decode();
struct rtp_payload_decode_t *rtp_mp4a_latm_decode();
struct rtp_payload_decode_t *rtp_mpeg4_generic_decode();
struct rtp_payload_decode_t *rtp_mpeg1or2es_decode();

int rtp_packet_serialize_header(const struct rtp_packet_t *pkt, void* data, int bytes);