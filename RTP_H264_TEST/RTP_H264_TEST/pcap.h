#pragma once
#include <functional>
#include <cstdlib>
#include <cstdio>
#include <cinttypes>

//for (auto& p : rtp) {
//	p
//}
namespace utils {
	class pcap_filereader {
	public:
		class packet;
		class iterator {
		protected:
			friend class pcap_filereader;
			pcap_filereader*	reader;
			int64_t	nPacket;
			iterator(pcap_filereader& reader, int64_t no) : reader(&reader), nPacket(no) { ; }
		public:
			iterator(const iterator& it) {
				nPacket = it.nPacket;
				reader = it.reader;
			}
			iterator& operator = (const iterator& it) {
				nPacket = it.nPacket;
				reader = it.reader;
				return *this;
			}
			~iterator() { ; }
			bool operator != (const iterator& it) {
				return nPacket != it.nPacket;
			}
			iterator& operator++ ();
			packet operator* () const;
			packet operator -> () const;
		};

	protected:
		friend class iterator;
		FILE *	hFile;
		uint8_t	uBuffer[65536];
		bool	_next();
	public:
		class packet {
		protected:
			friend class iterator;
			packet(int64_t no, uint8_t* payload, size_t length) : no(no), payload(payload), length(length) { ; }
		public:
			int64_t no;
			uint8_t* payload;
			size_t length;
		};

		

		pcap_filereader(const char* file_pathname);
		~pcap_filereader();

		iterator at(size_t nPacket);
		iterator begin();
		iterator end();
	};
}