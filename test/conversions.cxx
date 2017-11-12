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

template<typename int_t> struct toIntType_t
{
private:
	toInt_t<int_t> value;

public:
	toIntType_t(const char *const test) noexcept : value(test) { }
	template<typename test_t> void test(testsuit &suite, const char *const test)
	{
		test_t what{suite, test};
		what(value);
	}
};

template<typename... int_t> struct toIntTypes_t;
template<typename int_t, typename... ints_t> struct toIntTypes_t<int_t, ints_t...> : toIntTypes_t<ints_t...>
{
private:
	toIntType_t<int_t> type;

public:
	toIntTypes_t(const char *const test) noexcept : toIntTypes_t<ints_t...>(test), type(test) { }

	template<template<typename> class test_t> void test(testsuit &suite, const char *const test)
	{
		type.template test<test_t<toInt_t<int_t>>>(suite, test);
		toIntTypes_t<ints_t...>::template test<test_t>(suite, test);
	}
};

template<> struct toIntTypes_t<>
{
	toIntTypes_t(const char *const) noexcept { }
	template<template<typename> class test_t> void test(testsuit &, const char *const) { }
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

template<typename toInt_t> struct testDecShouldFail_t
{
private:
	testsuit &suite;
	const char *const test;

public:
	testDecShouldFail_t(testsuit &suite_, const char *const test_) noexcept : suite(suite_), test(test_) { }

	void operator ()(toInt_t &value)
	{
		suite.assertFalse(value.isInt());
		suite.assertEqual(value.length(), str_t::length(test));
	}
};

void testDecShouldFail(testsuit &suite, const testFailStr_t tests)
{
	for (const char *const test : tests)
	{
		toIntTypes_t<uint8_t, int8_t, uint16_t, int16_t, uint32_t, int32_t, uint64_t, int64_t> types{test};
		types.template test<testDecShouldFail_t>(suite, test);
	}
}

template<typename int_t> int_t swapBytes_(const int_t val) noexcept
{
	auto result(val);
	swapBytes(result);
	return result;
}

uint16_t testSwapBytes(const uint16_t val) noexcept { return swapBytes_(val); }
uint32_t testSwapBytes(const uint32_t val) noexcept { return swapBytes_(val); }
uint64_t testSwapBytes(const uint64_t val) noexcept { return swapBytes_(val); }
