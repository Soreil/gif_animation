#include "pch.h"
#include "CppUnitTest.h"
#include "../gif_animation/gif_animation.h"
#include <algorithm>
#include <bitset>
#include <optional>
#include <variant>
#include <fstream>
#include <filesystem>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace TestPalette
{
	TEST_CLASS(Output)
	{
	public:
		TEST_METHOD(TestFullEncode) {
			std::vector<gif::RGBpixel> image{
				{40,40,40},		{255,255,255},	{255,255,255},
				{255,255,255},	{40,40,40},		{255,255,255},
				{255,255,255},	{255,255,255},	{255,255,255},
				{255,255,255},	{255,255,255},	{255,255,255},
				{255,255,255},	{255,255,255},	{255,255,255},
			};

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

		TEST_METHOD(TestLZWWiki)
		{
			gif::encoder enc;

			auto expected = std::vector<byte>{ byte(0x00), byte(0x51), byte(0xFC), byte(0x1B), byte(0x28), byte(0x70), byte(0xA0), byte(0xC1), byte(0x83), byte(0x01),byte(0x01) };
			auto in = std::vector<byte>{ byte(0x28), byte(0xff), byte(0xff), byte(0xff), byte(0x28), byte(0xff), byte(0xff), byte(0xff), byte(0xff), byte(0xff), byte(0xff), byte(0xff), byte(0xff), byte(0xff), byte(0xff), };

			auto encoded = enc.lzw_encode(in, 8);

			//std::optional<std::pair<std::vector<byte>,size_t>
			auto packed = std::visit([](auto const& v) -> auto {
				return gif::pack(v);
			}, encoded);

			if (packed) {
				Assert::IsTrue(std::equal((*packed).first.begin(), (*packed).first.end(), expected.begin(), expected.end()));
			}
			else {
				Assert::Fail();
			}
		}

		TEST_METHOD(TestLZWWiki6)
		{
			gif::encoder enc;

			auto out = std::vector<byte>{ byte(0x00), byte(0x51), byte(0xFC), byte(0x1B), byte(0x28), byte(0x70), byte(0xA0), byte(0xC1), byte(0x83), byte(0x01),byte(0x01) };
			auto in = std::vector<byte>{ byte(0x28), byte(0x3f), byte(0x3f), byte(0x3f), byte(0x28), byte(0x3f), byte(0x3f), byte(0x3f), byte(0x3f), byte(0x3f), byte(0x3f), byte(0x3f), byte(0x3f), byte(0x3f), byte(0x3f), };
			auto encoded = enc.lzw_encode(in, 6);

			auto packed = std::visit([](auto const& v) -> auto {
				return gif::pack(v);
			}, encoded);

		}

		TEST_METHOD(Pack12)
		{
			auto in = std::vector<std::bitset<12>>({ {0xf0f},{0x1e1 } });

			auto expected = std::vector<byte>(3);
			expected[0] = std::byte{ 0xf };
			expected[1] = std::byte{ 0x1f };
			expected[2] = std::byte{ 0x1e };

			auto out = gif::pack(in);

			if (out) {
				Assert::IsTrue(std::equal((*out).first.begin(), (*out).first.end(), expected.begin(), expected.end()));
			}
			else {
				Assert::Fail();
			}
		}
		TEST_METHOD(Pack9)
		{
			auto in = std::vector<std::bitset<9>>({ {0x100 } });

			auto expected = std::vector<byte>(2);
			expected[0] = std::byte{ 0x00 };
			expected[1] = std::byte{ 0x01 };

			auto out = gif::pack(in);

			if (out) {
				Assert::IsTrue(std::equal((*out).first.begin(), (*out).first.end(), expected.begin(), expected.end()));
			}
			else {
				Assert::Fail();
			}
		}

		TEST_METHOD(Pack7)
		{
			auto in = std::vector<std::bitset<7>>({ {0x1f },{0x7f} });

			auto expected = std::vector<byte>(2);
			expected[0] = std::byte{ 0x9f };
			expected[1] = std::byte{ 0x3f };

			auto out = gif::pack(in);

			if (out) {
				Assert::IsTrue(std::equal((*out).first.begin(), (*out).first.end(), expected.begin(), expected.end()));
			}
			else {
				Assert::Fail();
			}
		}
	};
}
