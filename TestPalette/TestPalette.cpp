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
			auto palette = gif::palletize(image,8);
			Assert::IsTrue(std::equal(image.begin(),image.end(),palette.begin()));
		}
	};
}
