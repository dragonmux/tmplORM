#ifndef TEST_TYPES__HXX
#define TEST_TYPES__HXX

namespace dateTime
{
	void testCtor(testsuit &suite);
	void testFromString(testsuit &suite);
	void testFromSystemTime(testsuit &suite);
	void testAsString(testsuit &suite);
	void testWrapper(testsuit &suite);
}

namespace date
{
	void testCtor(testsuit &suite);
	void testFromString(testsuit &suite);
	void testAsString(testsuit &suite);
	void testWrapper(testsuit &suite);
}

namespace types
{
	void testDate(testsuit &suite);
	void testTime(testsuit &suite);
	void testDateTime(testsuit &suite);
	void testUUID(testsuit &suite);
}

#endif /*TEST_TYPES__HXX*/
