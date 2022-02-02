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
	constexpr static int32_t jan1st2000{0};
	constexpr static int32_t feb1st2022{8067};

	template<typename T> void checkValue(const T &var, const T &expected)
		{ assertEqual(var, expected); }

	// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
	void checkValue(const ormDate_t &var, const ormDate_t &expected)
	{
		if (var != expected)
			printf("Expected %04d-%02u-%02u, got %04d-%02u-%02u\n",
				expected.year(), expected.month(), expected.day(),
				var.year(), var.month(), var.day()
			);
		assertTrue(var == expected);
	}

	// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
	void checkValue(const ormDateTime_t &var, const ormDateTime_t &expected)
	{
		if (var != expected)
			printf("Expected %04d-%02u-%02uT%02u:%02u:%02u, got %04d-%02u-%02uT%02u:%02u:%02u\n",
				expected.year(), expected.month(), expected.day(), expected.hour(), expected.minute(), expected.second(),
				var.year(), var.month(), var.day(), var.hour(), var.minute(), var.second()
			);
		assertTrue(var == expected);
	}

	// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
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

	void testNull()
	{
		assertTrue(pgSQLValue_t{}.isNull());
		assertTrue(pgSQLValue_t{nullptr}.isNull());
		assertFalse(pgSQLValue_t{"", VARCHAROID}.isNull());
		tryIsNull<uint8_t>({});
		tryIsNull<int8_t>({});
		tryIsNull<uint16_t>({});
		tryIsNull<int16_t>({});
		tryIsNull<uint32_t>({});
		tryIsNull<int32_t>({});
		tryIsNull<uint64_t>({});
		tryIsNull<int64_t>({});
		tryIsNull<float>({});
		tryIsNull<double>({});
		tryIsNull<bool>({});
		tryIsNull<ormDate_t>({});
		tryIsNull<ormDateTime_t>({});
		//tryIsNull<ormUUID_t>({});
	}

	void testUint8()
	{
		std::array<char, 2> intBuffer{};

		tryIsNull<uint8_t>({nullptr, INT2OID});
		tryShouldFail<uint8_t>({"", VARCHAROID});
		tryShouldFail<uint8_t>({"", INT4OID});
		tryShouldFail<uint8_t>({"", INT8OID});
		tryShouldFail<uint8_t>({"", FLOAT4OID});
		tryShouldFail<uint8_t>({"", FLOAT8OID});
		tryShouldFail<uint8_t>({"", BYTEAOID});
		tryShouldFail<uint8_t>({"", BOOLOID});
		tryShouldFail<uint8_t>({"", DATEOID});
		tryShouldFail<uint8_t>({"", TIMESTAMPOID});
		tryShouldFail<uint8_t>({"", UUIDOID});
		substrate::buffer_utils::writeBE<uint16_t>(128, intBuffer.data());
		tryOk<uint8_t>({intBuffer.data(), INT2OID}, 128);
		substrate::buffer_utils::writeBE<uint16_t>(255, intBuffer.data());
		tryOk<uint8_t>({intBuffer.data(), INT2OID}, 255);
		substrate::buffer_utils::writeBE<uint16_t>(0, intBuffer.data());
		tryOk<uint8_t>({intBuffer.data(), INT2OID}, 0);
	}

	void testInt8()
	{
		std::array<char, 2> intBuffer{};

		tryIsNull<int8_t>({nullptr, INT2OID});
		tryShouldFail<int8_t>({"", VARCHAROID});
		tryShouldFail<int8_t>({"", INT4OID});
		tryShouldFail<int8_t>({"", INT8OID});
		tryShouldFail<int8_t>({"", FLOAT4OID});
		tryShouldFail<int8_t>({"", FLOAT8OID});
		tryShouldFail<int8_t>({"", BYTEAOID});
		tryShouldFail<int8_t>({"", BOOLOID});
		tryShouldFail<int8_t>({"", DATEOID});
		tryShouldFail<int8_t>({"", TIMESTAMPOID});
		tryShouldFail<int8_t>({"", UUIDOID});
		substrate::buffer_utils::writeBE<int16_t>(127, intBuffer.data());
		tryOk<int8_t>({intBuffer.data(), INT2OID}, 127);
		substrate::buffer_utils::writeBE<int16_t>(0, intBuffer.data());
		tryOk<int8_t>({intBuffer.data(), INT2OID}, 0);
		substrate::buffer_utils::writeBE<int16_t>(-1, intBuffer.data());
		tryOk<int8_t>({intBuffer.data(), INT2OID}, -1);
		substrate::buffer_utils::writeBE<int16_t>(-127, intBuffer.data());
		tryOk<int8_t>({intBuffer.data(), INT2OID}, -127);
		substrate::buffer_utils::writeBE<int16_t>(-128, intBuffer.data());
		tryOk<int8_t>({intBuffer.data(), INT2OID}, -128);
	}

	void testUint16()
	{
		std::array<char, 2> intBuffer{};

		tryIsNull<uint16_t>({nullptr, INT2OID});
		tryShouldFail<uint16_t>({"", VARCHAROID});
		tryShouldFail<uint16_t>({"", INT4OID});
		tryShouldFail<uint16_t>({"", INT8OID});
		tryShouldFail<uint16_t>({"", FLOAT4OID});
		tryShouldFail<uint16_t>({"", FLOAT8OID});
		tryShouldFail<uint16_t>({"", BYTEAOID});
		tryShouldFail<uint16_t>({"", BOOLOID});
		tryShouldFail<uint16_t>({"", DATEOID});
		tryShouldFail<uint16_t>({"", TIMESTAMPOID});
		tryShouldFail<uint16_t>({"", UUIDOID});
		substrate::buffer_utils::writeBE<uint16_t>(128, intBuffer.data());
		tryOk<uint16_t>({intBuffer.data(), INT2OID}, 128);
		substrate::buffer_utils::writeBE<uint16_t>(255, intBuffer.data());
		tryOk<uint16_t>({intBuffer.data(), INT2OID}, 255);
		substrate::buffer_utils::writeBE<uint16_t>(32768, intBuffer.data());
		tryOk<uint16_t>({intBuffer.data(), INT2OID}, 32768);
		substrate::buffer_utils::writeBE<uint16_t>(65535, intBuffer.data());
		tryOk<uint16_t>({intBuffer.data(), INT2OID}, 65535);
		substrate::buffer_utils::writeBE<uint16_t>(0, intBuffer.data());
		tryOk<uint16_t>({intBuffer.data(), INT2OID}, 0);
	}

	void testInt16()
	{
		std::array<char, 2> intBuffer{};

		tryIsNull<int16_t>({nullptr, INT2OID});
		tryShouldFail<int16_t>({"", VARCHAROID});
		tryShouldFail<int16_t>({"", INT4OID});
		tryShouldFail<int16_t>({"", INT8OID});
		tryShouldFail<int16_t>({"", FLOAT4OID});
		tryShouldFail<int16_t>({"", FLOAT8OID});
		tryShouldFail<int16_t>({"", BYTEAOID});
		tryShouldFail<int16_t>({"", BOOLOID});
		tryShouldFail<int16_t>({"", DATEOID});
		tryShouldFail<int16_t>({"", TIMESTAMPOID});
		tryShouldFail<int16_t>({"", UUIDOID});
		substrate::buffer_utils::writeBE<int16_t>(127, intBuffer.data());
		tryOk<int16_t>({intBuffer.data(), INT2OID}, 127);
		substrate::buffer_utils::writeBE<int16_t>(32767, intBuffer.data());
		tryOk<int16_t>({intBuffer.data(), INT2OID}, 32767);
		substrate::buffer_utils::writeBE<int16_t>(0, intBuffer.data());
		tryOk<int16_t>({intBuffer.data(), INT2OID}, 0);
		substrate::buffer_utils::writeBE<int16_t>(-1, intBuffer.data());
		tryOk<int16_t>({intBuffer.data(), INT2OID}, -1);
		substrate::buffer_utils::writeBE<int16_t>(-32767, intBuffer.data());
		tryOk<int16_t>({intBuffer.data(), INT2OID}, -32767);
		substrate::buffer_utils::writeBE<int16_t>(-32768, intBuffer.data());
		tryOk<int16_t>({intBuffer.data(), INT2OID}, -32768);
	}

	void testDate()
	{
		std::array<char, 4> dateBuffer{};

		tryIsNull<ormDate_t>({nullptr});
		substrate::buffer_utils::writeBE(jan1st2000, dateBuffer.data());
		tryOk<ormDate_t>({dateBuffer.data(), DATEOID}, {2000, 1, 1});
		substrate::buffer_utils::writeBE(feb1st2022, dateBuffer.data());
		tryOk<ormDate_t>({dateBuffer.data(), DATEOID}, {2022, 2, 1});
	}

	void testDateTime()
	{
		constexpr static auto jan1st2000Midday{jan1st2000 * INT64_C(86400000000)};
		std::array<char, 8> dateTimeBuffer{};

		tryIsNull<ormDateTime_t>({nullptr});
		substrate::buffer_utils::writeBE(jan1st2000Midday, dateTimeBuffer.data());
		tryOk<ormDateTime_t>({dateTimeBuffer.data(), TIMESTAMPOID}, {2000, 1, 1, 12, 0, 0, 0});
	}

public:
	void registerTests() final
	{
		CXX_TEST(testNull)
		CXX_TEST(testUint8)
		CXX_TEST(testInt8)
		CXX_TEST(testUint16)
		CXX_TEST(testInt16)
		CXX_TEST(testDate)
		CXX_TEST(testDateTime)
	}
};

CRUNCHpp_TESTS(testPgSQLValue_t);
