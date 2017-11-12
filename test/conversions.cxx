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
