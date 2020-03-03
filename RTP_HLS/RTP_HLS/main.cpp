#include <cstdio>
#include <iostream>
#include <cstring>
#include "libhls/include/hls_segmenter_mp4.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pcap.h"

static int convertCharStreamToHex(const char input[], uint8_t *output) {
	int j = 0;
	for (int i = 0; i < strlen(input); i += 2) {
		output[j] = (isalpha(input[i]) ? input[i] - 87 : input[i] - 48) * 16 + (isalpha(input[i + 1]) ? input[i + 1] - 87 : input[i + 1] - 48);
		j++;
	}
	return 1;
}

int main()
{
	const char spschar[] = "67420032e2900400183602dc04040690789115";
	uint8_t spshex[19];
	convertCharStreamToHex(spschar, spshex);
	hls_segmenter hls(10, 100, 6, "http://127.0.0.1/");
	hls.init_video_stream("H264", 96, spshex, 19);
	hls.init_audio_stream("pcmu", 0, 1, 8, 8000);
	hls.write_init_segment("init.mp4"); // вызывать после инициализации всех стримов, т.к. пишется init.mp4 со всей метадатой стримов
	utils::pcap_filereader rtp("test_large.pcap");
	constexpr uint32_t udp_pos = 34;
	constexpr uint32_t udp_length_pos = 4;
	constexpr uint32_t udp_header_length = 8;
		for (auto&& p : rtp) {
			uint16_t* udp_length_ptr = (uint16_t*)(p.payload + udp_pos + udp_length_pos);
			uint16_t udp_length = ntohs(*udp_length_ptr);
			hls.send_rtp(p.payload + udp_pos + udp_header_length, udp_length - udp_header_length);
		}
	auto playlist = hls.get_playlist_data();
	return 0;
}