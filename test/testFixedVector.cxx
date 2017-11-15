#include <crunch++.h>
#include <fixedVector.hxx>
#include "testFixedVector.hxx"

class testBoundedIterator final : public testsuit
{
public:
	void testCtor() { boundedIterator::testCtor(*this); }
	void testIndex() { boundedIterator::testIndex(*this); }
	void testInc() { boundedIterator::testInc(*this); }
	void testDec() { boundedIterator::testDec(*this); }

	void registerTests() final override
	{
		CXX_TEST(testCtor)
		CXX_TEST(testIndex)
		CXX_TEST(testInc)
		CXX_TEST(testDec)
	}
};

class testFixedVector final : public testsuit
{
public:
	void testInvalid() { fixedVector::testInvalid(*this); }
	void testIndexing() { fixedVector::testIndexing(*this); }
	void testSwap() { fixedVector::testSwap(*this); }

	void registerTests() final override
	{
		CXX_TEST(testInvalid)
		CXX_TEST(testIndexing)
		CXX_TEST(testSwap)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testBoundedIterator, testFixedVector>();
}
