#include "pcap.h"

#include <string.h>

using namespace utils;

#pragma pack(push,1)
typedef struct pcap_hdr_s {
	uint32_t magic_number;   /* magic number */
	uint16_t version_major;  /* major version number */
	uint16_t version_minor;  /* minor version number */
	int32_t  thiszone;       /* GMT to local correction */
	uint32_t sigfigs;        /* accuracy of timestamps */
	uint32_t snaplen;        /* max length of captured packets, in octets */
	uint32_t network;        /* data link type */
} pcap_hdr_t;

typedef struct pcaprec_hdr_s {
	uint32_t ts_sec;         /* timestamp seconds */
	uint32_t ts_usec;        /* timestamp microseconds */
	uint32_t incl_len;       /* number of octets of packet saved in file */
	uint32_t orig_len;       /* actual length of packet */
	uint8_t	packet[0];
} pcaprec_hdr_t;
#pragma pack(pop)

inline bool _header(FILE* f, pcap_hdr_t* buffer) {
	size_t readed = fread(buffer, 1, sizeof(pcap_hdr_t), f);
	return readed == sizeof(pcap_hdr_t);
}
inline bool _packet(FILE* f, pcaprec_hdr_t* buffer) {
	size_t readed = fread(buffer, 1, sizeof(pcaprec_hdr_t), f);
	if (readed && readed == sizeof(pcaprec_hdr_t)) {
		return fread(&buffer->packet, 1, buffer->incl_len, f) == buffer->incl_len;
	}
	return false;
}

template <typename T> T* cast(uint8_t *buffer) { return (T*)buffer; }

pcap_filereader::pcap_filereader(const char* file_pathname) : hFile(nullptr)
{
	if ((hFile = fopen(file_pathname, "rb")) == nullptr) {
		printf("[WARNING] error open file `%s` (%s:%d)\n", file_pathname, strerror(errno), errno);
	}
}


pcap_filereader::~pcap_filereader()
{
	if (hFile) {
		fclose(hFile);
		hFile = nullptr;
	}
}

bool pcap_filereader::_next() {
	return hFile != nullptr && _packet(hFile, cast<pcaprec_hdr_t>(uBuffer));
}

pcap_filereader::iterator& pcap_filereader::iterator::operator++ () {
	if (reader->_next()) {
		++nPacket;
	}
	else {
		nPacket = -1LL;
	}
	return *this;
}

pcap_filereader::packet pcap_filereader::iterator::operator* () const {
	auto p = cast<pcaprec_hdr_t>(reader->uBuffer);
	return pcap_filereader::packet(nPacket, p->packet, p->orig_len);
}

pcap_filereader::packet pcap_filereader::iterator::operator -> () const {
	auto p = cast<pcaprec_hdr_t>(reader->uBuffer);
	return pcap_filereader::packet(nPacket, p->packet, p->orig_len);
}

pcap_filereader::iterator pcap_filereader::at(size_t nPacket) {
	return iterator(*this, 0);
}
pcap_filereader::iterator pcap_filereader::begin() {

	if (hFile != nullptr) {
		auto pcap_header = cast<pcap_hdr_t>(uBuffer);
		auto swapped = false;
		fseek(hFile, 0, SEEK_SET);

		if (_header(hFile, pcap_header) && (pcap_header->magic_number == 0xa1b2c3d4 || (swapped = (pcap_header->magic_number == 0xd4c3b2a1))) && _next()) {
			return iterator(*this, 0);
		}
	}
	return iterator(*this, -1LL);
}
pcap_filereader::iterator pcap_filereader::end() {
	return iterator(*this, -1LL);
}