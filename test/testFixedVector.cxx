#include <crunch++.h>
#include <substrate/fixed_vector>
#include "testFixedVector.hxx"

using substrate::fixedVector_t;

class testBoundedIterator final : public testsuite
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

class testFixedVector final : public testsuite
{
public:
	void testTraits()
	{
		assertTrue(std::is_move_constructible<fixedVector_t<char>>::value);
		assertTrue(std::is_move_assignable<fixedVector_t<char>>::value);
		assertTrue(std::is_default_constructible<fixedVector_t<char>>::value);
		assertFalse(std::is_copy_constructible<fixedVector_t<char>>::value);
		assertFalse(std::is_copy_assignable<fixedVector_t<char>>::value);
	}

	void testInvalid() { fixedVector::testInvalid(*this); }
	void testSwap() { fixedVector::testSwap(*this); }

	void testIndexing() try
		{ fixedVector::testIndexing(*this); }
	catch (const std::out_of_range &)
		{ fail("Unexpected exception thrown during normal fixedVector_t<> access"); }

	void registerTests() final override
	{
		CXX_TEST(testTraits)
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
