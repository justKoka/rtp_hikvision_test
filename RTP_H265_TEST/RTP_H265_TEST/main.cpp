#include <cstdio>
#include <iostream>
#include <cstring>
#include "rtp-payload-internal.h"
#include <assert.h>

struct rtp_payload_test_t
{
	int payload;
	const char* encoding;
	void* encoder;
	void* decoder;
	size_t size;
	uint8_t *packet;
	/*uint8_t packet[64 * 1024];*/
};

static void* rtp_alloc(void* /*param*/, int bytes)
{
	static uint8_t buffer[2 * 1024 * 1024 + 4] = { 0, 0, 0, 1, };
	assert(bytes <= sizeof(buffer) - 4);
	return buffer + 4;
}

static void rtp_free(void* /*param*/, void * /*packet*/)
{
}

static int convertCharStreamToHex(const char[], uint8_t*);

static void rtp_decode_packet(void* param, const void *packet, int bytes, uint32_t timestamp, int flags)
{
	static const uint8_t start_code[4] = { 0, 0, 0, 1 };
	struct rtp_payload_test_t* ctx = (struct rtp_payload_test_t*)param;

	static uint8_t buffer[2 * 1024 * 1024];
	assert(bytes + 4 < sizeof(buffer));
	assert(0 == flags);

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
	//rtp packet decoded
	//now able to send decoded rtp payload to livestream
	//write(buffer,file)
}

int main()
{
	struct rtp_payload_test_t ctx;
	ctx.payload = 96;
	ctx.encoding = "h265";
	struct rtp_payload_t handler;
	handler.alloc = rtp_alloc;
	handler.free = rtp_free;
	handler.packet = rtp_decode_packet;
	ctx.decoder = rtp_payload_decode_create(ctx.payload, ctx.encoding, &handler, &ctx);
	uint8_t rtp_packet[1500];
	convertCharStreamToHex("a060bdc3567f1a3e3d73bfb540010c01ffff016000000300b0000003000003005dbc0901", rtp_packet);
	ctx.size = 36;
	ctx.packet = rtp_packet;
	rtp_payload_decode_input(ctx.decoder, ctx.packet, ctx.size);
}

static int convertCharStreamToHex(const char input[], uint8_t *output) {
	int j = 0;
	for (int i = 0; i < strlen(input); i += 2) {
		output[j] = (isalpha(input[i]) ? input[i] - 87 : input[i] - 48) * 16 + (isalpha(input[i + 1]) ? input[i + 1] - 87 : input[i + 1] - 48);
		j++;
	}
	return 1;
}

//int main() {
//	const char hexStream[] = "a0e0fe322cdc29340cdef5c821e7b007de7f005c510ed581768367c3e3b3100483dbd74f65719a10c702fbdfb747cb7b1cac63038d89ac59d7fc39cbccce950c8eb6249a41eebd4ec722cbb2d849fd1fc28061f7730d9d500eb00c361aa446b1497d37b48887cd892b19c0cb40319b5c2631c87b30d37ea2b30196eef0f66abcc851a9add3a236722b9c87231ad6ed3632052eb6f6897e49ac1a537a7e3ce4d7be0ad103c51eb564783c971937235364b8947de1783abe8e8b80f3f37a2ecc6548d6b42cc7211203e1784bb7c468000a14beef1e7871019f03444db8376723fd889d4e01c27ccb6abf45eddebf9a76daa33737329810d2cdd0e45f15f41645b872bf5c75533232791100b53de604fd7ae7efac423e5d17cf96e4322ea8a3324ea29df9d30f91076d281c6de08876ec83d6a4f947f7b1ef239edff4ffc5201d52a5e1644a824f17f24c5423b330d80c56f5e7f23d08d002c4648b9758784e7b958a7586cdab972249691e64e714cdd4e23a1aad28844567e2c682eb2be82b37988b1fc320d612dad09a8e249e8048a36eddaa2063e62db33bebe6dce0a18cdca35c1f4b75c96aa8530f7d8e381e984b46921c6ee6f2bf27a4d53d7897a092128510301f2e592853ee61291ff97ea5df5ad7f3e681ec0185b4536c8d6ed5a57eae889c0a9c07d175cae98d54ec281b9ada1bc901902bbda439a60053581a48489361af4d8cc0e788b4f700a7dd0ab60484294e3eb92c8cfa485880f9a5b03ec26d137991e658a8b4910fb7616728e792442fe6731f7821785430147d9e378b105955c0491874001a076dc37ec97869e558d848ce9ece98ae729b205f6544c64615217b3b2612f4888f638dfdcde2ef6bff32ddfffa8b23011f2fc4ce65a769b83e704837f9dc12a2117b81ea1b44208ae58b671b7e196adc75399b96221b4a7de20dedfc74896ef36e5dacd5db305f46516c6257d7d859c9d0c703ab9e060d9c65c74ebc4a46ca3e7c95f1b550978b68a535cdf69e32893ab8af3dd491e0bdbfbf682dc8b4b5744d8104a45c7ff87a5eadf9ea5232a0798b102efb2b4255e55eaa37e453052b189002c78afa41b577a4e04a7d65d6a587c9845a0bc3fffb7a8f8a871f3fcd85dab6b34a5376bd2947575c92bfb975267998fe4c3e2fe404ce4580a62ea027f436ea5d8248950de0ebdf7a35f997afe0b4008ba6a2b59b54a290c451f92d7c3572376273369695a97b8b864d2e488b0a8bcb63736df7b70f86b5fa80510fee085d67c98f6644d8ad1ee729bc479b816d100002";
//	char *hexArray = new char[strlen(hexStream)*3];
//	int j = 0;
//	for (int i = 0; i < strlen(hexStream); i += 2)
//	{
//		hexArray[j] = '0';
//		hexArray[j + 1] = 'x';
//		hexArray[j + 2] = hexStream[i];
//		hexArray[j + 3] = hexStream[i + 1];
//		hexArray[j + 4] = ',';
//		j += 5;
//	}
//	printf("hello!");
//	return 0;
//}