#include <chrono>
// AAAAAAAAGGGGHHH.. this should be in the libpq headers, but no distro puts it where they should.
#include <catalog/pg_type_d.h>
#include <substrate/buffer_utils>
#include <crunch++.h>
#include "pgsql.hxx"
#include "tmplORM.pgsql.hxx"

/*!
 * @internal
 * @file
 * @author Rachel Mant
 * @date 2022
 * @brief Unit tests for the PgSQL driver abstraction layer
 */

using namespace tmplORM::pgsql::driver;
using tmplORM::types::baseTypes::ormDateTime_t;

using systemClock_t = std::chrono::system_clock;

static ormDateTime_t now = systemClock_t::now();

class testPgSQLValue_t final : public testsuite
{
private:
	template<typename T> void checkValue(const T &var, const T &expected)
		{ assertEqual(var, expected); }
	void checkValue(const ormDate_t &var, const ormDate_t &expected)
	{
		if (var != expected)
			printf("Expected %04d-%02u-%02u, got %04d-%02u-%02u\n",
				expected.year(), expected.month(), expected.day(),
				var.year(), var.month(), var.day()
			);
		assertTrue(var == expected);
	}
	void checkValue(const ormDateTime_t &var, const ormDateTime_t &expected)
		{ assertTrue(var == expected); }
	void checkValue(const ormUUID_t &var, const ormUUID_t &expected)
		{ assertTrue(var == expected); }

	template<typename T> T tryOkConversion(const pgSQLValue_t &value)
	{
		try { return T(value); }
		catch (const pgSQLValueError_t &error)
			{ fail(error.error()); }
		return {};
	}

	template<typename T> void tryFailConversion(const pgSQLValue_t &value)
	{
		try
		{
			T v(value);
			printf("Conversion for %s - ", typeid(T).name());
			fail("Conversion succeeded (expected to fail)");
		}
		catch (const pgSQLValueError_t &) { }
	}

	template<typename T> void tryOk(const pgSQLValue_t &value, const T &expected)
	{
		assertFalse(value.isNull());
		T var = tryOkConversion<T>(value);
		checkValue(var, expected);
	}

	template<typename T> void tryIsNull(const pgSQLValue_t &value)
	{
		assertTrue(value.isNull());
		tryFailConversion<T>(value);
	}

	template<typename T> void tryShouldFail(const pgSQLValue_t &value)
	{
		assertFalse(value.isNull());
		tryFailConversion<T>(value);
	}

	void testDate()
	{
		constexpr static int32_t jan1st2000{2451545};
		constexpr static int32_t feb1st2022{2459612};
		std::array<char, 4> dateBuffer{};

		tryIsNull<ormDate_t>({nullptr});
		substrate::buffer_utils::writeBE(jan1st2000, dateBuffer.data());
		tryOk<ormDate_t>({dateBuffer.data(), DATEOID}, {2000, 1, 1});
		substrate::buffer_utils::writeBE(feb1st2022, dateBuffer.data());
		tryOk<ormDate_t>({dateBuffer.data(), DATEOID}, {2022, 2, 1});
	}

public:
	void registerTests() final
	{
		CXX_TEST(testDate)
	}
};

CRUNCHpp_TESTS(testPgSQLValue_t);
