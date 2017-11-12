#include <crunch++.h>
#include "testConversions.hxx"

struct testFromInt_t final : testsuit
{
public:
	void testUint8_t()
	{
		testFromUint8(*this,
			{
				{0, "0"},
				{127, "127"},
				{128, "128"},
				{255, "255"}
			}
		);
	}

	void testInt8_t()
	{
		testFromInt8(*this,
			{
				{0, "0"},
				{127, "127"},
				{-1, "-1"},
				{-127, "-127"},
				{-128, "-128"}
			}
		);
	}

	void testUint16_t()
	{
		testFromUint16(*this,
			{
				{0, "0"},
				{127, "127"},
				{128, "128"},
				{255, "255"},
				{256, "256"},
				{32767, "32767"},
				{32768, "32768"},
				{65535, "65535"}
			}
		);
	}

	void testInt16_t()
	{
		testFromInt16(*this,
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
			}
		);
	}

	void testUint32_t()
	{
		testFromUint32(*this,
			{
				{0, "0"},
				{127, "127"},
				{128, "128"},
				{255, "255"},
				{256, "256"},
				{32767, "32767"},
				{32768, "32768"},
				{65535, "65535"},
				{65536, "65536"},
				{2147483647, "2147483647"},
				{2147483648, "2147483648"},
				{4294967295, "4294967295"}
			}
		);
	}

	void testInt32_t()
	{
		testFromInt32(*this,
			{
				{0, "0"},
				{127, "127"},
				{128, "128"},
				{255, "255"},
				{256, "256"},
				{32767, "32767"},
				{32768, "32768"},
				{65535, "65535"},
				{65536, "65536"},
				{2147483647, "2147483647"},
				{-1, "-1"},
				{-127, "-127"},
				{-128, "-128"},
				{-255, "-255"},
				{-256, "-256"},
				{-32767, "-32767"},
				{-32768, "-32768"},
				{-65535, "-65535"},
				{-65536, "-65536"},
				{-2147483647, "-2147483647"},
#if __GNUC__ > 5
				{-2147483648, "-2147483648"}
#endif
			}
		);
	}

	void testUint64_t()
	{
		testFromUint64(*this,
			{
				{0, "0"},
				{127, "127"},
				{128, "128"},
				{255, "255"},
				{256, "256"},
				{32767, "32767"},
				{32768, "32768"},
				{65535, "65535"},
				{65536, "65536"},
				{2147483647, "2147483647"},
				{2147483648, "2147483648"},
				{4294967295, "4294967295"},
				{4294967296, "4294967296"},
				{140737488355327, "140737488355327"},
				{140737488355328, "140737488355328"},
				{9223372036854775807, "9223372036854775807"},
				{UINT64_C(9223372036854775808), "9223372036854775808"},
				{UINT64_C(18446744073709551615), "18446744073709551615"}
			}
		);
	}

	void testInt64_t()
	{
		testFromInt64(*this,
			{
				{0, "0"},
				{127, "127"},
				{128, "128"},
				{255, "255"},
				{256, "256"},
				{32767, "32767"},
				{32768, "32768"},
				{65535, "65535"},
				{65536, "65536"},
				{2147483647, "2147483647"},
				{2147483648, "2147483648"},
				{4294967295, "4294967295"},
				{4294967296, "4294967296"},
				{140737488355327, "140737488355327"},
				{140737488355328, "140737488355328"},
				{9223372036854775807, "9223372036854775807"},
				{-1, "-1"},
				{-127, "-127"},
				{-128, "-128"},
				{-255, "-255"},
				{-256, "-256"},
				{-32767, "-32767"},
				{-32768, "-32768"},
				{-65535, "-65535"},
				{-65536, "-65536"},
				{-2147483647, "-2147483647"},
				{-2147483648, "-2147483648"},
				{-4294967295, "-4294967295"},
				{-4294967296, "-4294967296"},
				{-140737488355327, "-140737488355327"},
				{-140737488355328, "-140737488355328"},
				{-9223372036854775807, "-9223372036854775807"},
#if __GNUC__ > 5
				{INT64_C(-9223372036854775808), "-9223372036854775808"}
#endif
			}
		);
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

struct testDecToInt_t final : testsuit
{
public:
	void testUint8_t()
	{
		testDecToUint8(*this,
			{
				{0, ""},
				{0, "0"},
				{127, "127"},
				{128, "128"},
				{255, "255"}
			}
		);
	}

	void testInt8_t()
	{
		testDecToInt8(*this,
			{
				{0, ""},
				{0, "0"},
				{127, "127"},
				{-1, "-1"},
				{-127, "-127"},
				{-128, "-128"}
			}
		);
	}

	void testUint16_t()
	{
		testDecToUint16(*this,
			{
				{0, ""},
				{0, "0"},
				{256, "256"},
				{32767, "32767"},
				{32768, "32768"},
				{65535, "65535"}
			}
		);
	}

	void testInt16_t()
	{
		testDecToInt16(*this,
			{
				{0, ""},
				{0, "0"},
				{128, "128"},
				{255, "255"},
				{256, "256"},
				{32767, "32767"},
				{-1, "-1"},
				{-255, "-255"},
				{-256, "-256"},
				{-32767, "-32767"},
				{-32768, "-32768"}
			}
		);
	}

	void testUint32_t()
	{
		testDecToUint32(*this,
			{
				{0, ""},
				{0, "0"},
				{65536, "65536"},
				{2147483647, "2147483647"},
				{2147483648, "2147483648"},
				{4294967295, "4294967295"}
			}
		);
	}

	void testInt32_t()
	{
		testDecToInt32(*this,
			{
				{0, ""},
				{0, "0"},
				{32768, "32768"},
				{65535, "65535"},
				{65536, "65536"},
				{2147483647, "2147483647"},
				{-1, "-1"},
				{-65535, "-65535"},
				{-65536, "-65536"},
				{-2147483647, "-2147483647"},
				{-2147483648, "-2147483648"}
			}
		);
	}

	void testUint64_t()
	{
		testDecToUint64(*this,
			{
				{0, ""},
				{0, "0"},
				{4294967296, "4294967296"},
				{140737488355327, "140737488355327"},
				{140737488355328, "140737488355328"},
				{9223372036854775807, "9223372036854775807"},
				{UINT64_C(9223372036854775808), "9223372036854775808"},
				{UINT64_C(18446744073709551615), "18446744073709551615"}
			}
		);
	}

	void testInt64_t()
	{
		testDecToInt64(*this,
			{
				{0, ""},
				{0, "0"},
				{2147483648, "2147483648"},
				{4294967295, "4294967295"},
				{4294967296, "4294967296"},
				{140737488355327, "140737488355327"},
				{140737488355328, "140737488355328"},
				{9223372036854775807, "9223372036854775807"},
				{-1, "-1"},
				{-4294967295, "-4294967295"},
				{-4294967296, "-4294967296"},
				{-140737488355327, "-140737488355327"},
				{-140737488355328, "-140737488355328"},
				{-9223372036854775807, "-9223372036854775807"},
				{INT64_C(-9223372036854775808), "-9223372036854775808"}
			}
		);
	}

	void testShouldFail()
	{
		testDecShouldFail(*this,
			{
				"-", "+", "a", "A",
				"-a", "-A", "$", "*",
				"/", "\x01", " ", "\t",
				"\n", "\r", "^", "&"
			}
		);
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
		CXX_TEST(testShouldFail)
	}
};

struct testHexToInt_t final : testsuit
{
public:
	void testUint8_t()
	{
		testHexToUint8(*this,
			{
				{0, ""},
				{0, "0"},
				{0, "00"},
				{127, "7F"},
				{128, "80"},
				{255, "FF"}
			}
		);
	}

	void testInt8_t()
	{
		testHexToInt8(*this,
			{
				{0, ""},
				{0, "00"},
				{127, "7F"},
				{-1, "FF"},
				{-127, "81"},
				{-128, "80"}
			}
		);
	}

	void testUint16_t()
	{
		testHexToUint16(*this,
			{
				{0, ""},
				{0, "00"},
				{256, "0100"},
				{32767, "7FFF"},
				{32768, "8000"},
				{65535, "FFFF"}
			}
		);
	}

	void testInt16_t()
	{
		testHexToInt16(*this,
			{
				{0, ""},
				{0, "00"},
				{128, "80"},
				{255, "FF"},
				{256, "0100"},
				{32767, "7FFF"},
				{-1, "FFFF"},
				{-255, "FF01"},
				{-256, "FF00"},
				{-32767, "8001"},
				{-32768, "8000"}
			}
		);
	}

	void testUint32_t()
	{
		testHexToUint32(*this,
			{
				{0, ""},
				{0, "00"},
				{65536, "00010000"},
				{2147483647, "7FFFFFFF"},
				{2147483648, "80000000"},
				{4294967295, "FFFFFFFF"}
			}
		);
	}

	void testInt32_t()
	{
		testHexToInt32(*this,
			{
				{0, ""},
				{0, "00"},
				{32768, "8000"},
				{65535, "FFFF"},
				{65536, "00010000"},
				{2147483647, "7FFFFFFF"},
				{-1, "FFFFFFFF"},
				{-65535, "FFFF0001"},
				{-65536, "FFFF0000"},
				{-2147483647, "80000001"},
				{-2147483648, "80000000"}
			}
		);
	}

	void testUint64_t()
	{
		testHexToUint64(*this,
			{
				{0, ""},
				{0, "00"},
				{4294967296, "0000000100000000"},
				{140737488355327, "00007FFFFFFFFFFF"},
				{140737488355328, "0000800000000000"},
				{9223372036854775807, "7FFFFFFFFFFFFFFF"},
				{UINT64_C(9223372036854775808), "8000000000000000"},
				{UINT64_C(18446744073709551615), "FFFFFFFFFFFFFFFF"}
			}
		);
	}

	void testInt64_t()
	{
		testHexToInt64(*this,
			{
				{0, ""},
				{0, "00"},
				{2147483648, "80000000"},
				{4294967295, "FFFFFFFF"},
				{4294967296, "0000000100000000"},
				{140737488355327, "00007FFFFFFFFFFF"},
				{140737488355328, "0000800000000000"},
				{9223372036854775807, "7FFFFFFFFFFFFFFF"},
				{-1, "FFFFFFFFFFFFFFFF"},
				{-4294967295, "FFFFFFFF00000001"},
				{-4294967296, "FFFFFFFF00000000"},
				{-140737488355327, "FFFF800000000001"},
				{-140737488355328, "FFFF800000000000"},
				{-9223372036854775807, "8000000000000001"},
				{INT64_C(-9223372036854775808), "8000000000000000"}
			}
		);
	}

	void testShouldFail()
	{
		testHexShouldFail(*this,
			{
				"-", "+", "-a", "-A",
				"-0", "-$", "$", "*",
				"/", "\x01", " ", "\t",
				"\n", "\r", "^", "&"
			}
		);
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
		CXX_TEST(testShouldFail)
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
		assertEqual(testSwapBytes(uint64_t(0x00FF0FF0FF00F00F)), 0x0FF000FFF00FFF00);
		assertEqual(testSwapBytes(uint64_t(0x55AA5AA5AA55A55A)), 0x5AA555AAA55AAA55);
		assertEqual(testSwapBytes(uint64_t(0x33EE3EE3EE33E33E)), 0x3EE333EEE33EEE33);
		assertEqual(testSwapBytes(uint64_t(0x0123456789ABCDEF)), 0xEFCDAB8967452301);
		assertEqual(testSwapBytes(uint64_t(0xFEDCBA9876543210)), 0x1032547698BADCFE);
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
	registerTestClasses<testFromInt_t, testDecToInt_t,
		testHexToInt_t, testSwapBytes_t>();
}
