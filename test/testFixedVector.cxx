#include <crunch++.h>
#include <fixedVector.hxx>
#include "testFixedVector.hxx"

class testFixedVector final : public testsuit
{
public:
	void registerTests() final override
	{
	}
};

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

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testFixedVector, testBoundedIterator>();
}
