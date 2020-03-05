#include "../include/hls-m3u.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

int hls_m3u_t::add_segment(double duration, const char uri[], const void* data, size_t bytes) {
	if (count < capacity)
	{
		fmp4_segment segment(duration, uri, data, bytes);
		segment_list.push_back(segment);
		segment_map[uri] = segment_list.end();
		count++;
		return 0;
	}
	else {
		delete_segment(count - remaining);
		return 1;
	}
}

int hls_m3u_t::set_playlist_capacity(int number_of_segments, int remaining)
{
		if (count > number_of_segments) 
			delete_segment(count - number_of_segments);
	capacity = number_of_segments;
	this->remaining = remaining;
	return 0;
}

hls_m3u_t::~hls_m3u_t()
{
	delete_segment(count);
}

void hls_m3u_t::set_x_map(std::string initFilename, void* data, size_t bytes)
{
	this->initFilename = initFilename;
	initFile.reserve(bytes);
	memcpy(&initFile[0], data, bytes);
}

void hls_m3u_t::set_x_endlist()
{
	endlist = true;
}

std::string hls_m3u_t::get_url()
{
	return url;
}

int hls_m3u_t::get_capacity()
{
	return capacity;
}

int hls_m3u_t::get_remaining()
{
	return remaining;
}

int hls_m3u_t::delete_segment(int number_of_segments) {
	for (int i = 0; i < number_of_segments; ++i) {
		segment_map.erase(segment_list.front().name);
		segment_list.pop_front();
	}
	seq += number_of_segments;
	count -= number_of_segments;
	return number_of_segments;
}

std::string hls_m3u_t::playlist_data()
{
	std::stringstream ssplaylist;
	ssplaylist << "#EXTM3U\n" << "#EXT-X-VERSION:" << version << '\n' << "#EXT-X-TARGETDURATION:" << duration << '\n' << "#EXT-X-MEDIA-SEQUENCE:" << seq << '\n' << "#EXT-X-PLAYLIST-TYPE:EVENT\n" << "#EXT-X-MAP:URI=\"" << url.c_str() << initFilename.c_str() << "\"\n";
	for (fmp4_segment segment : segment_list)
		ssplaylist << "#EXTINF:" << segment.EXTINF << '\n' << url.c_str() << segment.name.c_str() << '\n';
	ssplaylist << (endlist) ? "#EXT-X-ENDLIST\n" : "";
	return ssplaylist.str();
}

std::vector<uint8_t> hls_m3u_t::get_segment(std::string name)
{
	return (segment_map.find(name) != segment_map.end()) ? segment_map[name]->data : std::vector<uint8_t>();
}

std::vector<uint8_t> hls_m3u_t::get_init_file()
{
	return initFile;
}
