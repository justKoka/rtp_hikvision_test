#include <cstdio>
#include <iostream>
#include <cstring>
#include "libhls/include/hls_segmenter_mp4.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct rtp_payload_test_t
{
	int payload;
	const char* encoding;
	int track; // number of track in fmp4
	void* encoder;
	void* decoder;
	size_t size;
	uint8_t *packet;
	hls_segmenter *hsegmenter;
	/*uint8_t packet[64 * 1024];*/
};

static int convertCharStreamToHex(const char input[], uint8_t *output) {
	int j = 0;
	for (int i = 0; i < strlen(input); i += 2) {
		output[j] = (isalpha(input[i]) ? input[i] - 87 : input[i] - 48) * 16 + (isalpha(input[i + 1]) ? input[i + 1] - 87 : input[i + 1] - 48);
		j++;
	}
	return 1;
}

static void* rtp_alloc(void* /*param*/, int bytes)
{
	static uint8_t buffer[2 * 1024 * 1024 + 4] = { 0, 0, 0, 1, };
	assert(bytes <= sizeof(buffer) - 4);
	return buffer + 4;
}

static void rtp_free(void* /*param*/, void * /*packet*/)
{
}

static void rtp_decode_packet(void* param, const void *packet, int bytes, uint32_t timestamp, int flags)
{
	static const uint8_t start_code[4] = { 0, 0, 0, 1 };
	struct rtp_payload_test_t* ctx = (struct rtp_payload_test_t*)param;

	static uint8_t buffer[2 * 1024 * 1024];
	assert(bytes + 4 < sizeof(buffer));
	assert(!(flags & 0x01)); //check if RTP_PACKET_LOST flag is set

	size_t size = 0;
	if (0 == strcmp("H264", ctx->encoding) || 0 == strcmp("H265", ctx->encoding))
	{
		memcpy(buffer, start_code, sizeof(start_code));
		size += sizeof(start_code);
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
	}
	memcpy(buffer + size, packet, bytes);
	size += bytes;

	//send to fmp4 segmenter
	int keyframe = (flags & 0x02) ? 1 : 0; //check if RTP_I_FRAME flag is set
	hls_segmenter_mp4_send_packet(ctx->hsegmenter, timestamp, ctx->track, buffer, size, ctx->encoding, flags);
}

//static int send_rtp_to_segmenter(void *decoder, const void* rtp, int bytes, int track) {
//	rtp_payload_decode_input(decoder, rtp, bytes);
//}

//int main()
//{
//	struct rtp_payload_test_t ctx;
//	ctx.payload = 96;
//	ctx.encoding = "H264";
//	struct rtp_payload_t handler;
//	handler.alloc = rtp_alloc;
//	handler.free = rtp_free;
//	handler.packet = rtp_decode_packet;
//	ctx.decoder = rtp_payload_decode_create(ctx.payload, ctx.encoding, &handler, &ctx);
//	uint8_t rtp_packet[1500];
//	convertCharStreamToHex("806048623dadf22e069e3f085c81e7a807d77227ff0016c1c985e4bb96aded812cf6d0d863031400f8b5346287d9dd811b2d4b035e9caa7db68c1d70dbfcf922dce1b3254b0d1fbddc33758adcbf187e2159f7bfd102ced3a53434d5cefc08575699b1619e44e47f517e8d8969797baf1c0f35a4d10ed34e5898cde03da37e315760ff584ec9643347443ffcb56985199af2d2f391f7247ae4bed1024bec19419e73981a1c54e93c61a96700bbc1dfea534f3da69c8007e738a6a96bd477c9f21fde07b493c484f0b4e2b7ee6f454dffe11d58e81aec0e253586c466273ee225452efcb25a179aceb093aebb236b8e46d5b2f4f98dae1ad55c4c7e6b857b99ac51d3c42634f4a4bd652a1ad82bb8f9c142e8147223a6289b559923152362d0dccc0d2a888c90d45e14a8ff6d2e1f27bb438fe9730ac34c8fbde182e15a760cd59332e556a472af3e7bac31ff363b1f673172d77d5f3002e04e6dfdb2b10e8988a8d41fa3e73f29c62514ed2c3f22d1a99f9f4fc0dfee6bbd700335798a1a107ad44e02b16fb84381f1acf0089148f8f69359fbb971b03963b49b4a0045890a3442c023f7230a32d2a05398ca4eb5d2edd4fc7e093c3354559ea05e6eb91bfa1c1fac28736a92aa47e6db564fe69a885bf9356832d9b299ec546aa6ba563580e55047993fd3b7ff7e1934b12c713eaed7a880daf9f00fae2f681636fe22d31823b8415e8041e01e95a6ed9fcea1ca8e325d0994c81c621d8f5c73f8e2ddd81a359797c5cb598af47ce0f15eb743af09920d2de0779b3b257b668ee4decb8046f7f48a1c45ff9dcbdd87a041466f334459de1acd10c18e63e9a94a3c9a3b0dc3fdca7d2bb7cf7871da125ba551dd827207dbb464f1f7f7961e28e0febc84b84c943bcd160aa561787434aa36adcae0fc2e0e4bc5cca67308f732d2483526315b6b5a7afb6a92168666d8f6eca4a0513a8b552091742562041709cb3e10e871a51f18cd7b6533e319fb76411fa2e0677686c90cd3b47b666dd0b55944f96c93ba394eca5910cf6ed806e3c54c110a2327c14422853ae3d9633048aab5eaad10e85f4ca2ec3d5db6241a2a1aee9f7997e9079a41938a8de945267f7c44d93cd677e3a558741e053ddd6109a3c736ce2a84adb0a89532f65e56610fddc9655eb032bb0299a40a4f3cae249325eb0ca2298340075fb9aba7864df6c664fd815c0f2af6775d6cb8197b0ea8340239a3effdf73e5f2544637591fdc387584a08ebb89ec239b61c53dd5aca3b66781e5a3dd589fb4077e8bf021fbc69710f34fa9432223ff6556eec070e558550a39ee8662f749587cdcea57a33097de0c7b4ea9fc30be85f02a44b843b68041a0f59dcbb35264be55e58ecfda620c0d7a4af37ad4867637a1ac475e824231441dd86aba8a5f007b31e61e175b531d902e56a3b93829a136d6c800d45cd1e6c8ef7bd022db732821850a4a07e82b64109da8c7a9165621d2edefda1857af2acee635ed68442f35c62b3710f27db4d356750080f7b9c1b9a4570b0a5c3116cadcf4b179299177086acd69041b72377996112ab71756261286d7202ce7b6c3a1db3e4f23b83c05ff84a3bed865301789798864176886e85a355982f2d5b0c4e200a77fe59cab3cf6d138b07448f6e1bc23b6d68dd67e8368d72f54e11eecc3d1525a932ce382eb02c6e496d7deeba04d6fff4696f74f91b4ed813abb46a4e72ac4ad018f3bbd85f855595751614567010011037deb746a6bf79769e4812d4abf4e1a335299c4a96e93ecddc1a4b2c0c91c12abb38fdb0c4f7fca4ed234e19cc77a751e40ffcf1121197f5faa0ca7c4b79b99e3f4a9450537e55e3970131cd6bba6cb28a714822b54417c4e1582bcd7fbc1cb5370e5e4d08663ecbf104d40087466547a6d8a24e753f8ffe8608d8834551b5fdc3adadad28096d383d9b466b665413799dd69e80130ab874d0fa75a2a1345757c49ec6fc3a708d819df5ebe6809669f4be819cc890b3865133cf94aac5a3b66132720c3d56a6", rtp_packet);
//	ctx.size = 1440;
//	ctx.packet = rtp_packet;
//	rtp_payload_decode_input(ctx.decoder, ctx.packet, ctx.size);
//	convertCharStreamToHex("80e048633dadf22e069e3f085c4134999ddf7123daa04ef8e6639743801f363ee88a63acd93e1a9d784cf45c31883e548e7ae45a54af32a6d55071a67d245836163a2a7d4c0a19100ba56dac9140774954dc31671655bb772e519bcec8f012a6ca681348d0aca14a650a7df6f2bbb8c8a3484bf509565b704b3a520f302071240971a5ccb3d7d1d553660680fcb0702689478f4cf6a7ae2ab0c3818b051633069aad617121ff03ab4ad0a08efee31696ddbbade56068c1ceeeaa8ccc9d1d5456b00668d4c0aa3e0e0c2964662ba639c87ca25a691d9fd7d893723bf0dbc091075127ab5f86b72df057336dd5e416c8b0bb27f06bf48aa990c1c63364085f67f7c75e9f00e79fdf040b3fd1433310cb6031e8a0044db6200df90e935ffb47c96dede5e8052e9e7f6732b570692da390e69469934568ebcf4c5aa31c48884ea5dfca7b561bfb88eafa7d1721c0fa2f1322155f0529b4d01b84c75b9b2704b98caa7b06639b96029ce7a17421123b0772a6f400a8b3b05991d94b3a03b0b8e6d7ed6985ba67d7ced3c72869c2a86741b5a44bc4057a6c2f875d8345147e3b872194dd67228884d946b07af028865883404547a834b343d3fa1c4252324ae75b0056dc6c99426e7e36ad0356d250a5883e97fd621713530b7d0721c4b8bc59ece116bf1be740787963a43eb78ae44012e1f3f115bf5eeed0a5c464d057dbd0890238c270554fc780358b2808c88004e784b31a326eb85df97a0d70401379f1c022c5a355eb467f35a1e8876ab4c66b980dbc8ffd1367d578f2949a14edf64fc3379913d5db1f1504c43cf4c90075c8d5b64b94552ba93d5dbfa3848e510192397ca7cc664a36e93c7c3cb8aba1e6572a9261b788858832a5be899b1943150cf27e54e2bbf7d5b8e0171285dd9a261777d56b2841583e3ece0003f8a6c797ec0d09098a537611b027ae031f6943902e9c986533d1f5a9123fac4796d85cf6b7dc5c7bde9f0b8413fee9680db7c6392fd3d7b04fff988c3be3331de80a4a7d4771a01c97306dc08fe68e7ac573c92bc0a7688f541381cd343ba4d8260295dd93504640", rtp_packet);
//	ctx.size = 768;
//	ctx.packet = rtp_packet;
//	rtp_payload_decode_input(ctx.decoder, ctx.packet, ctx.size);
//	rtp_payload_decode_destroy(ctx.decoder);
//	/*to use with h265
//	ctx.encoding = "H265"*/
//}