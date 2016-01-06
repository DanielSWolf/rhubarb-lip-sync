#pragma once

#include <fstream>

namespace little_endian {

	template <typename Type, int bitsToRead = 8 * sizeof(Type)>
	Type read(std::istream &stream) {
		static_assert(bitsToRead % 8 == 0, "Cannot read fractional bytes.");
		static_assert(bitsToRead <= sizeof(Type) * 8, "Bits to read exceed target type size.");

		Type result = 0;
		char *p = reinterpret_cast<char*>(&result);
		int bytesToRead = bitsToRead / 8;
		for (int byteIndex = 0; byteIndex < bytesToRead; byteIndex++) {
			*(p + byteIndex) = static_cast<char>(stream.get());
		}
		return result;
	}

	template <typename Type, int bitsToWrite = 8 * sizeof(Type)>
	void write(Type value, std::ostream &stream) {
		static_assert(bitsToWrite % 8 == 0, "Cannot write fractional bytes.");
		static_assert(bitsToWrite <= sizeof(Type) * 8, "Bits to write exceed target type size.");

		char *p = reinterpret_cast<char*>(&value);
		int bytesToWrite = bitsToWrite / 8;
		for (int byteIndex = 0; byteIndex < bytesToWrite; byteIndex++) {
			stream.put(*(p + byteIndex));
		}
	}

	constexpr uint32_t fourcc(unsigned char c0, unsigned char c1, unsigned char c2, unsigned char c3) {
		return c0 | (c1 << 8) | (c2 << 16) | (c3 << 24);
	}

	inline std::string fourccToString(uint32_t fourcc) {
		return std::string(reinterpret_cast<char*>(&fourcc), 4);
	}

}
