#include <cstdio>
#include <iostream>
#include <cstring>
#include "libhls/include/hls_segmenter_mp4.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
	const char spschar[] = "67428032da00800304c05a8101012000000f00000301c20c";
	uint8_t spshex[24];
	convertCharStreamToHex(spschar, spshex);
	hls_segmenter hls(10, 100, 6);
	hls.init_video_stream("H264", spshex, 24);
	return 0;
}