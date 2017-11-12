#include <crunch++.h>
#include "testConversions.hxx"

struct testFromInt_t final : testsuit
{
public:
	void testUint8_t()
	{
		testUint8(*this,
			{
				{0, "0"},
				{127, "127"},
				{128, "128"},
				{255, "255"},
			});
	}

	void testInt8_t()
	{
		testInt8(*this,
			{
				{0, "0"},
				{127, "127"},
				{-1, "-1"},
				{-127, "-127"},
				{-128, "-128"}
			});
	}

	void registerTests() final override
	{
		CXX_TEST(testUint8_t)
		CXX_TEST(testInt8_t)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testFromInt_t>();
}
