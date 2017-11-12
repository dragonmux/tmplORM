#include <crunch++.h>
#include <conversions.hxx>
#include <testConversions.hxx>

using str_t = std::char_traits<char>;

template<typename int_t> struct testFromInt_t
{
private:
	using fromInt = fromInt_t<int_t, int_t>;

public:
	void testConversions(testsuit &suite, const testOk_t<int_t> &tests)
	{
		for (const auto &test : tests)
		{
			std::unique_ptr<char []> value = fromInt{test.first};
			suite.assertEqual(value.get(), test.second);
		}
	}
};

void testFromUint8(testsuit &suite, const testOk_t<uint8_t> tests)
	{ testFromInt_t<uint8_t> tester; tester.testConversions(suite, tests); }
void testFromInt8(testsuit &suite, const testOk_t<int8_t> tests)
	{ testFromInt_t<int8_t> tester; tester.testConversions(suite, tests); }
void testFromUint16(testsuit &suite, const testOk_t<uint16_t> tests)
	{ testFromInt_t<uint16_t> tester; tester.testConversions(suite, tests); }
void testFromInt16(testsuit &suite, const testOk_t<int16_t> tests)
	{ testFromInt_t<int16_t> tester; tester.testConversions(suite, tests); }
void testFromUint32(testsuit &suite, const testOk_t<uint32_t> tests)
	{ testFromInt_t<uint32_t> tester; tester.testConversions(suite, tests); }
void testFromInt32(testsuit &suite, const testOk_t<int32_t> tests)
	{ testFromInt_t<int32_t> tester; tester.testConversions(suite, tests); }
void testFromUint64(testsuit &suite, const testOk_t<uint64_t> tests)
	{ testFromInt_t<uint64_t> tester; tester.testConversions(suite, tests); }
void testFromInt64(testsuit &suite, const testOk_t<int64_t> tests)
	{ testFromInt_t<int64_t> tester; tester.testConversions(suite, tests); }

template<typename int_t> struct testToInt_t
{
private:
	using toInt = toInt_t<int_t>;

public:
	void testIntConversions(testsuit &suite, const testOk_t<int_t> &tests)
	{
		for (const auto &test : tests)
		{
			auto value = toInt{test.second};
			suite.assertTrue(value.isInt());
			suite.assertEqual(value.length(), str_t::length(test.second));
			suite.assertEqual(value, test.first);
		}
	}
};

void testDecToUint8(testsuit &suite, const testOk_t<uint8_t> tests)
	{ testToInt_t<uint8_t> tester; tester.testIntConversions(suite, tests); }
void testDecToInt8(testsuit &suite, const testOk_t<int8_t> tests)
	{ testToInt_t<int8_t> tester; tester.testIntConversions(suite, tests); }
void testDecToUint16(testsuit &suite, const testOk_t<uint16_t> tests)
	{ testToInt_t<uint16_t> tester; tester.testIntConversions(suite, tests); }
void testDecToInt16(testsuit &suite, const testOk_t<int16_t> tests)
	{ testToInt_t<int16_t> tester; tester.testIntConversions(suite, tests); }
void testDecToUint32(testsuit &suite, const testOk_t<uint32_t> tests)
	{ testToInt_t<uint32_t> tester; tester.testIntConversions(suite, tests); }
void testDecToInt32(testsuit &suite, const testOk_t<int32_t> tests)
	{ testToInt_t<int32_t> tester; tester.testIntConversions(suite, tests); }
void testDecToUint64(testsuit &suite, const testOk_t<uint64_t> tests)
	{ testToInt_t<uint64_t> tester; tester.testIntConversions(suite, tests); }
void testDecToInt64(testsuit &suite, const testOk_t<int64_t> tests)
	{ testToInt_t<int64_t> tester; tester.testIntConversions(suite, tests); }

template<typename int_t> int_t swapBytes_(const int_t val) noexcept
{
	auto result(val);
	swapBytes(result);
	return result;
}

uint16_t testSwapBytes(const uint16_t val) noexcept { return swapBytes_(val); }
uint32_t testSwapBytes(const uint32_t val) noexcept { return swapBytes_(val); }
uint64_t testSwapBytes(const uint64_t val) noexcept { return swapBytes_(val); }
