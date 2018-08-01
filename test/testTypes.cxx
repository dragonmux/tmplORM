#include <crunch++.h>
#include "testTypes.hxx"

class testDateTime_t final : public testsuit
{
private:
	void testCtor() { dateTime::testCtor(*this); }
	void testFromString() { dateTime::testFromString(*this); }
	void testFromSystemTime() { dateTime::testFromSystemTime(*this); }
	void testWrapper() { dateTime::testWrapper(*this); }

	void testDate() { types::testDate(*this); }
	void testTime() { types::testTime(*this); }
	void testDateTime() { types::testDateTime(*this); }
	void testUUID() { types::testUUID(*this); }

public:
	void registerTests() final override
	{
		CXX_TEST(testCtor)
		CXX_TEST(testFromString)
		CXX_TEST(testFromSystemTime)
		CXX_TEST(testWrapper)

		CXX_TEST(testDate)
		CXX_TEST(testTime)
		CXX_TEST(testDateTime)
		CXX_TEST(testUUID)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testDateTime_t>();
}
