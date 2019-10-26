#include <crunch++.h>
#include "testTypes.hxx"

class testDateTime_t final : public testsuit
{
private:
	void testCtor() { dateTime::testCtor(*this); }
	void testFromString() { dateTime::testFromString(*this); }
	void testFromSystemTime() { dateTime::testFromSystemTime(*this); }
	void testAsString() { dateTime::testAsString(*this); }
	void testWrapper() { dateTime::testWrapper(*this); }

public:
	void registerTests() final override
	{
		CXX_TEST(testCtor)
		CXX_TEST(testFromString)
		CXX_TEST(testFromSystemTime)
		CXX_TEST(testAsString)
		CXX_TEST(testWrapper)
	}
};

class testDate_t final : public testsuit
{
private:
	void testCtor() { date::testCtor(*this); }
	void testFromString() { date::testFromString(*this); }
	void testAsString() { date::testAsString(*this); }
	void testWrapper() { date::testWrapper(*this); }

public:
	void registerTests() final override
	{
		CXX_TEST(testCtor)
		CXX_TEST(testFromString)
		CXX_TEST(testAsString)
		CXX_TEST(testWrapper)
	}
};

class testTypes_t final : public testsuit
{
private:
	void testDate() { types::testDate(*this); }
	void testTime() { types::testTime(*this); }
	void testDateTime() { types::testDateTime(*this); }
	void testUUID() { types::testUUID(*this); }

public:
	void registerTests() final override
	{
		CXX_TEST(testDate)
		CXX_TEST(testTime)
		CXX_TEST(testDateTime)
		CXX_TEST(testUUID)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testDateTime_t, testDate_t, testTypes_t>();
}
