#include <chrono>
// AAAAAAAAGGGGHHH.. this should be in the libpq headers, but no distro puts it where they should.
#include <catalog/pg_type_d.h>
#include <substrate/buffer_utils>
#include <crunch++.h>
#include "pgsql.hxx"
#include "tmplORM.pgsql.hxx"
#include "constString.hxx"

/*!
 * @internal
 * @file
 * @author Rachel Mant
 * @date 2022
 * @brief Unit tests for the PgSQL driver abstraction layer
 */

using namespace tmplORM::pgsql::driver;
using irqus::typestring;
using tmplORM::pgsql::fieldLength;
using tmplORM::types::baseTypes::ormDateTime_t;

using systemClock_t = std::chrono::system_clock;
#define u64(n)		UINT64_C(n)
#define i64(n)		INT64_C(n)

static ormDateTime_t now = systemClock_t::now();
constexpr static int64_t usPerDay{i64(86400000000)};
constexpr static int64_t usPerHour{i64(3600000000)};

struct data_t final
{
	tmplORM::types::int32_t<typestring<>> entryID;
	tmplORM::types::unicode_t<typestring<>, 50> name;
	tmplORM::types::nullable_t<tmplORM::types::int32_t<typestring<>>> value;
	tmplORM::types::dateTime_t<typestring<>> when;
};

struct type_t final
{
	tmplORM::types::int32_t<typestring<>> entryID;
	tmplORM::types::int64_t<typestring<>> int64;
	tmplORM::types::int32_t<typestring<>> int32;
	tmplORM::types::int16_t<typestring<>> int16;
	tmplORM::types::bool_t<typestring<>> boolean;
	tmplORM::types::unicode_t<typestring<>, 50> string;
	tmplORM::types::unicodeText_t<typestring<>> text;
	tmplORM::types::float_t<typestring<>> decimalF;
	tmplORM::types::double_t<typestring<>> decimalD;
	tmplORM::types::date_t<typestring<>> date;
	tmplORM::types::dateTime_t<typestring<>> dateTime;
	tmplORM::types::uuid_t<typestring<>> uuid;
};

static std::array<data_t, 2> testData
{{
	{0, "kevin", 50, {}},
	{0, "dave", nullptr, {}}
}};

static type_t typeData
{
	0, i64(9223372036854775807), 2147483647,
	32767, true, "This is a string", "This is some text",
	2.125, 5.325, ormDate_t{2018, 07, 04},
	ormDateTime_t{2018, 07, 04, 12, 34, 56, 789012345},
	ormUUID_t{0xbf052777, 0x89b7, 0x4ed6, 0xbc04, 0x4e36e9d63287}
};

class testPgSQL_t final : public testsuite
{
	constString_t host{}, username{}, password{};
	constexpr static const char *port = "5432";
	pgSQLClient_t client{};

	bool haveEnvironment() noexcept
	{
		host = getenv("PGSQL_HOST");
		// port?
		username = getenv("PGSQL_USERNAME");
		password = getenv("PGSQL_PASSWORD");
		return !(host.empty() || username.empty() || password.empty());
	}

	static void printError(const pgSQLResult_t &result) noexcept
	{
		const auto errorNum{result.errorNum()};
		const auto error{result.error()};
		printf("Query failed (%u): %s\n", errorNum, error);
	}

	static void printError(const pgSQLClient_t &client, const char *const prefix) noexcept
		{ printf("%s failed: %s\n", prefix, client.error()); }

	void testInvalid()
	{
		pgSQLClient_t testClient{};
		assertFalse(testClient.valid());
		assertFalse(testClient.switchDB(nullptr));
		assertFalse(testClient.query("").valid());
		assertFalse(testClient.prepare("", 0).valid());
		assertFalse(testClient.beginTransact());
		assertTrue(testClient.commit());
		assertTrue(testClient.rollback());
		assertNull(testClient.error());
		pgSQLQuery_t testQuery{};
		assertFalse(testQuery.valid());
		assertFalse(testQuery.execute().valid());
		pgSQLResult_t testResult{};
		assertFalse(testResult.valid());
		assertEqual(testResult.errorNum(), static_cast<uint32_t>(PGRES_COMMAND_OK));
		assertNull(testResult.error());
		assertTrue(testResult.successful());
		assertFalse(testResult.hasData());
		assertEqual(testResult.numRows(), 0);
		assertFalse(testResult.next());
		assertFalse(testResult[0].valid());
		pgSQLValue_t testValue{};
		assertFalse(testValue.valid());
		assertTrue(testValue.isNull());
	}

	void testConnect()
	{
		assertFalse(client.valid());
		assertFalse(client.connect(nullptr, nullptr, nullptr, nullptr, nullptr));
		assertFalse(client.valid());
		assertTrue(client.connect(host, port, username, password, "postgres"));
		assertTrue(client.valid());
		assertFalse(client.connect(nullptr, nullptr, nullptr, nullptr, nullptr));
		assertTrue(client.valid());
	}

	void testCreateDB()
	{
		assertTrue(client.valid());
		const auto result{client.query(R"(
			CREATE DATABASE "tmplORM" WITH
				OWNER postgres
				TEMPLATE template0
				ENCODING 'UTF8'
				LOCALE 'C'
			;)"
		)};
		assertTrue(result.valid());
		if (!result.successful())
			printError(result);
		assertTrue(result.successful());
		assertNotNull(result.error());
		assertEqual(result.numFields(), 0);
		assertEqual(result.numRows(), 0);
		assertFalse(result[0].valid());
		assertTrue(client.switchDB("tmplORM"));
		assertTrue(client.valid());
	}

	void testSwitchDB()
	{
		pgSQLClient_t testClient{};
		assertFalse(testClient.valid());
		assertTrue(testClient.connect(host, port, username, password, "postgres"));
		assertTrue(testClient.switchDB(nullptr));
		assertTrue(testClient.valid());
	}

	void testCreateTable()
	{
		assertTrue(client.valid());
		pgSQLResult_t result{};
		assertFalse(result.valid());

		result = client.query(R"(
			CREATE TABLE "tmplORM"
			(
				"EntryID" INT4 PRIMARY KEY GENERATED BY DEFAULT AS IDENTITY NOT NULL,
				"Name" VARCHAR(50) NOT NULL,
				"Value" INT4 NULL,
				"When" TIMESTAMP NOT NULL DEFAULT CLOCK_TIMESTAMP()
			);)"
		);
		assertTrue(result.valid());
		if (!result.successful())
			printError(result);
		assertTrue(result.successful());

		// TINYINT/INT1 is not a thing on Postgres, so we don't bother with that here.
		result = client.query(R"(
			CREATE TABLE "TypeTest"
			(
				"EntryID" INT4 PRIMARY KEY GENERATED BY DEFAULT AS IDENTITY NOT NULL,
				"Int64" INT8 NOT NULL,
				"Int32" INT4 NOT NULL,
				"Int16" INT2 NOT NULL,
				"Bool" BOOLEAN NOT NULL,
				"String" VARCHAR(50) NOT NULL,
				"Text" TEXT NOT NULL,
				"Float" FLOAT4 NOT NULL,
				"Double" FLOAT8 NOT NULL,
				"Date" DATE NOT NULL,
				"DateTime" TIMESTAMP NOT NULL,
				"UUID" UUID NOT NULL
			);)"
		);
		assertTrue(result.valid());
		if (!result.successful())
			printError(result);
		assertTrue(result.successful());
	}

	void testPrepared() try
	{
		assertTrue(client.valid());
		pgSQLQuery_t query{};
		assertFalse(query.valid());
		pgSQLResult_t result{};
		assertFalse(result.valid());

		query = client.prepare(R"(
			INSERT INTO "tmplORM" ("Name", "Value")
			VALUES ($1, $2) RETURNING "EntryID";)", 2
		);
		assertTrue(query.valid());
		query.bind(0, testData[0].name.value(), fieldLength(testData[0].name));
		query.bind(1, testData[0].value.value(), fieldLength(testData[0].value));
		result = query.execute();
		assertTrue(result.valid());
		if (!result.successful())
			printError(result);
		assertTrue(result.successful());

		assertTrue(result.hasData());
		assertEqual(result.numRows(), 1);
		assertEqual(result.numFields(), 1);
		assertTrue(result[0].valid());
		assertFalse(result[0].isNull());
		testData[0].entryID = result[0];
		assertEqual(testData[0].entryID, 1);
		assertFalse(result.next());

		testData[1].when = now;
		query = client.prepare(R"(
			INSERT INTO "tmplORM" ("Name", "Value", "When")
			VALUES ($1, $2, $3) RETURNING "EntryID";)", 3
		);
		assertTrue(query.valid());
		query.bind(0, testData[1].name.value(), fieldLength(testData[1].name));
		query.bind<int32_t>(1, nullptr, fieldLength(testData[1].value));
		query.bind(2, testData[1].when.value(), fieldLength(testData[1].when));
		result = query.execute();
		assertTrue(result.valid());
		if (!result.successful())
			printError(result);
		assertTrue(result.successful());

		assertTrue(result.hasData());
		assertEqual(result.numRows(), 1);
		assertEqual(result.numFields(), 1);
		assertTrue(result[0].valid());
		assertFalse(result[0].isNull());
		testData[1].entryID = result[0];
		assertEqual(testData[1].entryID, 2);
		assertFalse(result.next());
	}
	catch (const pgSQLValueError_t &error)
	{
		puts(error.error());
		fail("Exception throw while converting value");
	}

	void testResult() try
	{
		assertTrue(client.valid());
		auto result{client.query(R"(SELECT "EntryID", "Name", "Value", "When" FROM "tmplORM";)")};
		assertTrue(result.valid());
		if (!result.successful())
			printError(result);
		assertTrue(result.successful());
		assertTrue(result.hasData());
		assertEqual(result.numRows(), 2);
		assertEqual(result.numFields(), 4);

		assertEqual(result[0], testData[0].entryID);
		assertEqual(result[1].asString(), testData[0].name);
		assertFalse(result[2].isNull());
		assertEqual(result[2], testData[0].value);
		const auto timestamp{result[3]};
		testData[0].when = timestamp;
		const auto when{testData[0].when.value()};
		assertTrue(result.next());

		assertEqual(result[0], testData[1].entryID);
		assertEqual(result[1].asString(), testData[1].name);
		assertTrue(result[2].isNull());
		// Apply Postgres rounding..
		now.nanoSecond((now.nanoSecond() / 1000U) * 1000U);
		if (result[3].asDateTime() != now)
		{
			const auto nowStr{now.asString()};
			const auto resStr{result[3].asDateTime().asString()};
			printf("Got %s, expected %s\n", resStr.get(), nowStr.get());
		}
		assertTrue(result[3].asDateTime() == now);
		assertFalse(result.next());
	}
	catch (const pgSQLValueError_t &error)
	{
		puts(error.error());
		fail("Exception thrown while converting value");
	}

	void testTransact()
	{
		assertTrue(client.valid());
		pgSQLResult_t result{};
		assertFalse(result.valid());
		const auto started{client.beginTransact()};
		if (!started)
			printError(client, "Start transaction");
		assertTrue(started);
		assertFalse(client.beginTransact());

		result = client.query(R"(UPDATE "tmplORM" SET "Name" = 'Karl' WHERE "EntryID" = '1';)");
		assertTrue(result.valid());
		if (!result.successful())
		{
			printError(result);
			client.rollback();
		}
		assertTrue(result.successful());
		assertFalse(result.hasData());
		assertEqual(result.numRows(), 0);
		assertEqual(result.numFields(), 0);

		const bool rolledBack{client.rollback()};
		if (!rolledBack)
			printError(client, "Abort transaction");
		assertTrue(rolledBack);

		result = client.query(R"(SELECT "Name" FROM "tmplORM" WHERE "EntryID" = '1';)");
		assertTrue(result.valid());
		if (!result.successful())
			printError(result);
		assertTrue(result.successful());
		assertTrue(result.hasData());
		assertEqual(result.numRows(), 1);
		assertEqual(result.numFields(), 1);
		assertEqual(result[0].asString(), testData[0].name);
		assertFalse(result.next());

		const auto restarted{client.beginTransact()};
		if (!restarted)
			printError(client, "Start transaction");
		assertTrue(restarted);

		result = client.query(R"(UPDATE "tmplORM" SET "Name" = 'Karl' WHERE "EntryID" = '1';)");
		assertTrue(result.valid());
		if (!result.successful())
		{
			printError(result);
			client.rollback();
		}
		assertTrue(result.successful());
		assertFalse(result.hasData());
		assertEqual(result.numRows(), 0);
		assertEqual(result.numFields(), 0);

		const auto committed{client.commit()};
		if (!committed)
			printError(client, "Commit transaction");
		assertTrue(committed);

		result = client.query(R"(SELECT "Name" FROM "tmplORM" WHERE "EntryID" = '1';)");
		assertTrue(result.valid());
		if (!result.successful())
			printError(result);
		assertTrue(result.successful());
		assertTrue(result.hasData());
		assertEqual(result.numRows(), 1);
		assertEqual(result.numFields(), 1);
		assertEqual(result[0].asString(), "Karl");
		assertFalse(result.next());
	}

	void testBind() try
	{
		assertTrue(client.valid());
		pgSQLResult_t result{};
		assertFalse(result.valid());

		auto query
		{
			client.prepare(R"(
				INSERT INTO "TypeTest"
				(
					"Int64", "Int32", "Int16", "Bool", "String", "Text",
					"Float", "Double", "Date", "DateTime", "UUID"
				)
				VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11)
				RETURNING "EntryID";
			)", 11)
		};
		assertTrue(query.valid());
		query.bind(0, typeData.int64.value(), fieldLength(typeData.int64));
		query.bind(1, typeData.int32.value(), fieldLength(typeData.int32));
		query.bind(2, typeData.int16.value(), fieldLength(typeData.int16));
		query.bind(3, typeData.boolean.value(), fieldLength(typeData.boolean));
		query.bind(4, typeData.string.value(), fieldLength(typeData.string));
		query.bind(5, typeData.text.value(), fieldLength(typeData.text));
		query.bind(6, typeData.decimalF.value(), fieldLength(typeData.decimalF));
		query.bind(7, typeData.decimalD.value(), fieldLength(typeData.decimalD));
		query.bind(8, typeData.date.value(), fieldLength(typeData.date));
		query.bind(9, typeData.dateTime.value(), fieldLength(typeData.dateTime));
		query.bind(10, typeData.uuid.value(), fieldLength(typeData.uuid));
		result = query.execute();
		assertTrue(result.valid());
		if (!result.successful())
			printError(result);
		assertTrue(result.successful());
	}
	catch (const pgSQLValueError_t &error)
	{
		puts(error.error());
		fail("Exception thrown while converting value");
	}

	void testDestroyDB()
	{
		assertTrue(client.valid());
		client = {};
		assertFalse(client.valid());
		assertTrue(client.connect(host, port, username, password, "postgres"));
		assertTrue(client.valid());
		const auto result{client.query(R"(DROP DATABASE "tmplORM";)")};
		assertTrue(result.valid());
		if (!result.successful())
			printError(result);
		assertTrue(result.successful());
	}

	void testDisconnect()
	{
		if (client.valid())
		{
			assertTrue(client.beginTransact());
			client.disconnect();
		}
		assertFalse(client.valid());
	}

public:
	void registerTests() final
	{
		if (!haveEnvironment())
			skip("No suitable environment found, running only invalidity test");
		CXX_TEST(testInvalid)
		CXX_TEST(testConnect)
		CXX_TEST(testCreateDB)
		CXX_TEST(testSwitchDB)
		CXX_TEST(testCreateTable)
		CXX_TEST(testPrepared)
		CXX_TEST(testResult)
		CXX_TEST(testTransact)
		CXX_TEST(testBind)
		CXX_TEST(testDestroyDB)
		CXX_TEST(testDisconnect)
	}
};

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
		constexpr static auto jan1st2000Midday{jan1st2000 * usPerDay + (12 * usPerHour)};
		std::array<char, 8> dateTimeBuffer{};

		tryIsNull<ormDateTime_t>({nullptr});
		substrate::buffer_utils::writeBE(jan1st2000Midday, dateTimeBuffer.data());
		tryOk<ormDateTime_t>({dateTimeBuffer.data(), TIMESTAMPOID}, {2000, 1, 1, 12, 0, 0, 0});
	}

public:
	void registerTests() final
	{
		CXX_TEST(testNull)
		//CXX_TEST(testString)
		CXX_TEST(testUint8)
		CXX_TEST(testInt8)
		CXX_TEST(testUint16)
		CXX_TEST(testInt16)
		CXX_TEST(testDate)
		CXX_TEST(testDateTime)
	}
};

CRUNCHpp_TESTS(testPgSQL_t, testPgSQLValue_t);
