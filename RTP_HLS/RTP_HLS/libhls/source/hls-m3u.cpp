#include "../include/hls-m3u.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

int hls_m3u_t::add_segment(int duration, const char uri[]) {
	if (count < capacity)
	{
		std::pair<int, std::string> segment = std::make_pair(duration, uri);
		segment_list.push_front(segment);
		count++;
		return 0;
	}
	else 
		delete_segment(count - remaining);
}

void hls_m3u_t::set_x_map(std::string initFilename)
{
	this->initFilename = initFilename;
}

void hls_m3u_t::set_x_endlist()
{
	endlist = true;
}

int hls_m3u_t::playlist_write() {
	fd = fopen(filename.c_str(), "wb");
	if (fd != NULL) {
		std::stringstream ssplaylist;
		ssplaylist << "#EXTM3U\n" << "#EXT-X-VERSION:" << version << '\n' << "#EXT-X-TARGETDURATION:" << duration << '\n' << "#EXT-X-MEDIA-SEQUENCE:" << seq << '\n' << "#EXT-X-PLAYLIST-TYPE:EVENT\n" << "#EXT-X-MAP:URI=\"" << initFilename.c_str() << '\"';
		for (std::pair<int, std::string> segment : segment_list)
				ssplaylist << "#EXTINF:" << segment.first << '\n' << segment.second.c_str() << '\n';
		ssplaylist << (endlist) ? "#EXT-X-ENDLIST\n" : "";
		size_t nbytes = fwrite(ssplaylist.str().c_str(), 1, ssplaylist.str().length(), fd);
		return nbytes;
	}
	else
		return errno;
}

int hls_m3u_t::delete_segment(int number_of_segments) {
	for (int i = 0; i < number_of_segments; ++i) {
		segment_list.pop_back();
	}
	seq += number_of_segments;
	count -= number_of_segments;
	return number_of_segments;
}

std::string hls_m3u_t::playlist_request() {
	playlist_write();
	return filename;
}