#include "CppUnitTest.h"
#include "../gif_animation/gif_animation.h"
#include <algorithm>
#include <bitset>
#include <optional>
#include <variant>
#include <fstream>
#include <filesystem>
#include <cmath>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace TestPalette
{
	TEST_CLASS(HSL)
	{
	private:
		template<typename T>
		struct hsv;

		template<typename T>
		struct rgb {
			T r;
			T g;
			T b;

			rgb(hsv<T> in) {
				auto C = in.v * in.s;
				auto Hprime = in.h / 60.0;
				auto X = C * (1 - std::abs(std::fmod(Hprime, 2.0) - 1.0));

				if (Hprime >= 0.0 && Hprime <= 1.0)
				{
					r = C;
					g = X;
					b = 0.0;
				}
				else if (Hprime >= 1.0 && Hprime <= 2.0)
				{
					r = X;
					g = C;
					b = 0.0;
				}
				else if (Hprime >= 2.0 && Hprime <= 3.0)
				{
					r = 0.0;
					g = C;
					b = X;

				}
				else if (Hprime >= 3.0 && Hprime <= 4.0)
				{
					r = 0.0;
					g = X;
					b = C;
				}
				else if (Hprime >= 4.0 && Hprime <= 5.0)
				{
					r = X;
					g = 0.0;
					b = C;
				}
				else if (Hprime >= 5.0 && Hprime <= 6.0)
				{
					r = C;
					g = 0.0;
					b = X;
				}
				else
				{
					r = 0;
					g = 0;
					b = 0;
				}

				auto m = in.v - C;
				r += m;
				g += m;
				b += m;
			}
			rgb(T rIn, T bIn, T cIn) : r(rIn), g(bIn), b(cIn) {};

			auto toPixelRGB() -> gif::RGBpixel {
				return gif::RGBpixel{ uint8_t(r * 255.0),uint8_t(g * 255.0),uint8_t(b * 255.0) };
			}
		};

		template<typename T>
		struct hsv {
			T h;
			T s;
			T v;

			hsv(rgb<T> in) {
				auto [min, max] = std::minmax({ in.r,in.g,in.b });
				if (max == min) h = 0;
				else if (max == in.r) h = 60.0 * ((in.g - in.b) / (max - min));
				else if (max == in.g) h = 60.0 * (2.0 + ((in.b - in.r) / (max - min)));
				else h = 60.0 * (4.0 + ((in.r - in.g) / (max - min))); //This actually matches b by design, it should be impossible to not hit one of the 4 cases.

				if (h < 0.0) h += 360.0;

				if (max == 0.0) s = 0.0;
				else s = (max - min) / max;

				v = max;
			}

			hsv(T hIn, T sIn, T vIn) : h(fmod(hIn, 360.0)), s(sIn), v(vIn) {};
			hsv() = default;
		};
	public:
		TEST_METHOD(RGBtoHSV) {
			auto red = rgb<double>{ 1,0,0 };
			auto hsv_red = hsv<double>(red);
			auto pink = rgb<double>{ 0.750,0.375,0.750 };
			auto hsv_pink = hsv<double>(pink);
			auto pink2 = rgb<double>(hsv_pink);
		};

		TEST_METHOD(HSVtoRGB) {
			auto red = rgb<double>{ 1,0,0 };
			auto hsv_red = hsv<double>(red);
			auto pink = rgb<double>{ 0.750,0.375,0.750 };
			auto hsv_pink = hsv<double>(pink);
		};

		TEST_METHOD(generateRainbow) {
			uint16_t width = 32;
			uint16_t height = 32;

			std::vector<hsv<double>> pixelsHSV(size_t(width * height));
			std::vector<gif::RGBpixel> outPixels(pixelsHSV.size());

			for (size_t i = 0; i < pixelsHSV.size(); i++) {
				pixelsHSV[i].h = (360.0*(double(i)/ double(pixelsHSV.size())));
				pixelsHSV[i].s = 1;
				pixelsHSV[i].v = 1;
			}

			for (size_t i = 0; i < outPixels.size(); i++) {
				auto tmp = rgb<double>(pixelsHSV[i]);
				outPixels[i] = tmp.toPixelRGB();
			}

			auto enc = gif::encoder(width, height, outPixels);
			auto hasImg = enc.write();

			if (hasImg) {
				auto img = hasImg.value();
				auto outFileDir = std::filesystem::path("buggedRainbow");
				std::filesystem::create_directory(outFileDir);

				int fileID = 0;
				while (std::filesystem::exists(outFileDir /
					std::filesystem::path(std::to_string(fileID) +
						"-" + std::to_string(width) +
						"x" + std::to_string(height) +
						".gif")))
					fileID++;

				auto outFile = std::ofstream(outFileDir /
					std::filesystem::path(std::to_string(fileID) +
						"-" + std::to_string(width) +
						"x" + std::to_string(height) +
						".gif"), std::ofstream::binary);

				outFile.write(reinterpret_cast<const char*>(img.data()), img.size());

			}
		}

		TEST_METHOD(generateRainbowAnimated) {
			std::vector<std::vector<gif::RGBpixel>> frames;
			for (double i = 0.0; i < 360.0; i += 10.0) {
				std::vector<hsv<double>> pixelsHSV;
				std::vector<gif::RGBpixel> outPixels;
				for (double h = 0.0 + i; h < 360.0 + i; h += 0.1) {
					pixelsHSV.emplace_back(hsv<double>(double(h), 1.0, 1.0));
				}
				for (auto const& x : pixelsHSV) {
					auto asFloat = rgb<double>(x);
					outPixels.emplace_back(asFloat.toPixelRGB());
				}
				frames.emplace_back(outPixels);
			}
			//pad to number which does a clean sqrt

			uint16_t width = uint16_t(sqrt(frames[0].size()));
			auto height = width;

			for (auto& frame : frames) {
				frame.resize(size_t(width) * size_t(height));
			}

			auto enc = gif::encoder(width, height, frames);
			auto hasImg = enc.write();
			if (hasImg) {
				auto img = hasImg.value();
				auto outFilePath = std::filesystem::path("animated.gif");
				auto outFile = std::ofstream(outFilePath.string(), std::ofstream::binary);
				outFile.write(reinterpret_cast<const char*>(img.data()), img.size());
			}
		}

		TEST_METHOD(generateRainbowAnimatedBuggy) {
			std::vector<std::vector<gif::RGBpixel>> frames;
			for (double i = 0.0; i < 360.0; i += 10.0) {
				std::vector<hsv<double>> pixelsHSV;
				std::vector<gif::RGBpixel> outPixels;
				for (double h = 0.0 + i; h < 360.0 + i; h += 0.5) {
					pixelsHSV.emplace_back(hsv<double>(double(h), 1.0, 1.0));
				}
				for (auto const& x : pixelsHSV) {
					auto asFloat = rgb<double>(x);
					outPixels.emplace_back(asFloat.toPixelRGB());
				}
				frames.emplace_back(outPixels);
			}
			//pad to number which does a clean sqrt

			uint16_t width = uint16_t(sqrt(frames[0].size()));
			auto height = width;

			for (auto& frame : frames) {
				frame.resize(size_t(width) * size_t(height));
			}

			auto enc = gif::encoder(width, height, frames);
			auto hasImg = enc.write();
			if (hasImg) {
				auto img = hasImg.value();
				auto outFilePath = std::filesystem::path("largeanimation.gif");
				auto outFile = std::ofstream(outFilePath.string(), std::ofstream::binary);
				outFile.write(reinterpret_cast<const char*>(img.data()), img.size());
			}
		}


	};

	TEST_CLASS(Output)
	{
	public:
		TEST_METHOD(TestFullEncode) {
			//auto const black = gif::RGBpixel{ 40,40,40 };
			//auto const white = gif::RGBpixel{ 255,255,255 };
			//auto const red = gif::RGBpixel{ 255,0,0 };
			//auto const green = gif::RGBpixel{ 0,255,0 };
			//auto const blue = gif::RGBpixel{ 0,0,255 };

			std::vector<gif::RGBpixel> image{
				{40,40,40},		{255,255,255},	{255,255,255},
				{255,255,255},	{40,40,40},		{255,255,255},
				{255,255,255},	{255,255,255},	{255,255,255},
				{255,255,255},	{255,255,255},	{255,255,255},
				{255,255,255},	{255,255,255},	{255,255,255},
			};

			//std::vector<gif::RGBpixel> image{
			//	red,white,red,
			//	white,black,green,
			//	white,white,blue,
			//	white,white,white,
			//	white,white,white
			//};

			auto enc = gif::encoder(3, 5, image);
			//For now the colortable and image are missing


			auto binaryImage = enc.write();
			if (binaryImage) {
				auto const img = binaryImage.value();

				auto outFilePath = std::filesystem::path("out.gif");
				auto outFile = std::ofstream(outFilePath.string(), std::ofstream::binary);
				outFile.write(reinterpret_cast<const char*>(img.data()), img.size());
			}
		}

	};
	TEST_CLASS(Internals)
	{
	public:
		TEST_METHOD(TestMethodPalletize)
		{
			std::vector<gif::RGBpixel> image{
				{10,20,30},
			{40,50,60},
			{70,80,90},
			{100,110,120},
			{130,140,150},
			{160,170,180},
			{190,200,210},
			{220,230,240}
			};
			auto palette = gif::palletize(image, 8);
			Assert::IsTrue(std::equal(image.begin(), image.end(), palette.begin()));
		}

		//We want our palletization code to support creating palettes larger than
		//needed to represent the original data. Extra space is padded with zeroes.
		TEST_METHOD(TestMethodPalletizeOversized)
		{
			std::vector<gif::RGBpixel> image{
				{10,20,30},
			{40,50,60},
			{70,80,90},
			{100,110,120},
			{130,140,150},
			{160,170,180},
			{190,200,210},
			{220,230,240}
			};
			auto palette = gif::palletize(image, 256);
			Assert::IsFalse(std::equal(image.begin(), image.end(), palette.begin()));
		}

		TEST_METHOD(TestMapPixels)
		{
			std::vector<gif::RGBpixel> image{
				{10,20,30},
			{40,50,60},
			{70,80,90},
			{100,110,120},
			{130,140,150},
			{160,170,180},
			{190,200,210},
			{220,230,240}
			};
			auto palette = gif::palletize(image, 8);
			Assert::IsTrue(std::equal(image.begin(), image.end(), palette.begin()));

			auto mapped = gif::mapPixels(image, palette);
		}

		TEST_METHOD(TestPixel)
		{
			auto expected = std::vector<byte>{ byte(0x55), byte(0xff), byte(0x00) };
			auto pixel = gif::RGBpixel{ 0x55,0xff,0x00 };
			auto bytes = pixel.write();
			Assert::IsTrue(std::equal(bytes.begin(), bytes.end(), expected.begin()));
		}

		TEST_METHOD(TestPixelWide)
		{
			auto expectedBE = std::vector<byte>{
				byte(0xff) ,byte(0x00) ,byte(0x11) ,byte(0xee),
				byte(0xd0) ,byte(0xd0) ,byte(0xd0) ,byte(0xd0),
				byte(0xab) ,byte(0xcd) ,byte(0xef) ,byte(0x01)
			};

			auto expectedLE = std::vector<byte>{
				byte(0xee),byte(0x11),byte(0x00) ,byte(0xff) ,
				byte(0xd0),byte(0xd0),byte(0xd0) ,byte(0xd0) ,
				byte(0x01),byte(0xef),byte(0xcd) ,byte(0xab) ,
			};

			auto pixel = gif::RGBpixel32{ 0xff0011ee,0xd0d0d0d0,0xabcdef01 };
			auto bytes = pixel.write();
			Assert::IsFalse(std::equal(bytes.begin(), bytes.end(), expectedBE.begin()));
			Assert::IsTrue(std::equal(bytes.begin(), bytes.end(), expectedLE.begin()));
		}

		TEST_METHOD(Pack12)
		{
			auto in = std::vector<std::bitset<12>>({ {0xf0f},{0x1e1 } });

			auto expected = std::vector<byte>(3);
			expected[0] = std::byte{ 0xf };
			expected[1] = std::byte{ 0x1f };
			expected[2] = std::byte{ 0x1e };

			auto out = gif::pack(in);

			Assert::IsTrue(std::equal(out.first.begin(), out.first.end(), expected.begin(), expected.end()));

		};

		TEST_METHOD(Pack9)
		{
			auto in = std::vector<std::bitset<9>>({ {0x100 } });

			auto expected = std::vector<byte>(2);
			expected[0] = std::byte{ 0x00 };
			expected[1] = std::byte{ 0x01 };

			auto out = gif::pack(in);

			Assert::IsTrue(std::equal(out.first.begin(), out.first.end(), expected.begin(), expected.end()));
		};

		TEST_METHOD(Pack7)
		{
			auto in = std::vector<std::bitset<7>>({ {0x1f },{0x7f} });

			auto expected = std::vector<byte>(2);
			expected[0] = std::byte{ 0x9f };
			expected[1] = std::byte{ 0x3f };

			auto out = gif::pack(in);

			Assert::IsTrue(std::equal(out.first.begin(), out.first.end(), expected.begin(), expected.end()));

		};
	};
}
