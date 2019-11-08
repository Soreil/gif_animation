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
			std::vector<gif::RGBpixel> dummy;
			dummy.resize(0x100);
			gif::colorTable table(dummy);

			//auto out = std::vector<byte>{ byte(0x00), byte(0x51), byte(0xFC), byte(0x1B), byte(0x28), byte(0x70), byte(0xA0), byte(0xC1), byte(0x83), byte(0x01),byte(0x01) };
			auto in = std::vector<byte>{ byte(0x28), byte(0xff), byte(0xff), byte(0xff), byte(0x28), byte(0xff), byte(0xff), byte(0xff), byte(0xff), byte(0xff), byte(0xff), byte(0xff), byte(0xff), byte(0xff), byte(0xff), };
			auto res = enc.lzw_encode(in,table);
		}
	};
}
