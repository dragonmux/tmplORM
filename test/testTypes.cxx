#include <crunch++.h>
#include "testTypes.hxx"

class testDateTime_t final : public testsuite
{
private:
	void testCtor() { testDateTime::testCtor(*this); }
	void testFromString() { testDateTime::testFromString(*this); }
	void testFromSystemTime() { testDateTime::testFromSystemTime(*this); }
	void testAsString() { testDateTime::testAsString(*this); }
	void testWrapper() { testDateTime::testWrapper(*this); }
	void testTimeZones() { testDateTime::testTimeZones(*this); }

public:
	void registerTests() final override
	{
		CXX_TEST(testCtor)
		CXX_TEST(testFromString)
		CXX_TEST(testFromSystemTime)
		CXX_TEST(testAsString)
		CXX_TEST(testWrapper)
		CXX_TEST(testTimeZones)
	}
};

class testDate_t final : public testsuite
{
private:
	void testCtor() { testDate::testCtor(*this); }
	void testFromString() { testDate::testFromString(*this); }
	void testAsString() { testDate::testAsString(*this); }
	void testWrapper() { testDate::testWrapper(*this); }

public:
	void registerTests() final override
	{
		CXX_TEST(testCtor)
		CXX_TEST(testFromString)
		CXX_TEST(testAsString)
		CXX_TEST(testWrapper)
	}
};

class testTime_t final : public testsuite
{
private:
	void testCtor() { testTime::testCtor(*this); }
	void testFromString() { testTime::testFromString(*this); }
	void testAsString() { testTime::testAsString(*this); }
	void testWrapper() { testTime::testWrapper(*this); }

public:
	void registerTests() final override
	{
		CXX_TEST(testCtor)
		CXX_TEST(testFromString)
		CXX_TEST(testAsString)
		CXX_TEST(testWrapper)
	}
};

class testTypes_t final : public testsuite
{
private:
	void testDate() { testTypes::testDate(*this); }
	void testTime() { testTypes::testTime(*this); }
	void testDateTime() { testTypes::testDateTime(*this); }
	void testUUID() { testTypes::testUUID(*this); }

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
	registerTestClasses<testDateTime_t, testDate_t, testTime_t, testTypes_t>();
}
