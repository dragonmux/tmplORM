#ifndef TEST_TYPES__HXX
#define TEST_TYPES__HXX

namespace dateTime
{
	void testCtor(testsuit &suite);
	void testFromString(testsuit &suite);
	void testFromSystemTime(testsuit &suite);
	void testWrapper(testsuit &suite);
}

namespace types
{
	void testDateTime(testsuit &suite);
	void testDate(testsuit &suite);
}

#endif /*TEST_TYPES__HXX*/
