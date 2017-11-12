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
				{255, "255"}
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

	void testUint16_t()
	{
		testUint16(*this,
			{
				{0, "0"},
				{127, "127"},
				{128, "128"},
				{255, "255"},
				{256, "256"},
				{32767, "32767"},
				{32768, "32768"},
				{65535, "65535"}
			});
	}

	void testInt16_t()
	{
		testInt16(*this,
			{
				{0, "0"},
				{127, "127"},
				{128, "128"},
				{255, "255"},
				{256, "256"},
				{32767, "32767"},
				{-1, "-1"},
				{-127, "-127"},
				{-128, "-128"},
				{-255, "-255"},
				{-256, "-256"},
				{-32767, "-32767"},
				{-32768, "-32768"}
			});
	}

	void testUint32_t()
	{
		testUint32(*this,
			{
				{0, "0"},
				{127, "127"},
				{128, "128"},
				{255, "255"},
				{256, "256"},
				{32767, "32767"},
				{32768, "32768"},
				{65535, "65535"}
			});
	}

	void testInt32_t()
	{
		testInt32(*this,
			{
				{0, "0"},
				{127, "127"},
				{128, "128"},
				{255, "255"},
				{256, "256"},
				{32767, "32767"},
				{-1, "-1"},
				{-127, "-127"},
				{-128, "-128"},
				{-255, "-255"},
				{-256, "-256"},
				{-32767, "-32767"},
				{-32768, "-32768"}
			});
	}

	void testUint64_t()
	{
		testUint64(*this,
			{
				{0, "0"},
				{127, "127"},
				{128, "128"},
				{255, "255"},
				{256, "256"},
				{32767, "32767"},
				{32768, "32768"},
				{65535, "65535"}
			});
	}

	void testInt64_t()
	{
		testInt64(*this,
			{
				{0, "0"},
				{127, "127"},
				{128, "128"},
				{255, "255"},
				{256, "256"},
				{32767, "32767"},
				{-1, "-1"},
				{-127, "-127"},
				{-128, "-128"},
				{-255, "-255"},
				{-256, "-256"},
				{-32767, "-32767"},
				{-32768, "-32768"}
			});
	}

	void registerTests() final override
	{
		CXX_TEST(testUint8_t)
		CXX_TEST(testInt8_t)
		CXX_TEST(testUint16_t)
		CXX_TEST(testInt16_t)
		CXX_TEST(testUint32_t)
		CXX_TEST(testInt32_t)
		CXX_TEST(testUint64_t)
		CXX_TEST(testInt64_t)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testFromInt_t>();
}
