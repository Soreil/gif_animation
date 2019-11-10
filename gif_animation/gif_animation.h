#pragma once
#include <vector>
#include <algorithm>
#include <numeric>
#include <optional>
#include <map>
#include <bitset>
#include <cstddef>

using std::byte;

//https://www.w3.org/Graphics/GIF/spec-gif89a.txt implementation
namespace gif {
	class header {
		std::vector<byte> const  signature = { byte('G'),byte('I'),byte('F'),byte('8'),byte('9'),byte('a') };
	};

	class screenDescriptor {
		uint16_t width = 0;
		uint16_t height = 0;
		bool hasColorMapping = true; //1 bit
		size_t bitsPerChannel = 8; //3 bit
		bool ColorMapIsSorted = false;
		size_t bitsPerPixel = 8; //3 bit,+1 is added so we get 8 for max bit depth;
		byte backgroundColorIndex = std::byte(0);
		byte pixelAspectRatio = std::byte(0);

	};

	template<typename T>
	struct pixel {
		T r = 0;
		T g = 0;
		T b = 0;

		auto operator==(pixel& rhs) -> bool {
			return r == rhs.r && g == rhs.g && b == rhs.b;
		}
		auto operator+(pixel<uint8_t> const& rhs) -> pixel {
			return pixel{ r + rhs.r,g + rhs.g,b + rhs.b };
		};

	};

	using RGBpixel = pixel<uint8_t>;
	using RGBpixel32 = pixel<uint32_t>;

	class colorTable {
	public:
		std::vector<RGBpixel> const table;
		colorTable(std::vector<RGBpixel>& t) : table(t) {}
	};

	class imageDescriptor {
		std::byte const seperator = std::byte{ 0x2c };
		uint16_t left = 0;
		uint16_t top = 0;
		uint16_t width = 0;
		uint16_t height = 0;
		bool hasLocalColor = false;
		bool isInterlaced = false;
		bool isSorted = false;
		size_t localColorSize = 0;
	};

	class trailer {
		std::byte const trail = std::byte{ 0x3b };
	};

	auto median_cut(std::vector<RGBpixel> const& pixels) -> std::pair<std::vector<RGBpixel>, std::vector<RGBpixel>> {

		auto bucket = pixels;
		auto Rplane = std::vector<uint8_t>(bucket.size());
		auto Gplane = std::vector<uint8_t>(bucket.size());
		auto Bplane = std::vector<uint8_t>(bucket.size());

		for (size_t i = 0; i < pixels.size(); i++) {
			Rplane[i] = bucket[i].r;
			Gplane[i] = bucket[i].g;
			Bplane[i] = bucket[i].b;
		}
		auto const Rbounds = std::minmax_element(Rplane.begin(), Rplane.end());
		auto const Gbounds = std::minmax_element(Gplane.begin(), Gplane.end());
		auto const Bbounds = std::minmax_element(Bplane.begin(), Bplane.end());

		auto const Rrange = *Rbounds.second - *Rbounds.first;
		auto const Grange = *Gbounds.second - *Gbounds.first;
		auto const Brange = *Bbounds.second - *Bbounds.first;

		auto const ranges = std::vector{ Rrange, Grange, Brange };

		auto const max = std::max_element(ranges.begin(), ranges.end());

		auto const greatest = std::clamp(std::distance(ranges.begin(), max), ptrdiff_t(0), ptrdiff_t(2));
		std::sort(bucket.begin(), bucket.end(), [greatest](RGBpixel& i, RGBpixel& j) -> bool {
			switch (greatest) {
			case 0:
				return i.r < j.r;
			case 1:
				return i.g < j.g;
			case 2:
				return i.b < j.b;
			default:
				return false; //Earth has exploded if we hit this
			}
			});

		auto lower = std::vector<RGBpixel>(bucket.begin(), bucket.begin() + (bucket.size() / 2));
		auto upper = std::vector<RGBpixel>(bucket.begin() + (bucket.size() / 2), bucket.end());
		return std::pair{ lower,upper };
	}

	auto average(std::vector<RGBpixel> const& pixels) -> RGBpixel {
		RGBpixel32 sum;
		for (auto const& p : pixels) {
			sum = sum + p;
		}

		return RGBpixel{ uint8_t(sum.r / pixels.size()),uint8_t(sum.g / pixels.size()),uint8_t(sum.b / pixels.size()) };
	}

	auto palletize(std::vector<RGBpixel> const& pixels, int bitDepth = 64) -> std::vector<RGBpixel> {
		if (bitDepth == 1) {
			return std::vector{ average(pixels) };
		}
		auto res = median_cut(pixels);
		auto lhs = palletize(res.first, bitDepth / 2);
		auto rhs = palletize(res.second, bitDepth / 2);
		lhs.insert(lhs.end(), rhs.begin(), rhs.end());
		return lhs;
	}

	class encoder {
	public:
		header signature;
		screenDescriptor screen;
		std::optional<colorTable> GCT;
		std::vector<std::tuple<
			imageDescriptor,
			std::optional<colorTable>,
			std::vector<RGBpixel>>
			>descriptors;

		trailer end;

		auto lzw_encode(std::vector<byte> const& in, size_t const colorTableBits) -> std::vector<uint16_t> {
			auto const lzw_code_size = colorTableBits; //number of color bits??
			uint16_t const clearCode = size_t(1) << lzw_code_size;
			uint16_t const end_of_info = clearCode + 1;

			uint16_t compressionID = clearCode + 2;
			//uint16_t codeBits = lzw_code_size + 1;

			std::map<uint16_t, std::vector<byte>> table;

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
			table[compressionID++] = std::vector{ in[0],in[1] };

			//initial state from which the algorithm can operate
			std::vector<byte> hash{ in[1] };
			uint16_t currentKey = 0;

			auto result = std::find_if(table.begin(), table.end(), [hash](auto const& kv) -> bool {
				return kv.second == hash;
				});

			//this has to always succeed or the input data has more range than the colortable
			if (result != table.end()) {
				currentKey = result->first;
			}


			for (size_t i = 2; i < in.size(); i++) {

				hash.emplace_back(in[i]);

				auto result = std::find_if(table.begin(), table.end(), [hash](auto const& kv) -> bool {
					return kv.second == hash;
					});

				if (result != table.end()) {
					currentKey = result->first;
				}
				else {
					out.push_back(currentKey);
					table[compressionID++] = hash;

					//Initialize local string with suffix of old string
					hash = std::vector{ hash.back() };

				}

			}
			//End of pixels, final key
			out.push_back(currentKey);
			out.push_back(end_of_info);


			//Instead of returning is uint16s we could pack right here.
			
			return out;
		}

	};

	template<std::size_t n>
	auto pack(std::vector<std::bitset<n>> const in) -> std::optional<std::vector<byte>> {
		if constexpr (n < 2 || n > 14) {
			return std::nullopt;
		}

		auto totalbits = in.size() * n;

		std::vector<bool> buffer(totalbits);

		auto visitedbits = 0;
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

		return out;
	}
}
