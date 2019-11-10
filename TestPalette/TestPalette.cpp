#include "pch.h"
#include "CppUnitTest.h"
#include "../gif_animation/gif_animation.h"
#include <algorithm>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace TestPalette
{
	TEST_CLASS(TestPalette)
	{
	public:

		TEST_METHOD(TestMethodPalletize)
		{
			std::vector<gif::RGBpixel> image{ {10,20,30},{40,50,60},{70,80,90},{100,110,120},{130,140,150},{160,170,180},{190,200,210},{220,230,240} };
			auto palette = gif::palletize(image, 8);
			Assert::IsTrue(std::equal(image.begin(), image.end(), palette.begin()));
		}
		TEST_METHOD(TestLZWWiki)
		{
			gif::encoder enc;
			//std::vector<gif::RGBpixel> dummy;
			//dummy.resize(0x100);
			//gif::colorTable table(dummy);

			auto out = std::vector<byte>{ byte(0x00), byte(0x51), byte(0xFC), byte(0x1B), byte(0x28), byte(0x70), byte(0xA0), byte(0xC1), byte(0x83), byte(0x01),byte(0x01) };
			auto in = std::vector<byte>{ byte(0x28), byte(0xff), byte(0xff), byte(0xff), byte(0x28), byte(0xff), byte(0xff), byte(0xff), byte(0xff), byte(0xff), byte(0xff), byte(0xff), byte(0xff), byte(0xff), byte(0xff), };
			auto res = enc.lzw_encode(in, 8);
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
				Assert::IsTrue(std::equal((*out).begin(), (*out).end(), expected.begin(), expected.end()));
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
				Assert::IsTrue(std::equal((*out).begin(), (*out).end(), expected.begin(), expected.end()));
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
				Assert::IsTrue(std::equal((*out).begin(), (*out).end(), expected.begin(), expected.end()));
			}
			else {
				Assert::Fail();
			}
		}
	};
}
