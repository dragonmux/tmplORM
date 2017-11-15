#include <crunch++.h>
#include <fixedVector.hxx>
#include <testFixedVector.hxx>

std::array<int32_t, 10> testNums =
{
	0, 1, 2, 3, 4,
	5, 6, 7, 8, 9
};

using arrayIter_t = boundedIterator_t<int32_t>;
template<typename T, size_t N> constexpr size_t count(const std::array<T, N> &) noexcept
	{ return N - 1; }

namespace boundedIterator
{
	void testCtor(testsuit &suite)
	{
		arrayIter_t iter{testNums.data(), count(testNums)};
		suite.assertEqual(*iter, 0);
		suite.assertTrue(iter == iter);
		suite.assertFalse(iter == iter + 1);
		suite.assertTrue(iter != iter + 1);
	}

	void testIndex(testsuit &suite)
	{
		arrayIter_t iter{testNums.data(), count(testNums)};
		suite.assertEqual(iter[0], 0);
		suite.assertEqual(iter[10], 9);
		suite.assertEqual(iter[9], 9);
		suite.assertEqual(iter[5], 5);
	}

	void testInc(testsuit &suite)
	{
		arrayIter_t iter{testNums.data(), count(testNums)};
		iter += 10;
		suite.assertEqual(*iter, 9);
		++iter;
		suite.assertEqual(*iter, 9);
		suite.assertTrue(iter == iter++);
		suite.assertTrue(iter == ++iter);
		suite.assertTrue(iter == iter + 1);
		suite.assertTrue(iter == iter + SIZE_MAX);
	}

	void testDec(testsuit &suite)
	{
		arrayIter_t iter{testNums.data(), count(testNums)};
		iter -= 1;
		suite.assertEqual(*iter, 0);
		--iter;
		suite.assertEqual(*iter, 0);
		suite.assertTrue(iter == iter--);
		suite.assertTrue(iter == --iter);
		suite.assertTrue(iter == iter - 1);
		suite.assertTrue(iter == iter - SIZE_MAX);
	}
}

namespace fixedVector
{
	template<typename T, typename E> void testThrowsExcept(testsuit &suite, T &vec, const char *const errorText)
	{
		try
		{
			int value = vec[2];
			(void)value;
			suite.fail("fixedVector_t<> failed to throw exception when expected");
		}
		catch (const E &except)
			{ suite.assertEqual(except.what(), errorText); }
	}

	template<typename T> struct typeOfVector;
	template<typename T> struct typeOfVector<fixedVector_t<T>> { using type = T; };
	template<typename T> struct typeOfVector<const fixedVector_t<T>> { using type = const T; };

	template<typename T, typename value_t = typename typeOfVector<T>::type>
		value_t &index(T &vec, const size_t index) { return vec[index]; }

	void testInvalid(testsuit &suite)
	{
		fixedVector_t<int> vec;
		suite.assertFalse(vec.valid());
		suite.assertFalse(bool(vec));
		suite.assertNull(vec.data());
		suite.assertEqual(vec.length(), 0);
		suite.assertEqual(vec.size(), 0);
		suite.assertEqual(vec.count(), 0);
		testThrowsExcept<fixedVector_t<int>, vectorStateException_t>(suite, vec, "fixedVector_t in invalid state");
		testThrowsExcept<const fixedVector_t<int>, vectorStateException_t>(suite, vec, "fixedVector_t in invalid state");
	}

	void testIndexing(testsuit &suite)
	{
		using fixedVec = fixedVector_t<int>;
		using constFixedVec = const fixedVector_t<int>;

		fixedVec vec(2);
		suite.assertTrue(vec.valid());
		try
		{
			suite.assertEqual(index<fixedVec>(vec, 0), 0);
			suite.assertEqual(index<constFixedVec>(vec, 0), 0);
			index<fixedVec>(vec, 1) = 5;
			suite.assertEqual(index<constFixedVec>(vec, 1), 5);
		}
		catch (const std::out_of_range &)
			{ suite.fail("Unexpected exception thrown during normal fixedVector_t<> access"); }

		testThrowsExcept<fixedVec, std::out_of_range>(suite, vec, "Index into fixedVector_t out of bounds");
		testThrowsExcept<constFixedVec, std::out_of_range>(suite, vec, "Index into fixedVector_t out of bounds");
	}

	void testSwap(testsuit &suite)
	{
		fixedVector_t<int> vecA(2), vecB(3);
		suite.assertTrue(vecA.valid());
		suite.assertTrue(vecB.valid());
		suite.assertEqual(vecA.length(), 2);
		suite.assertEqual(vecB.length(), 3);

		const auto dataA = vecA.data(), dataB = vecB.data();
		swap(vecA, vecB);

		suite.assertNotEqual(vecA.data(), dataA);
		suite.assertEqual(vecA.data(), dataB);
		suite.assertNotEqual(vecB.data(), dataB);
		suite.assertEqual(vecB.data(), dataA);
		suite.assertEqual(vecA.length(), 3);
		suite.assertEqual(vecB.length(), 2);
	}
}
