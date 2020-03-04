#pragma once
#include <stdint.h>
#include <stddef.h>
#include <list>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstring>

struct fmp4_segment {
	double EXTINF;
	std::string name;
	std::vector<uint8_t> data;
	fmp4_segment(double dur, const char* name, const void* dat, size_t bytes) : EXTINF(dur), name(name) {
		data.reserve(bytes);
		memcpy(&data[0], dat, bytes);
	}
};

class hls_m3u_t
{
	int version; //m3u8 version 
	int64_t seq = 0; //m3u8 sequence number (base 0)
	int64_t duration = 10; //target duration
	size_t count = 0; //count of segments
	size_t capacity; //max number of segments in playlist
	size_t remaining; //number of remaining segments in new playlist
	std::string url;
	std::list<fmp4_segment> segment_list;
	std::unordered_map<std::string, std::list<fmp4_segment>::iterator> segment_map;
	std::vector<uint8_t> initFile;
	std::string initFilename; // file that required to parse all segments (x-map)
	bool endlist = false; // ext-x-endlist set
public:
	hls_m3u_t() { }
	hls_m3u_t(int version, int64_t seq, int64_t duration, size_t capacity, size_t remaining, std::string url) : version(version), seq(seq), duration(duration), capacity(capacity), remaining(remaining), url(url) { }
	void set_x_map(std::string initFilename, void *data, size_t bytes);
	void set_x_endlist();
	int delete_segment(int number_of_segments); // deletes n segments from front
	int add_segment(double duration, const char uri[], const void* data, size_t bytes); // adds segment to back
	int set_playlist_capacity(int number_of_segments, int remaining);
	std::string playlist_data();
	std::vector<uint8_t> get_segment(std::string name);
	std::vector<uint8_t> get_init_file();
};





