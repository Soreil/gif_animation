#pragma once
#include <vector>
#include <algorithm>
#include <numeric>
#include <optional>
#include <map>
#include <bitset>
#include <cstddef>
#include <variant>
#include <tuple>
#include "image.h"

using std::byte;

//https://www.w3.org/Graphics/GIF/spec-gif89a.txt implementation
namespace gif {
	class header {
	public:
		std::vector<byte> const  signature = { byte('G'),byte('I'),byte('F'),byte('8'),byte('9'),byte('a') };
	};

	class screenDescriptor {
	private:
		uint16_t width = 0;
		uint16_t height = 0;
		bool hasGCT = true; //1 bit
		std::bitset<3> bitsPerChannel = 7; //3 bit
		bool sortedGCT = false;
		std::bitset<3> GCTsize = 7; //3 bit,+1 is added so we get 8 for max bit depth;
		byte backgroundColorIndex = byte(0);
		byte pixelAspectRatio = byte(0);

	public:
		screenDescriptor(uint16_t wide, uint16_t high, bool useGCT = true) :width(wide), height(high), hasGCT(useGCT) {};
		screenDescriptor() = default;

		bool useGCT() {
			return hasGCT;
		}

		auto write() -> std::vector<byte> {
			std::vector<byte>out(7); //Fixed size required by spec
			out[0] = byte(((width >> 0) & 0xff));
			out[1] = byte(((width >> 8) & 0xff));

			out[2] = byte(((height >> 0) & 0xff));
			out[3] = byte(((height >> 8) & 0xff));

			std::bitset<8> bitfield;
			bitfield.set(7, hasGCT);

			bitfield.set(6, bitsPerChannel[2]);
			bitfield.set(5, bitsPerChannel[1]);
			bitfield.set(4, bitsPerChannel[0]);

			bitfield.set(3, sortedGCT);

			bitfield.set(2, GCTsize[2]);
			bitfield.set(1, GCTsize[1]);
			bitfield.set(0, GCTsize[0]);

			out[4] = byte(bitfield.to_ulong());

			out[5] = backgroundColorIndex;
			out[6] = pixelAspectRatio;

			return out;
		}
	};

	class colorTable {
	public:
		std::vector<RGBpixel> const table;
		colorTable(std::vector<RGBpixel> const& t) : table(t) {}

		auto bitsNeeded() -> size_t {
			for (int i = sizeof(size_t) * 8; i > 2; i--) {
				if ((table.size() >> i) == 1) {
					return i;
				}
			}
			return 2;
		}
	};

	class applicationExtensionLoop {
	private:
		byte extensionLabel = byte(0x21);
		byte appExtensionLabel = byte(0xff);

		byte blockSize = byte(0x0b);

		std::vector<byte> appIdentifier = { byte('N'), byte('E'), byte('T'), byte('S'), byte('C'), byte('A'), byte('P'), byte('E'), };
		std::vector<byte> appAuthentication = { byte('2'), byte('.'), byte('0'), };

		byte subBlockDataSize = byte(0x03);
		byte subBlockID = byte(0x01);
		uint16_t loopCount = 0; //infinite
		byte blockTerminator = byte(0x0);

	public:
		auto write() -> std::vector<byte> {
			std::vector<byte> out;

			out.emplace_back(extensionLabel);
			out.emplace_back(appExtensionLabel);
			out.emplace_back(blockSize);

			std::copy(appIdentifier.begin(), appIdentifier.end(), std::back_inserter(out));
			std::copy(appAuthentication.begin(), appAuthentication.end(), std::back_inserter(out));

			out.emplace_back(subBlockDataSize);
			out.emplace_back(subBlockID);
			out.emplace_back(byte((loopCount >> 0) & 0xff));
			out.emplace_back(byte((loopCount >> 8) & 0xff));
			out.emplace_back(blockTerminator);
			return out;
		}
	};

	class imageDescriptor {
	private:
		std::byte const seperator = std::byte{ 0x2c };
		uint16_t left = 0;
		uint16_t top = 0;
		uint16_t width = 0;
		uint16_t height = 0;
		bool hasLocalColor = false;
		bool isInterlaced = false;
		bool isSorted = false;
		std::bitset<2> reserved;
		std::bitset<3> localColorSize = 0;

	public:
		imageDescriptor(uint16_t width, uint16_t height, bool hasLocalColor = false) :width(width), height(height), hasLocalColor(hasLocalColor) {}

		auto write() const -> std::vector<byte> {
			std::vector<byte> out(10);
			out[0] = seperator;

			out[1] = byte(((left >> 0) & 0xff));
			out[2] = byte(((left >> 8) & 0xff));

			out[3] = byte(((top >> 0) & 0xff));
			out[4] = byte(((top >> 8) & 0xff));

			out[5] = byte(((width >> 0) & 0xff));
			out[6] = byte(((width >> 8) & 0xff));

			out[7] = byte(((height >> 0) & 0xff));
			out[8] = byte(((height >> 8) & 0xff));

			std::bitset<8> bitfield;

			bitfield[7] = hasLocalColor;
			bitfield[6] = isInterlaced;
			bitfield[5] = isSorted;

			bitfield[4] = reserved[1];
			bitfield[3] = reserved[0];

			bitfield[2] = localColorSize[2];
			bitfield[1] = localColorSize[1];
			bitfield[0] = localColorSize[0];

			out[9] = byte(bitfield.to_ulong());

			return out;
		}
	};

	class trailer {
	public:
		std::byte const trail = std::byte{ 0x3b };
	};

	using lzw_code = std::tuple<
		std::vector<std::bitset<3>>,
		std::vector<std::bitset<4>>,
		std::vector<std::bitset<5>>,
		std::vector<std::bitset<6>>,
		std::vector<std::bitset<7>>,
		std::vector<std::bitset<8>>,
		std::vector<std::bitset<9>>,
		std::vector<std::bitset<10>>,
		std::vector<std::bitset<11>>,
		std::vector<std::bitset<12>>
	>;

	template<std::size_t const n>
	auto toBitset(std::vector<uint16_t> const& in) -> std::vector<std::bitset<n>> {
		auto buffer = std::vector<std::bitset<n>>(in.size());
		for (size_t i = 0; i < buffer.size(); i++) {
			buffer[i] = in[i];
		}
		return buffer;
	}

	auto mapPixels(std::vector<RGBpixel> const& p, colorTable const& m) -> std::vector<byte> {
		auto distance = [](RGBpixel const& lhs, RGBpixel const& rhs) -> auto {
			return ((rhs.r - lhs.r) * (rhs.r - lhs.r)) +
				((rhs.g - lhs.g) * (rhs.g - lhs.g)) +
				((rhs.b - lhs.b) * (rhs.b - lhs.b));
		};

		std::vector<byte> out(p.size());

		for (size_t i = 0; i < p.size(); i++) {
			int smallest = INT_MAX;
			size_t pos = 0;

			for (size_t j = 0; j < m.table.size(); j++) {
				if (auto d = distance(p[i], m.table[j]); d < smallest) {
					smallest = d;
					pos = j;
				}
			}
			out[i] = std::byte(pos);
		}
		return out;
	}


	class encoder {
	private:
		header signature;
		screenDescriptor screen;
		std::optional<colorTable> GCT;
		std::optional<applicationExtensionLoop> loop;
		std::vector<std::tuple<
			imageDescriptor,
			std::optional<colorTable>,
			std::vector<RGBpixel>>
			>descriptors;

		trailer end;

	public:
		//TODO: imagedescriptor, image data, support for multiple images in constructor
		encoder(uint16_t width, uint16_t height, std::vector<RGBpixel> const& pixels) : screen(width, height),
			descriptors{ std::tuple{imageDescriptor(width,height),std::nullopt, pixels} },
			GCT(colorTable(palletize(pixels))) {};

		encoder(uint16_t width, uint16_t height, std::vector<std::vector<RGBpixel>> const& pixels, bool looping = true) : screen(width, height),
			GCT(colorTable(palletize(pixels[0]))) {
			if (looping)
				loop = applicationExtensionLoop();
			for (size_t i = 0; i < pixels.size(); i++) {
				descriptors.emplace_back(std::tuple{ imageDescriptor(width,height),std::nullopt,pixels[i] });
			}
		};

		encoder() = default;

		//This function returns a std::bitset<N> by design where N is the amount of bits needed to store colortable+clearcode+stopcode+generated codes
		//bitset size can't be determined just based on the function input since it depends on the pixels how many codes are generated in the table
		//a lower bound can be determined though based on the size of the colortable
		auto lzw_encode(std::vector<byte> const& in, size_t const colorTableBits) -> std::vector<uint16_t> {
			auto const lzw_code_size = colorTableBits; //number of color bits??
			uint16_t const clearCode = size_t(1) << lzw_code_size;
			uint16_t const end_of_info = clearCode + 1;

			uint16_t compressionID = clearCode + 1;
			//uint16_t codeBits = lzw_code_size + 1;

			std::vector<std::vector<byte>> table(0x1000);

			//initialize with our palette we should really clean this up, it's close to overflow
			for (auto i = size_t(0); i < (size_t(1) << colorTableBits); i++) {
				table[uint16_t(i)] = { byte(i) };
			}

			table[clearCode] = std::vector{ byte((clearCode & 0xf00) >> 8),byte(clearCode & 0xff) }; //This doesn't find it a byte
			table[end_of_info] = std::vector{ byte((end_of_info & 0xf00) >> 8),byte(end_of_info & 0xff) }; //this doesn't fit in a byte

			std::vector<uint16_t> out;

			out.push_back(clearCode);

			//Push first pixel right on to output
			out.push_back(uint16_t(in[0]));

			//Second pixel is always added to the map
			//table[++compressionID] = std::vector{ in[0],in[1] };

			//initial state from which the algorithm can operate
			std::vector<byte> hash{ in[1] };
			size_t currentKey = 0;

			auto earlyResult = std::find_if(table.begin(), table.end(), [hash](auto const& kv) -> bool {
				return kv == hash;
				});

			//this has to always succeed or the input data has more range than the colortable
			if (earlyResult != table.end()) {
				currentKey = std::distance(table.begin(), earlyResult);
			}

			//Always add first string to table
			table[++compressionID] = std::vector{ in[0],in[1] };

			//For some reason currentkey starts updating at some point for some inputs
			for (size_t i = 2; i < in.size(); i++) {

				hash.emplace_back(in[i]);

				auto result = std::find_if(table.begin(), table.end(), [hash](auto const& kv) -> bool {
					return kv == hash;
					});

				if (result != table.end()) {
					currentKey = std::distance(table.begin(), result);
				}
				else {
					out.push_back(uint16_t(currentKey));
					if (compressionID >= 0xFFF) {
						throw std::exception("CompressionID grew bigger than 12 bit in size");
					}
					else {
						table[++compressionID] = hash;
					}

					//Initialize local string with suffix of old string
					hash = std::vector{ hash.back() };
					auto at = std::find_if(table.begin(), table.end(), [hash](auto const& kv) -> bool {
						return kv == hash;
						});

					if (at != table.end()) {
						currentKey = std::distance(table.begin(), at);
					}
					else {
						throw std::exception("Impossible to hit this");
					}
				}

			}
			//End of pixels, final key
			out.push_back(uint16_t(currentKey));
			out.push_back(end_of_info);

			return out;
		}

		auto encode(std::vector<byte> const& in, size_t const colorTableBits) -> std::vector<byte> {
			auto enc = lzw_encode(in, colorTableBits);

			auto index = std::vector<std::vector<uint16_t>::iterator >();

			for (size_t i = colorTableBits; i < 12; i++) {
				if (auto found = std::find_if(enc.begin(), enc.end(), [i](auto const& cur) -> bool {
					return (cur & (1 << i)) != 0;
					}); found != enc.end()) {
					index.emplace_back(found);
				}
			}
			index.emplace_back(enc.end());

			std::vector<std::vector<uint16_t>> blocks;

			for (size_t i = 0; i < index.size() - 1; i++) {
				blocks.emplace_back(std::vector(index[i], index[i + 1]));
			}

			std::vector<byte> out;

			size_t bitsUsed = 0;
			for (size_t i = 0; i < blocks.size(); i++) {
				size_t bitsPerCode = colorTableBits + 1 + i;
				auto const& currentBlock = blocks[i];

				for (size_t b = 0; b < currentBlock.size(); b++) {
					auto bitsLeftIn = bitsPerCode;
					auto bitsLeftOut = 8 - (bitsUsed % 8); //BUG: This doesn't work

					if (bitsLeftOut == 8) {
						out.resize(out.size() + 1);
						bitsLeftOut = 8;
					}

					out.back() |= byte(currentBlock[b] << (8 - bitsLeftOut));
					auto bitsWritten = std::min(bitsLeftOut, bitsLeftIn);
					bitsUsed += bitsWritten;

					bitsLeftIn -= bitsWritten;
					if (bitsLeftIn == 0) {
						continue;
					}

					out.resize(out.size() + 1);
					bitsLeftOut = 8;

					out.back() |= byte(currentBlock[b] >> (bitsPerCode - bitsLeftIn));
					bitsWritten = std::min(bitsLeftOut, bitsLeftIn);
					bitsUsed += bitsWritten;

					bitsLeftIn -= bitsWritten;
					if (bitsLeftIn == 0) {
						continue;
					}

					out.resize(out.size() + 1);
					bitsLeftOut = 8;

					out.back() |= byte(currentBlock[b] >> (bitsPerCode - bitsLeftIn));
					bitsWritten = std::min(bitsLeftOut, bitsLeftIn);
					bitsUsed += bitsWritten;

					bitsLeftIn -= bitsWritten;
					if (bitsLeftIn != 0) {
						throw std::exception("This should never happen");
					}
				}
			}

			return out;
		}

		auto write() -> std::optional<std::vector<byte>> {
			std::vector<byte> out;
			std::copy(signature.signature.begin(), signature.signature.end(), std::back_inserter(out));

			auto it = screen.write();
			std::copy(it.begin(), it.end(), std::back_inserter(out));

			colorTable* activeTable = nullptr;

			if (GCT) {
				activeTable = &GCT.value();
				for (auto const& p : activeTable->table) {
					auto bytes = p.write();
					std::copy(bytes.begin(), bytes.end(), std::back_inserter(out));
				}
			}
			if (loop) {
				auto bytes = loop.value().write();
				std::copy(bytes.begin(), bytes.end(), std::back_inserter(out));
			}

			for (auto& [desc, localTable, pixels] : descriptors) {
				{
					auto bytes = desc.write();
					std::copy(bytes.begin(), bytes.end(), std::back_inserter(out));
				}

				if (localTable) {
					activeTable = &localTable.value();
					for (auto const& p : activeTable->table) {
						auto bytes = p.write();
						std::copy(bytes.begin(), bytes.end(), std::back_inserter(out));
					}
				}

				//we need a table after all
				if (activeTable == nullptr)
					continue;

				auto const mapped = mapPixels(pixels, *activeTable);
				auto const bytes = encode(mapped, activeTable->bitsNeeded());

				out.emplace_back(byte(activeTable->bitsNeeded()));

				auto bytesLeft = bytes.size();
				auto amountOfBlocks = bytesLeft / 0xff;
				for (size_t block = 0; block < amountOfBlocks; block++) {
					out.emplace_back(byte(0xff)); //Block size
					std::copy(bytes.begin() + (block * 0xff), bytes.begin() + ((block + 1) * 0xff), std::back_inserter(out));
				}
				bytesLeft -= amountOfBlocks * 0xff;
				auto trailingBytes = bytesLeft % 0xff;
				out.emplace_back(byte(trailingBytes));

				std::copy(bytes.end() - trailingBytes, bytes.end(), std::back_inserter(out));
				out.emplace_back(byte(0)); //END of image block
			}
			out.emplace_back(end.trail);
			return out;
		}
	};

	template<std::size_t n>
	auto pack(std::vector<std::bitset<n>> const in) -> std::pair<std::vector<byte>, size_t> {
		if constexpr (n < 2 || n > 12) {
			throw std::exception("Bitset too small or large, error somewhere else?");
		}

		auto totalbits = in.size() * n;

		std::vector<bool> buffer(totalbits);

		size_t visitedbits = 0;
		for (auto const& bs : in) {
			for (size_t i = 0; i < bs.size(); i++) {
				buffer[visitedbits + i] = bs[i];
			}
			visitedbits += bs.size();
		}

		std::vector<byte> out;

		if (buffer.size() % 8 != 0)
			out.resize((buffer.size() / 8) + 1);
		else
			out.resize(buffer.size() / 8);

		for (size_t i = 0; i < buffer.size(); i++) {
			out[i / 8] |= byte(uint8_t(buffer[i]) << (i % 8));
		}

		return std::pair<std::vector<byte>, size_t>(out, n);
	}
}
