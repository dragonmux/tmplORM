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

struct testSwapBytes_t final : testsuit
{
public:
	void testSwapBytes16()
	{
		assertEqual(testSwapBytes(uint16_t(0x0FF0)), 0xF00F);
		assertEqual(testSwapBytes(uint16_t(0x5AA5)), 0xA55A);
		assertEqual(testSwapBytes(uint16_t(0x3EE3)), 0xE33E);
	}

	void testSwapBytes32()
	{
		assertEqual(testSwapBytes(uint32_t(0x0FF000FF)), 0xFF00F00F);
		assertEqual(testSwapBytes(uint32_t(0x5AA555AA)), 0xAA55A55A);
		assertEqual(testSwapBytes(uint32_t(0x3EE333EE)), 0xEE33E33E);
		assertEqual(testSwapBytes(uint32_t(0x01234567)), 0x67452301);
		assertEqual(testSwapBytes(uint32_t(0x76543210)), 0x10325476);
	}

	void testSwapBytes64()
	{
		//assertEqual(testSwapBytes(uint64_t(0x0FF0)), 0xF00F);
		//assertEqual(testSwapBytes(uint64_t(0x5AA5)), 0xA55A);
		//assertEqual(testSwapBytes(uint64_t(0x3EE3)), 0xE33E);
	}

	void registerTests() final override
	{
		CXX_TEST(testSwapBytes16)
		CXX_TEST(testSwapBytes32)
		CXX_TEST(testSwapBytes64)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testFromInt_t, testSwapBytes_t>();
}
