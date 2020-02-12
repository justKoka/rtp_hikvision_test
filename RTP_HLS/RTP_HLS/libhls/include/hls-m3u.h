#pragma once
#include <stdint.h>
#include <stddef.h>
#include <list>
#include <string>

struct hls_m3u_t
{
	int version; //m3u8 version 
	int64_t seq = 0; //m3u8 sequence number (base 0)
	int64_t duration = 10; //target duration
	size_t count = 0; //count of segments
	size_t capacity; //max number of segments in playlist
	size_t remaining; //number of remaining segments in new playlist
	std::list<std::pair<int, std::string>> segment_list;
	std::string filename;
	FILE* fd;
	hls_m3u_t() { }
	hls_m3u_t(int version, int64_t seq, int64_t duration, size_t capacity, size_t remaining, std::string filename) : version(version), seq(seq), duration(duration), filename(filename) { }
};

int playlist_write(hls_m3u_t* m3u);
int delete_segment(hls_m3u_t* m3u, int number_of_segments); // deletes n segments from front, rewrites playlist
int add_segment(hls_m3u_t* m3u, int duration, const char uri[]); // adds segment to back, writes segment to playlist
std::string playlist_request(hls_m3u_t* m3u);



