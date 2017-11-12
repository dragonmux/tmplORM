#include <crunch++.h>
#include <conversions.hxx>
#include <testConversions.hxx>

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

void testUint8(testsuit &suite, const testOk_t<uint8_t> tests)
	{ testFromInt_t<uint8_t> tester; tester.testConversions(suite, tests); }
void testInt8(testsuit &suite, const testOk_t<int8_t> tests)
	{ testFromInt_t<int8_t> tester; tester.testConversions(suite, tests); }
void testUint16(testsuit &suite, const testOk_t<uint16_t> tests)
	{ testFromInt_t<uint16_t> tester; tester.testConversions(suite, tests); }
void testInt16(testsuit &suite, const testOk_t<int16_t> tests)
	{ testFromInt_t<int16_t> tester; tester.testConversions(suite, tests); }
void testUint32(testsuit &suite, const testOk_t<uint32_t> tests)
	{ testFromInt_t<uint32_t> tester; tester.testConversions(suite, tests); }
void testInt32(testsuit &suite, const testOk_t<int32_t> tests)
	{ testFromInt_t<int32_t> tester; tester.testConversions(suite, tests); }
void testUint64(testsuit &suite, const testOk_t<uint64_t> tests)
	{ testFromInt_t<uint64_t> tester; tester.testConversions(suite, tests); }
void testInt64(testsuit &suite, const testOk_t<int64_t> tests)
	{ testFromInt_t<int64_t> tester; tester.testConversions(suite, tests); }
