#include "../include/hls-m3u.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>

int playlist_init(hls_m3u_t* m3u, const char filename[]) {
	m3u->filename = filename;
	m3u->fd = fopen(m3u->filename.c_str(), "wb");
	if (m3u->fd != NULL)
		return 0;
	else
		return errno;
}

int add_segment(hls_m3u_t* m3u, int duration, const char uri[]) {
	if (m3u->count < m3u->capacity)
	{
		std::pair<int, std::string> segment = std::make_pair(duration, uri);
		m3u->segment_list.push_front(segment);
		m3u->count++;
		return 0;
	}
	else 
		delete_segment(m3u, m3u->count - m3u->remaining);
}

int playlist_write(hls_m3u_t* m3u) {
	m3u->fd = fopen(m3u->filename.c_str(), "wb");
	if (m3u->fd != NULL) {
		std::stringstream ssplaylist;
		ssplaylist << "#EXTM3U\n" << "#EXT-X-VERSION:" << m3u->version << '\n' << "#EXT-X-TARGETDURATION:" << m3u->duration << '\n' << "#EXT-X-MEDIA-SEQUENCE:" << m3u->seq << '\n' << "#EXT-X-PLAYLIST-TYPE:EVENT\n";
		for (std::pair<int, std::string> segment : m3u->segment_list)
				ssplaylist << "#EXTINF:" << segment.first << '\n' << segment.second.c_str() << '\n';
		size_t nbytes = fwrite(ssplaylist.str().c_str(), 1, ssplaylist.str().length(), m3u->fd);
		return nbytes;
	}
	else
		return errno;
}

int delete_segment(hls_m3u_t* m3u, int number_of_segments) {
	for (int i = 0; i < number_of_segments; ++i) {
		m3u->segment_list.pop_back();
	}
	m3u->seq += number_of_segments;
	m3u->count -= number_of_segments;
	return number_of_segments;
}

std::string playlist_request(hls_m3u_t* m3u) {
	playlist_write(m3u);
	return m3u->filename;
}