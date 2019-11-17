#pragma once
#include <cstdint>
#include <vector>
#include <algorithm>

namespace gif {

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

		//Currently this is writing big endian, gif wants little endian!
		auto write() const -> std::vector<std::byte> {
			std::vector<std::byte> out(sizeof(T) * 3);
			for (size_t i = 0; i < sizeof(T); i++) {
				out[i] = std::byte((r >> (i * 8)) & 0xff);
			}
			for (size_t i = 0; i < sizeof(T); i++) {
				out[sizeof(T) + i] = std::byte((g >> (i * 8)) & 0xff);
			}
			for (size_t i = 0; i < sizeof(T); i++) {
				out[(2 * sizeof(T)) + i] = std::byte((b >> (i * 8)) & 0xff);
			}
			return out;
		}
	};

	using RGBpixel = pixel<uint8_t>;
	using RGBpixel32 = pixel<uint32_t>;


	auto median_cut(std::vector<RGBpixel> const& pixels) -> std::pair<std::vector<RGBpixel>, std::vector<RGBpixel>> {
		if (pixels.size() == 0) {
			return {};
		}

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

	auto palletize(std::vector<RGBpixel> const& pixels, int bitDepth = 256) -> std::vector<RGBpixel> {
		if (bitDepth == 1) {
			if (pixels.size() != 0)
				return std::vector{ average(pixels) };
			else
				return { RGBpixel() };
		}
		auto res = median_cut(pixels);
		auto lhs = palletize(res.first, bitDepth / 2);
		auto rhs = palletize(res.second, bitDepth / 2);
		lhs.insert(lhs.end(), rhs.begin(), rhs.end());
		return lhs;
	}



}