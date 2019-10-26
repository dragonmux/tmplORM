#ifndef TEST_TYPES__HXX
#define TEST_TYPES__HXX

namespace testDateTime
{
	void testCtor(testsuit &suite);
	void testFromString(testsuit &suite);
	void testFromSystemTime(testsuit &suite);
	void testAsString(testsuit &suite);
	void testWrapper(testsuit &suite);
}

namespace testDate
{
	void testCtor(testsuit &suite);
	void testFromString(testsuit &suite);
	void testAsString(testsuit &suite);
	void testWrapper(testsuit &suite);
}

namespace testTypes
{
	void testDate(testsuit &suite);
	void testTime(testsuit &suite);
	void testDateTime(testsuit &suite);
	void testUUID(testsuit &suite);
}

#endif /*TEST_TYPES__HXX*/
