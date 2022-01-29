#ifndef TEST_TYPES_HXX
#define TEST_TYPES_HXX

namespace testDateTime
{
	void testCtor(testsuite &suite);
	void testFromString(testsuite &suite);
	void testFromSystemTime(testsuite &suite);
	void testAsString(testsuite &suite);
	void testWrapper(testsuite &suite);
	void testTimeZones(testsuite &suite);
}

namespace testDate
{
	void testCtor(testsuite &suite);
	void testFromString(testsuite &suite);
	void testAsString(testsuite &suite);
	void testWrapper(testsuite &suite);
}

namespace testTime
{
	void testCtor(testsuite &suite);
	void testFromString(testsuite &suite);
	void testAsString(testsuite &suite);
	void testWrapper(testsuite &suite);
}

namespace testTypes
{
	void testDate(testsuite &suite);
	void testTime(testsuite &suite);
	void testDateTime(testsuite &suite);
	void testUUID(testsuite &suite);
}

#endif /*TEST_TYPES_HXX*/
