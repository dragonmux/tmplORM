#include <crunch++.h>
#include "testTypes.hxx"

class testDateTime_t final : public testsuit
{
private:
	void testCtor() { dateTime::testCtor(*this); }
	void testFromString() { dateTime::testFromString(*this); }
	void testFromSystemTime() { dateTime::testFromSystemTime(*this); }

public:
	void registerTests() final override
	{
		CXX_TEST(testCtor)
		CXX_TEST(testFromString)
		CXX_TEST(testFromSystemTime)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testDateTime_t>();
}
