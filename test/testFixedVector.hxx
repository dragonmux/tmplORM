#ifndef TEST_FIXED_VECTOR_HXX
#define TEST_FIXED_VECTOR_HXX

namespace boundedIterator
{
	void testCtor(testsuite &suite);
	void testIndex(testsuite &suite);
	void testInc(testsuite &suite);
	void testDec(testsuite &suite);
}

namespace fixedVector
{
	void testInvalid(testsuite &suite);
	void testIndexing(testsuite &suite);
	void testSwap(testsuite &suite);
}

#endif /*TEST_FIXED_VECTOR_HXX*/
