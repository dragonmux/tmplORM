#include <memory>
#include <string>
#include <chrono>
#include <type_traits>
#include <substrate/utility>
#include <crunch++.h>
#include <mssql.hxx>
#include <tmplORM.mssql.hxx>
#include "constString.hxx"

/*!
 * @internal
 * @file
 * @author Rachel Mant
 * @date 2017-2020
 * @brief Unit tests for the MSSQL driver abstraction layer
 */

using namespace tmplORM::mssql::driver;
using irqus::typestring;
using tmplORM::mssql::fieldLength;
using tmplORM::types::baseTypes::ormDateTime_t;

using systemClock_t = std::chrono::system_clock;
#define u64(n)		UINT64_C(n)
#define i64(n)		INT64_C(n)
using std::chrono::milliseconds;
using tmplORM::types::chrono::durationIn;

std::unique_ptr<tSQLClient_t> testClient{};
constString_t driver, host, username, password;

constexpr static uint32_t port = 1433;
ormDateTime_t now = systemClock_t::now();

struct data_t
{
	tmplORM::types::int32_t<typestring<>> entryID;
	tmplORM::types::unicode_t<typestring<>, 50> name;
	tmplORM::types::nullable_t<tmplORM::types::int32_t<typestring<>>> value;
	tmplORM::types::dateTime_t<typestring<>> when;
};

struct type_t
{
	tmplORM::types::int32_t<typestring<>> entryID;
	tmplORM::types::int64_t<typestring<>> int64;
	tmplORM::types::int32_t<typestring<>> int32;
	tmplORM::types::int16_t<typestring<>> int16;
	tmplORM::types::int8_t<typestring<>> int8;
	tmplORM::types::bool_t<typestring<>> boolean;
	tmplORM::types::unicode_t<typestring<>, 50> string;
	tmplORM::types::unicodeText_t<typestring<>> text;
	tmplORM::types::float_t<typestring<>> decimalF;
	tmplORM::types::double_t<typestring<>> decimalD;
	tmplORM::types::date_t<typestring<>> date;
	tmplORM::types::dateTime_t<typestring<>> dateTime;
	tmplORM::types::uuid_t<typestring<>> uuid;
};

std::array<data_t, 2> testData
{
	data_t{0, "kevin", 50, {}},
	data_t{0, "dave", nullptr, {}}
};

type_t typeData
{
	0, i64(9223372036854775807), 2147483647,
	32767, 127, true, "This is a string",
	"This is some text", 2.125, 5.325, ormDate_t{2018, 07, 04},
	ormDateTime_t{2018, 07, 04, 12, 34, 56, 789012345},
	ormUUID_t{}
};

bool haveEnvironment() noexcept
{
	driver = getenv("MSSQL_DRIVER");
	host = getenv("MSSQL_HOST");
	// port?
	username = getenv("MSSQL_USERNAME");
	password = getenv("MSSQL_PASSWORD");
	return !(driver.empty() || host.empty() || username.empty() || password.empty());
}

class testMSSQL_t final : public testsuite
{
private:
	void printError(const char *prefix, const tSQLExecError_t &error) const noexcept
	{
		const auto errorNum = uint8_t(error.errorNum());
		printf("%s failed (%u): %s\n", prefix, errorNum, error.error());
		printf("\tserver message: %s\n", error.message());
		printf("\tstate code: %s\n", error.state());
	}

	void testInvalid()
	{
		tSQLClient_t testClient;
		assertFalse(testClient.valid());
		assertFalse(testClient.query("").valid());
		assertFalse(testClient.prepare("", 0).valid());
		tSQLQuery_t testQuery;
		assertFalse(testQuery.valid());
		assertFalse(testQuery.execute().valid());
		tSQLResult_t testResult;
		assertFalse(testResult.valid());
		assertEqual(testResult.numRows(), 0);
		assertFalse(testResult.next());
		assertTrue(testResult[0].isNull());
		tSQLValue_t testValue;
		assertTrue(testValue.isNull());
	}

	void testConnect()
	{
		assertNull(testClient);
		tSQLClient_t client;
		assertFalse(client.valid());
		const bool selected = client.selectDB("master");
		if (!selected)
			printError("DB selection", client.error());
		assertFalse(client.connect(driver, host, port, username, ""));
		const bool connected = client.connect(driver, host, port, username, password);
		if (!connected)
			printError("Connection", client.error());
		assertTrue(connected);
		assertTrue(client.error() == tSQLExecErrorType_t::ok);
		assertTrue(client.valid());

		testClient = substrate::make_unique_nothrow<tSQLClient_t>();
		assertNotNull(testClient);
		assertFalse(testClient->valid());
		*testClient = std::move(client);
		assertFalse(client.valid());
		assertTrue(testClient->valid());

		assertFalse(testClient->connect(driver, host, port, username, password));
		assertTrue(testClient->error() == tSQLExecErrorType_t::connect);
		assertFalse(client.beginTransact());
	}

	void testCreateDB()
	{
		assertNotNull(testClient);
		assertTrue(testClient->valid());
		tSQLResult_t result = testClient->query("CREATE DATABASE [tmplORM] COLLATE latin1_general_100_CI_AI_SC;");
		if (!result.valid())
			printError("Query", testClient->error());
		assertTrue(result.valid());
		assertTrue(testClient->error() == tSQLExecErrorType_t::ok);
	}

	void testSelectDB()
	{
		assertNotNull(testClient);
		assertTrue(testClient->valid());
		const bool selected = testClient->selectDB("tmplORM");
		if (!selected)
			printError("DB selection", testClient->error());
		assertTrue(selected);
		assertTrue(testClient->error() == tSQLExecErrorType_t::ok);
		assertFalse(testClient->selectDB(nullptr));
	}

	void testCreateTable()
	{
		assertNotNull(testClient);
		assertTrue(testClient->valid());
		tSQLResult_t result;
		assertFalse(result.valid());

		result = testClient->query(
			"CREATE TABLE [tmplORM] ("
			"[EntryID] INT NOT NULL PRIMARY KEY IDENTITY, "
			"[Name] NVARCHAR(50) NOT NULL, "
			"[Value] INT NULL, "
			"[When] DATETIME2 NOT NULL DEFAULT CURRENT_TIMESTAMP);"
		);
		if (!result.valid())
			printError("Query", testClient->error());
		assertTrue(result.valid());
		assertTrue(testClient->error() == tSQLExecErrorType_t::ok);

		result = testClient->query(
			"CREATE TABLE [TypeTest] ("
			"[EntryID] INT NOT NULL PRIMARY KEY IDENTITY, "
			"[Int64] BIGINT NOT NULL, "
			"[Int32] INT NOT NULL, "
			"[Int16] SMALLINT NOT NULL, "
			"[Int8] TINYINT NOT NULL, "
			"[Bool] BIT NOT NULL, "
			"[String] NVARCHAR(50) NOT NULL, "
			"[Text] NVARCHAR(MAX) NOT NULL, "
			"[Float] REAL NOT NULL, "
			"[Double] FLOAT NOT NULL, "
			"[Date] DATE NOT NULL, "
			"[DateTime] DATETIME2 NOT NULL, "
			"[UUID] UNIQUEIDENTIFIER NOT NULL);"
		);
		if (!result.valid())
			printError("Query", testClient->error());
		assertTrue(result.valid());
		assertTrue(testClient->error() == tSQLExecErrorType_t::ok);
	}

	void testPrepared() try
	{
		assertNotNull(testClient);
		assertTrue(testClient->valid());
		tSQLQuery_t query;
		assertFalse(query.valid());
		tSQLResult_t result;
		assertFalse(result.valid());

		query = testClient->prepare(
			"INSERT INTO [tmplORM] ([Name], [Value]) "
			"OUTPUT INSERTED.[EntryID] VALUES (?, ?);", 2
		);
		assertTrue(query.valid());
		query.bind(0, testData[0].name.value(), fieldLength(testData[0].name));
		assertTrue(testClient->error() == tSQLExecErrorType_t::ok);
		query.bind(1, testData[0].value.value(), fieldLength(testData[0].value));
		assertTrue(testClient->error() == tSQLExecErrorType_t::ok);
		result = query.execute();
		if (testClient->error() != tSQLExecErrorType_t::ok)
			printError("Prepared exec", testClient->error());
		assertTrue(result.valid());

		assertTrue(result.hasData());
		assertEqual(result.numRows(), 0);
		assertEqual(result.numFields(), 1);
		assertFalse(result[0].isNull());
		testData[0].entryID = result[0];
		assertEqual(testData[0].entryID, 1);
		assertFalse(result.next());

		testData[1].when = now;
		query = testClient->prepare(
			"INSERT INTO [tmplORM] ([Name], [Value], [When]) "
			"OUTPUT INSERTED.[EntryID] VALUES (?, ?, ?);", 3
		);
		assertTrue(query.valid());
		query.bind(0, testData[1].name.value(), fieldLength(testData[1].name));
		assertTrue(testClient->error() == tSQLExecErrorType_t::ok);
		query.bind<const char *>(1, nullptr, fieldLength(testData[1].value));
		assertTrue(testClient->error() == tSQLExecErrorType_t::ok);
		query.bind(2, testData[1].when.value(), fieldLength(testData[1].when));
		assertTrue(testClient->error() == tSQLExecErrorType_t::ok);
		result = query.execute();
		if (testClient->error() != tSQLExecErrorType_t::ok)
			printError("Prepared exec", testClient->error());
		assertTrue(result.valid());

		assertTrue(result.hasData());
		assertEqual(result.numRows(), 0);
		assertEqual(result.numFields(), 1);
		assertFalse(result[0].isNull());
		testData[1].entryID = result[0];
		assertEqual(testData[1].entryID, 2);
		assertFalse(result.next());
	}
	catch (const tSQLValueError_t &error)
	{
		puts(error.error());
		fail("Exception thrown while converting value");
	}

	void testResult() try
	{
		assertNotNull(testClient);
		assertTrue(testClient->valid());
		tSQLResult_t result = testClient->query("SELECT [EntryID], [Name], [Value], [When] FROM [tmplORM];");
		if (testClient->error() != tSQLExecErrorType_t::ok)
			printError("Query", testClient->error());
		assertTrue(result.valid());
		assertEqual(result.numRows(), 0);
		assertEqual(result.numFields(), 4);

		assertEqual(result[0], testData[0].entryID);
		assertEqual(result[1].asString(false).get(), testData[0].name);
		assertFalse(result[2].isNull());
		assertEqual(result[2], testData[0].value);
		testData[0].when = result[3];
		auto when = testData[0].when.value();
		assertNotEqual(when.year(), 0);
		assertNotEqual(when.month(), 0);
		assertNotEqual(when.day(), 0);
		assertTrue(result.next());

		assertEqual(result[0], testData[1].entryID);
		assertEqual(result[1].asString().get(), testData[1].name);
		assertTrue(result[2].isNull());
		// Apply MSSQL rounding..
		now.nanoSecond((now.nanoSecond() / 100) * 100);
		assertTrue(result[3].asDateTime() == now);
		assertFalse(result.next());
	}
	catch (const tSQLValueError_t &error)
	{
		puts(error.error());
		fail("Exception thrown while converting value");
	}

	void testTransact()
	{
		assertNotNull(testClient);
		assertTrue(testClient->valid());
		tSQLResult_t result;
		assertFalse(result.valid());
		const bool started = testClient->beginTransact();
		if (!started)
			printError("Start transaction", testClient->error());
		assertTrue(started);
		assertTrue(testClient->error() == tSQLExecErrorType_t::ok);
		assertFalse(testClient->beginTransact());

		result = testClient->query("UPDATE [tmplORM] SET [Name] = 'Karl' WHERE [EntryID] = '1';");
		if (!result.valid())
		{
			printError("Query", testClient->error());
			testClient->rollback();
		}
		assertTrue(result.valid());
		assertTrue(testClient->error() == tSQLExecErrorType_t::ok);
		assertEqual(result.numFields(), 0);
		assertEqual(result.numRows(), 1);

		const bool rolledBack = testClient->rollback();
		if (!rolledBack)
			printError("Abort transaction", testClient->error());
		assertTrue(rolledBack);
		assertTrue(testClient->error() == tSQLExecErrorType_t::ok);

		result = testClient->query("SELECT [Name] FROM [tmplORM] WHERE [EntryID] = '1';");
		if (testClient->error() != tSQLExecErrorType_t::ok)
			printError("Query", testClient->error());
		assertTrue(result.valid());
		assertEqual(result.numFields(), 1);
		assertEqual(result.numRows(), 0);
		assertEqual(result[0].asString(false).get(), testData[0].name);
		assertFalse(result.next());

		const bool restarted = testClient->beginTransact();
		if (!restarted)
			printError("Start transaction", testClient->error());
		assertTrue(restarted);
		assertTrue(testClient->error() == tSQLExecErrorType_t::ok);

		result = testClient->query("UPDATE [tmplORM] SET [Name] = 'Karl' WHERE [EntryID] = '1';");
		if (!result.valid())
		{
			printError("Query", testClient->error());
			testClient->rollback();
		}
		assertTrue(result.valid());
		assertTrue(testClient->error() == tSQLExecErrorType_t::ok);
		assertEqual(result.numFields(), 0);
		assertEqual(result.numRows(), 1);

		const bool committed = testClient->commit();
		if (!committed)
			printError("Commit transaction", testClient->error());
		assertTrue(committed);

		result = testClient->query("SELECT [Name] FROM [tmplORM] WHERE [EntryID] = '1';");
		if (testClient->error() != tSQLExecErrorType_t::ok)
			printError("Query", testClient->error());
		assertTrue(result.valid());
		assertEqual(result.numFields(), 1);
		assertEqual(result.numRows(), 0);
		assertNotEqual(result[0].asString(false).get(), testData[0].name);
		assertEqual(result[0].asString(false).get(), "Karl");
		assertFalse(result.next());
	}

	void testBind() try
	{
		assertNotNull(testClient);
		assertTrue(testClient->valid());
		tSQLResult_t result;
		assertFalse(result.valid());

		// Set up our UUID value.
		const auto now = systemClock_t::now().time_since_epoch();
		const uint64_t time = durationIn<milliseconds>(now);
		const auto nanoSeconds = (now - milliseconds{time}).count();
		typeData.uuid.value(ormUUID_t{uint32_t(time), uint16_t(time >> 32),
			uint16_t(0x1000 | ((time >> 48) & 0x0FFF)),
			uint16_t((nanoSeconds >> 14) | 0x8000), swapBytes(uint64_t{0x123456789ABCU}) >> 16});

		tSQLQuery_t query = testClient->prepare(
			"INSERT INTO [TypeTest] ([Int64], [Int32], [Int16], [Int8], "
			"[Bool], [String], [Text], [Float], [Double], [Date], "
			"[DateTime], [UUID]) OUTPUT INSERTED.[EntryID] "
			"VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);", 12
		);
		assertTrue(query.valid());
		query.bind(0, typeData.int64.value(), fieldLength(typeData.int64));
		assertTrue(testClient->error() == tSQLExecErrorType_t::ok);
		query.bind(1, typeData.int32.value(), fieldLength(typeData.int32));
		assertTrue(testClient->error() == tSQLExecErrorType_t::ok);
		query.bind(2, typeData.int16.value(), fieldLength(typeData.int16));
		assertTrue(testClient->error() == tSQLExecErrorType_t::ok);
		query.bind(3, typeData.int8.value(), fieldLength(typeData.int8));
		assertTrue(testClient->error() == tSQLExecErrorType_t::ok);
		query.bind(4, typeData.boolean.value(), fieldLength(typeData.boolean));
		assertTrue(testClient->error() == tSQLExecErrorType_t::ok);
		query.bind(5, typeData.string.value(), fieldLength(typeData.string));
		assertTrue(testClient->error() == tSQLExecErrorType_t::ok);
		query.bind(6, typeData.text.value(), fieldLength(typeData.text));
		assertTrue(testClient->error() == tSQLExecErrorType_t::ok);
		query.bind(7, typeData.decimalF.value(), fieldLength(typeData.decimalF));
		assertTrue(testClient->error() == tSQLExecErrorType_t::ok);
		query.bind(8, typeData.decimalD.value(), fieldLength(typeData.decimalD));
		assertTrue(testClient->error() == tSQLExecErrorType_t::ok);
		query.bind(9, typeData.date.value(), fieldLength(typeData.date));
		assertTrue(testClient->error() == tSQLExecErrorType_t::ok);
		query.bind(10, typeData.dateTime.value(), fieldLength(typeData.dateTime));
		assertTrue(testClient->error() == tSQLExecErrorType_t::ok);
		query.bind(11, typeData.uuid.value(), fieldLength(typeData.uuid));
		assertTrue(testClient->error() == tSQLExecErrorType_t::ok);
		result = query.execute();
		if (testClient->error() != tSQLExecErrorType_t::ok)
			printError("Prepared exec", testClient->error());
		assertTrue(result.valid());

		assertTrue(result.hasData());
		assertEqual(result.numRows(), 0);
		assertEqual(result.numFields(), 1);
		assertFalse(result[0].isNull());
		typeData.entryID = result[0];
		assertEqual(typeData.entryID, 1);
		assertFalse(result.next());

		result = testClient->query("SELECT [EntryID], [Int64], [Int32], [Int16], [Int8], "
			"[Bool], [String], [Text], [Float], [Double], [Date], [DateTime], [UUID] "
			"FROM [TypeTest];");
		if (testClient->error() != tSQLExecErrorType_t::ok)
			printError("Query", testClient->error());
		assertTrue(result.valid());
		assertEqual(result.numRows(), 0);
		assertEqual(result.numFields(), 13);

		ormDateTime_t dateTime = typeData.dateTime;
		// Apply MSSQL rounding..
		dateTime.nanoSecond((dateTime.nanoSecond() / 100) * 100);

		assertEqual(result[0], typeData.entryID);
		assertEqual(result[1], typeData.int64);
		assertEqual(result[2], typeData.int32);
		assertEqual(result[3], typeData.int16);
		assertEqual(result[4], typeData.int8);
		assertTrue(bool{result[5]} == typeData.boolean);
		assertEqual(result[6].asString(false).get(), typeData.string);
		assertEqual(result[7].asString(false).get(), typeData.text);
		assertEqual(result[8].asFloat(), typeData.decimalF);
		assertEqual(result[9], typeData.decimalD);
		assertTrue(result[10].asDate() == typeData.date);
		assertTrue(result[11] == dateTime);
		assertTrue(result[12] == typeData.uuid);
		assertFalse(result.next());
	}
	catch (const tSQLValueError_t &error)
	{
		puts(error.error());
		fail("Exception thrown while converting value");
	}

	void testBadQuery()
	{
		assertNotNull(testClient);
		assertTrue(testClient->valid());
		tSQLQuery_t query;
		assertFalse(query.valid());
		tSQLResult_t result;
		assertFalse(result.valid());

		query = testClient->prepare(nullptr, 0);
		assertFalse(query.valid());

		query = testClient->prepare("SELECT `Bad` FROM [tmplORM];", 0);
		assertTrue(query.valid());
		result = query.execute();
		assertFalse(result.valid());
		assertFalse(testClient->error() == tSQLExecErrorType_t::ok);
	}

	void testDestroyDB()
	{
		assertNotNull(testClient);
		assertTrue(testClient->valid());
		assertTrue(testClient->selectDB("master"));
		tSQLResult_t result = testClient->query("DROP DATABASE [tmplORM];");
		if (!result.valid())
			printError("Query", testClient->error());
		assertTrue(result.valid());
		assertTrue(testClient->error() == tSQLExecErrorType_t::ok);
	}

	void testDisconnect()
	{
		assertNotNull(testClient);
		if (testClient->valid())
		{
			assertTrue(testClient->beginTransact());
			testClient->disconnect();
		}
		assertFalse(testClient->valid());
		testClient = nullptr;
		assertNull(testClient.get());
	}

	void testError()
	{
		constexpr const char *unknownError = "Unknown error";
		constexpr const char *noError = "No error";
		assertNotEqual(tSQLExecError_t{tSQLExecErrorType_t::ok, 0, nullptr}.error(), unknownError);
		assertEqual(tSQLExecError_t{tSQLExecErrorType_t::ok, 0, nullptr}.error(), noError);
		assertNotEqual(tSQLExecError_t{tSQLExecErrorType_t::connect, 0, nullptr}.error(), unknownError);
		assertNotEqual(tSQLExecError_t{tSQLExecErrorType_t::query, 0, nullptr}.error(), unknownError);
		assertNotEqual(tSQLExecError_t{tSQLExecErrorType_t::handleInv, 0, nullptr}.error(), unknownError);
		assertNotEqual(tSQLExecError_t{tSQLExecErrorType_t::generalError, 0, nullptr}.error(), unknownError);
		assertNotEqual(tSQLExecError_t{tSQLExecErrorType_t::needData, 0, nullptr}.error(), unknownError);
		assertNotEqual(tSQLExecError_t{tSQLExecErrorType_t::noData, 0, nullptr}.error(), unknownError);
		assertNotEqual(tSQLExecError_t{tSQLExecErrorType_t::dataAvail, 0, nullptr}.error(), unknownError);
		assertEqual(tSQLExecError_t{tSQLExecErrorType_t::unknown, 0, nullptr}.error(), unknownError);
		assertEqual(tSQLExecError_t{(tSQLExecErrorType_t)-1, 0, nullptr}.error(), unknownError);
	}

public:
	void registerTests() final
	{
		if (!haveEnvironment())
			skip("No suitable environment found, refusing to run");
		CXX_TEST(testInvalid)
		CXX_TEST(testConnect)
		CXX_TEST(testCreateDB)
		CXX_TEST(testSelectDB)
		CXX_TEST(testCreateTable)
		CXX_TEST(testPrepared)
		CXX_TEST(testResult)
		CXX_TEST(testTransact)
		CXX_TEST(testBind)
		CXX_TEST(testBadQuery)
		CXX_TEST(testDestroyDB)
		CXX_TEST(testDisconnect)
		CXX_TEST(testError)
	}
};

class testMSSQLValue_t final : public testsuite
{
private:
	char *S_(const char *const str, const int64_t len = -1) noexcept
	{
		if (len == -1)
			return stringDup(str).release();
		char *ret = new (std::nothrow) char[len + 1];
		if (!ret)
			return nullptr;
		return static_cast<char *>(memcpy(ret, str, len + 1));
	}

	template<typename T, typename = typename std::enable_if<std::is_fundamental<T>::value ||
		std::is_same<T, SQL_DATE_STRUCT>::value || std::is_same<T, SQL_TIMESTAMP_STRUCT>::value ||
		std::is_same<T, SQLGUID>::value>::type>
		char *S_(const T value) noexcept
	{
		constexpr size_t len = sizeof(T);
		char *ret = new (std::nothrow) char[len + 1];
		if (!ret)
			return nullptr;
		ret[len] = 0;
		*reinterpret_cast<T *>(ret) = value;
		return ret;
	}

	SQL_DATE_STRUCT asSQLType(const ormDate_t value) noexcept
		{ return {int16_t(value.year()), value.month(), value.day()}; }
	SQL_TIMESTAMP_STRUCT asSQLType(const ormDateTime_t value) noexcept
		{ return {int16_t(value.year()), value.month(), value.day(),
			value.hour(), value.minute(), value.second(), value.nanoSecond()}; }

	template<typename T> void checkValue(const T &var, const T &expected)
		{ assertEqual(var, expected); }
	void checkValue(const ormDate_t &var, const ormDate_t &expected)
		{ assertTrue(var == expected); }
	void checkValue(const ormDateTime_t &var, const ormDateTime_t &expected)
		{ assertTrue(var == expected); }
	void checkValue(const ormUUID_t &var, const ormUUID_t &expected)
		{ assertTrue(var == expected); }

	template<typename T> T tryOkConversion(const tSQLValue_t &value)
	{
		try { return T(value); }
		catch (const tSQLValueError_t &error)
			{ fail(error.error()); }
		return {};
	}

	template<typename T> void tryFailConversion(const tSQLValue_t &value)
	{
		try
		{
			T v(value);
			printf("Conversion for %s: ", typeid(T).name());
			fail("Conversion succeeded (expected to fail)");
		}
		catch (const tSQLValueError_t &) { }
	}

	template<typename T> void tryOk(const tSQLValue_t value, const T expected)
	{
		assertFalse(value.isNull());
		T var = tryOkConversion<T>(value);
		checkValue(var, expected);
	}

	template<typename T> void tryIsNull(const tSQLValue_t value)
	{
		assertTrue(value.isNull());
		tryFailConversion<T>(value);
	}

	template<typename T> void tryShouldFail(const tSQLValue_t value)
	{
		assertFalse(value.isNull());
		tryFailConversion<T>(value);
	}

	void testNull()
	{
		assertTrue(tSQLValue_t{}.isNull());
		assertFalse(tSQLValue_t{S_(""), 1, SQL_VARCHAR}.isNull());
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
		tryIsNull<ormUUID_t>({});
	}

	void testString()
	{
		const auto testData = "This is \x00\xFF only a test"_s;
		auto value = tSQLValue_t{nullptr, 0, SQL_VARCHAR};
		assertTrue(value.isNull());
		tryFailConversion<std::unique_ptr<const char []>>(value);
		value = {S_(testData.data(), testData.size()), testData.size(), SQL_VARCHAR};
		assertFalse(value.isNull());
		auto testStr = value.asString(true);
		assertNotNull(testStr);
		assertEqual(testStr.get(), testData.data(), testData.size());
	}

	void testUint8()
	{
		tryIsNull<uint8_t>({nullptr, 0, SQL_TINYINT});
		tryShouldFail<uint8_t>({S_(""), 0, SQL_VARCHAR});
		tryShouldFail<uint8_t>({S_(""), 0, SQL_SMALLINT});
		tryShouldFail<uint8_t>({S_(""), 0, SQL_INTEGER});
		tryShouldFail<uint8_t>({S_(""), 0, SQL_BIGINT});
		tryShouldFail<uint8_t>({S_(""), 0, SQL_REAL});
		tryShouldFail<uint8_t>({S_(""), 0, SQL_DOUBLE});
		tryShouldFail<uint8_t>({S_(""), 0, SQL_VARBINARY});
		tryShouldFail<uint8_t>({S_(""), 0, SQL_BIT});
		tryShouldFail<uint8_t>({S_(""), 0, SQL_TYPE_DATE});
		tryShouldFail<uint8_t>({S_(""), 0, SQL_TYPE_TIMESTAMP});
		tryShouldFail<uint8_t>({S_(""), 0, SQL_GUID});
		tryOk<uint8_t>({S_<uint8_t>(128), 2, SQL_TINYINT}, 128);
		tryOk<uint8_t>({S_<uint8_t>(255), 2, SQL_TINYINT}, 255);
		tryOk<uint8_t>({S_(""), 1, SQL_TINYINT}, 0);
		tryOk<uint8_t>({S_(""), 0, SQL_TINYINT}, 0);
	}

	void testInt8()
	{
		tryIsNull<int8_t>({nullptr, 0, SQL_TINYINT});
		tryShouldFail<int8_t>({S_(""), 0, SQL_VARCHAR});
		tryShouldFail<int8_t>({S_(""), 0, SQL_SMALLINT});
		tryShouldFail<int8_t>({S_(""), 0, SQL_INTEGER});
		tryShouldFail<int8_t>({S_(""), 0, SQL_BIGINT});
		tryShouldFail<int8_t>({S_(""), 0, SQL_REAL});
		tryShouldFail<int8_t>({S_(""), 0, SQL_DOUBLE});
		tryShouldFail<int8_t>({S_(""), 0, SQL_VARBINARY});
		tryShouldFail<int8_t>({S_(""), 0, SQL_BIT});
		tryShouldFail<int8_t>({S_(""), 0, SQL_TYPE_DATE});
		tryShouldFail<int8_t>({S_(""), 0, SQL_TYPE_TIMESTAMP});
		tryShouldFail<int8_t>({S_(""), 0, SQL_GUID});
		tryOk<int8_t>({S_<int8_t>(127), 2, SQL_TINYINT}, 127);
		tryOk<int8_t>({S_(""), 1, SQL_TINYINT}, 0);
		tryOk<int8_t>({S_(""), 0, SQL_TINYINT}, 0);
		tryOk<int8_t>({S_<int8_t>(-1), 2, SQL_TINYINT}, -1);
		tryOk<int8_t>({S_<int8_t>(-127), 2, SQL_TINYINT}, -127);
		tryOk<int8_t>({S_<int8_t>(128), 2, SQL_TINYINT}, -128);
	}

	void testUint16()
	{
		tryIsNull<uint16_t>({nullptr, 0, SQL_SMALLINT});
		tryShouldFail<uint16_t>({S_(""), 0, SQL_VARCHAR});
		tryShouldFail<uint16_t>({S_(""), 0, SQL_TINYINT});
		tryShouldFail<uint16_t>({S_(""), 0, SQL_INTEGER});
		tryShouldFail<uint16_t>({S_(""), 0, SQL_BIGINT});
		tryShouldFail<uint16_t>({S_(""), 0, SQL_REAL});
		tryShouldFail<uint16_t>({S_(""), 0, SQL_DOUBLE});
		tryShouldFail<uint16_t>({S_(""), 0, SQL_VARBINARY});
		tryShouldFail<uint16_t>({S_(""), 0, SQL_BIT});
		tryShouldFail<uint16_t>({S_(""), 0, SQL_TYPE_DATE});
		tryShouldFail<uint16_t>({S_(""), 0, SQL_TYPE_TIMESTAMP});
		tryShouldFail<uint16_t>({S_(""), 0, SQL_GUID});
		tryOk<uint16_t>({S_<uint16_t>(128), 3, SQL_SMALLINT}, 128);
		tryOk<uint16_t>({S_<uint16_t>(255), 3, SQL_SMALLINT}, 255);
		tryOk<uint16_t>({S_<uint16_t>(32768), 3, SQL_SMALLINT}, 32768);
		tryOk<uint16_t>({S_<uint16_t>(65535), 3, SQL_SMALLINT}, 65535);
		tryOk<uint16_t>({S_(""), 1, SQL_SMALLINT}, 0);
		tryOk<uint16_t>({S_(""), 0, SQL_SMALLINT}, 0);
	}

	void testInt16()
	{
		tryIsNull<int16_t>({nullptr, 0, SQL_SMALLINT});
		tryShouldFail<int16_t>({S_(""), 0, SQL_VARCHAR});
		tryShouldFail<int16_t>({S_(""), 0, SQL_TINYINT});
		tryShouldFail<int16_t>({S_(""), 0, SQL_INTEGER});
		tryShouldFail<int16_t>({S_(""), 0, SQL_BIGINT});
		tryShouldFail<int16_t>({S_(""), 0, SQL_REAL});
		tryShouldFail<int16_t>({S_(""), 0, SQL_DOUBLE});
		tryShouldFail<int16_t>({S_(""), 0, SQL_VARBINARY});
		tryShouldFail<int16_t>({S_(""), 0, SQL_BIT});
		tryShouldFail<int16_t>({S_(""), 0, SQL_TYPE_DATE});
		tryShouldFail<int16_t>({S_(""), 0, SQL_TYPE_TIMESTAMP});
		tryShouldFail<int16_t>({S_(""), 0, SQL_GUID});
		tryOk<int16_t>({S_<int16_t>(127), 3, SQL_SMALLINT}, 127);
		tryOk<int16_t>({S_<int16_t>(32767), 3, SQL_SMALLINT}, 32767);
		tryOk<int16_t>({S_(""), 1, SQL_SMALLINT}, 0);
		tryOk<int16_t>({S_(""), 0, SQL_SMALLINT}, 0);
		tryOk<int16_t>({S_<int16_t>(-1), 3, SQL_SMALLINT}, -1);
		tryOk<int16_t>({S_<int16_t>(-32767), 3, SQL_SMALLINT}, -32767);
		tryOk<int16_t>({S_<int16_t>(-32768), 3, SQL_SMALLINT}, -32768);
	}

	void testUint32()
	{
		tryIsNull<uint32_t>({nullptr, 0, SQL_INTEGER});
		tryShouldFail<uint32_t>({S_(""), 0, SQL_VARCHAR});
		tryShouldFail<uint32_t>({S_(""), 0, SQL_TINYINT});
		tryShouldFail<uint32_t>({S_(""), 0, SQL_SMALLINT});
		tryShouldFail<uint32_t>({S_(""), 0, SQL_BIGINT});
		tryShouldFail<uint32_t>({S_(""), 0, SQL_REAL});
		tryShouldFail<uint32_t>({S_(""), 0, SQL_DOUBLE});
		tryShouldFail<uint32_t>({S_(""), 0, SQL_VARBINARY});
		tryShouldFail<uint32_t>({S_(""), 0, SQL_BIT});
		tryShouldFail<uint32_t>({S_(""), 0, SQL_TYPE_DATE});
		tryShouldFail<uint32_t>({S_(""), 0, SQL_TYPE_TIMESTAMP});
		tryShouldFail<uint32_t>({S_(""), 0, SQL_GUID});
		tryOk<uint32_t>({S_<uint32_t>(128), 5, SQL_INTEGER}, 128);
		tryOk<uint32_t>({S_<uint32_t>(255), 5, SQL_INTEGER}, 255);
		tryOk<uint32_t>({S_<uint32_t>(32768), 5, SQL_INTEGER}, 32768);
		tryOk<uint32_t>({S_<uint32_t>(65535), 5, SQL_INTEGER}, 65535);
		tryOk<uint32_t>({S_<uint32_t>(2147483648), 5, SQL_INTEGER}, 2147483648);
		tryOk<uint32_t>({S_<uint32_t>(4294967295), 5, SQL_INTEGER}, 4294967295);
		tryOk<uint32_t>({S_(""), 1, SQL_INTEGER}, 0);
		tryOk<uint32_t>({S_(""), 0, SQL_INTEGER}, 0);
	}

	void testInt32()
	{
		tryIsNull<int32_t>({nullptr, 0, SQL_INTEGER});
		tryShouldFail<int32_t>({S_(""), 0, SQL_VARCHAR});
		tryShouldFail<int32_t>({S_(""), 0, SQL_TINYINT});
		tryShouldFail<int32_t>({S_(""), 0, SQL_SMALLINT});
		tryShouldFail<int32_t>({S_(""), 0, SQL_BIGINT});
		tryShouldFail<int32_t>({S_(""), 0, SQL_REAL});
		tryShouldFail<int32_t>({S_(""), 0, SQL_DOUBLE});
		tryShouldFail<int32_t>({S_(""), 0, SQL_VARBINARY});
		tryShouldFail<int32_t>({S_(""), 0, SQL_BIT});
		tryShouldFail<int32_t>({S_(""), 0, SQL_TYPE_DATE});
		tryShouldFail<int32_t>({S_(""), 0, SQL_TYPE_TIMESTAMP});
		tryShouldFail<int32_t>({S_(""), 0, SQL_GUID});
		tryOk<int32_t>({S_<int32_t>(127), 5, SQL_INTEGER}, 127);
		tryOk<int32_t>({S_<int32_t>(32767), 5, SQL_INTEGER}, 32767);
		tryOk<int32_t>({S_<int32_t>(2147483647), 5, SQL_INTEGER}, 2147483647);
		tryOk<int32_t>({S_(""), 1, SQL_INTEGER}, 0);
		tryOk<int32_t>({S_(""), 0, SQL_INTEGER}, 0);
		tryOk<int32_t>({S_<int32_t>(-1), 5, SQL_INTEGER}, -1);
		tryOk<int32_t>({S_<int32_t>(-2147483647), 5, SQL_INTEGER}, -2147483647);
		tryOk<int32_t>({S_<int32_t>(-2147483648), 5, SQL_INTEGER}, -2147483648);
	}

	void testUint64()
	{
		tryIsNull<uint64_t>({nullptr, 0, SQL_BIGINT});
		tryShouldFail<uint64_t>({S_(""), 0, SQL_VARCHAR});
		tryShouldFail<uint64_t>({S_(""), 0, SQL_TINYINT});
		tryShouldFail<uint64_t>({S_(""), 0, SQL_SMALLINT});
		tryShouldFail<uint64_t>({S_(""), 0, SQL_INTEGER});
		tryShouldFail<uint64_t>({S_(""), 0, SQL_REAL});
		tryShouldFail<uint64_t>({S_(""), 0, SQL_DOUBLE});
		tryShouldFail<uint64_t>({S_(""), 0, SQL_VARBINARY});
		tryShouldFail<uint64_t>({S_(""), 0, SQL_BIT});
		tryShouldFail<uint64_t>({S_(""), 0, SQL_TYPE_DATE});
		tryShouldFail<uint64_t>({S_(""), 0, SQL_TYPE_TIMESTAMP});
		tryShouldFail<uint64_t>({S_(""), 0, SQL_GUID});
		tryOk<uint64_t>({S_<uint64_t>(128), 9, SQL_BIGINT}, 128);
		tryOk<uint64_t>({S_<uint64_t>(255), 9, SQL_BIGINT}, 255);
		tryOk<uint64_t>({S_<uint64_t>(32768), 9, SQL_BIGINT}, 32768);
		tryOk<uint64_t>({S_<uint64_t>(65535), 9, SQL_BIGINT}, 65535);
		tryOk<uint64_t>({S_<uint64_t>(2147483648), 9, SQL_BIGINT}, 2147483648);
		tryOk<uint64_t>({S_<uint64_t>(4294967295), 9, SQL_BIGINT}, 4294967295);
		tryOk<uint64_t>({S_<uint64_t>(u64(9223372036854775808)), 9, SQL_BIGINT}, u64(9223372036854775808));
		tryOk<uint64_t>({S_<uint64_t>(u64(18446744073709551615)), 9, SQL_BIGINT}, u64(18446744073709551615));
		tryOk<uint64_t>({S_(""), 1, SQL_BIGINT}, 0);
		tryOk<uint64_t>({S_(""), 0, SQL_BIGINT}, 0);
	}

	void testInt64()
	{
		tryIsNull<int64_t>({nullptr, 0, SQL_BIGINT});
		tryShouldFail<int64_t>({S_(""), 0, SQL_VARCHAR});
		tryShouldFail<int64_t>({S_(""), 0, SQL_TINYINT});
		tryShouldFail<int64_t>({S_(""), 0, SQL_SMALLINT});
		tryShouldFail<int64_t>({S_(""), 0, SQL_INTEGER});
		tryShouldFail<int64_t>({S_(""), 0, SQL_REAL});
		tryShouldFail<int64_t>({S_(""), 0, SQL_DOUBLE});
		tryShouldFail<int64_t>({S_(""), 0, SQL_VARBINARY});
		tryShouldFail<int64_t>({S_(""), 0, SQL_BIT});
		tryShouldFail<int64_t>({S_(""), 0, SQL_TYPE_DATE});
		tryShouldFail<int64_t>({S_(""), 0, SQL_TYPE_TIMESTAMP});
		tryShouldFail<int64_t>({S_(""), 0, SQL_GUID});
		tryOk<int64_t>({S_<int64_t>(127), 9, SQL_BIGINT}, 127);
		tryOk<int64_t>({S_<int64_t>(32767), 9, SQL_BIGINT}, 32767);
		tryOk<int64_t>({S_<int64_t>(2147483647), 9, SQL_BIGINT}, 2147483647);
		tryOk<int64_t>({S_<int64_t>(i64(9223372036854775807)), 9, SQL_BIGINT}, i64(9223372036854775807));
		tryOk<int64_t>({S_(""), 1, SQL_BIGINT}, 0);
		tryOk<int64_t>({S_(""), 0, SQL_BIGINT}, 0);
		tryOk<int64_t>({S_<int64_t>(-1), 9, SQL_BIGINT}, -1);
		tryOk<int64_t>({S_<int64_t>(-2147483647), 9, SQL_BIGINT}, -2147483647);
		tryOk<int64_t>({S_<int64_t>(-2147483648), 9, SQL_BIGINT}, -2147483648);
		tryOk<int64_t>({S_<int64_t>(i64(-9223372036854775807)), 9, SQL_BIGINT}, i64(-9223372036854775807));
		tryOk<int64_t>({S_<int64_t>(i64(-9223372036854775807) - 1), 9, SQL_BIGINT}, i64(-9223372036854775807) - 1);
	}

	void testFloat()
	{
		constexpr float infPos = std::numeric_limits<float>::infinity(),
			infNeg = -std::numeric_limits<float>::infinity();
		tryIsNull<float>({nullptr, 0, SQL_REAL});
		tryShouldFail<float>({S_(""), 0, SQL_VARCHAR});
		tryShouldFail<float>({S_(""), 0, SQL_TINYINT});
		tryShouldFail<float>({S_(""), 0, SQL_SMALLINT});
		tryShouldFail<float>({S_(""), 0, SQL_INTEGER});
		tryShouldFail<float>({S_(""), 0, SQL_BIGINT});
		tryShouldFail<float>({S_(""), 0, SQL_DOUBLE});
		tryShouldFail<float>({S_(""), 0, SQL_VARBINARY});
		tryShouldFail<float>({S_(""), 0, SQL_BIT});
		tryShouldFail<float>({S_(""), 0, SQL_TYPE_DATE});
		tryShouldFail<float>({S_(""), 0, SQL_TYPE_TIMESTAMP});
		tryShouldFail<float>({S_(""), 0, SQL_GUID});
		tryOk<float>({S_<float>(0.0), 5, SQL_REAL}, 0.0);
		tryOk<float>({S_<float>(infPos), 5, SQL_REAL}, infPos);
		tryOk<float>({S_<float>(infNeg), 5, SQL_REAL}, infNeg);
		tryOk<float>({S_(""), 0, SQL_REAL}, 0);
		tryOk<float>({S_(""), 1, SQL_REAL}, 0);
	}

	void testDouble()
	{
		constexpr double infPos = std::numeric_limits<double>::infinity(),
			infNeg = -std::numeric_limits<double>::infinity();
		tryIsNull<double>({nullptr, 0, SQL_DOUBLE});
		tryShouldFail<double>({S_(""), 0, SQL_VARCHAR});
		tryShouldFail<double>({S_(""), 0, SQL_TINYINT});
		tryShouldFail<double>({S_(""), 0, SQL_SMALLINT});
		tryShouldFail<double>({S_(""), 0, SQL_INTEGER});
		tryShouldFail<double>({S_(""), 0, SQL_BIGINT});
		tryShouldFail<double>({S_(""), 0, SQL_REAL});
		tryShouldFail<double>({S_(""), 0, SQL_VARBINARY});
		tryShouldFail<double>({S_(""), 0, SQL_BIT});
		tryShouldFail<double>({S_(""), 0, SQL_TYPE_DATE});
		tryShouldFail<double>({S_(""), 0, SQL_TYPE_TIMESTAMP});
		tryShouldFail<double>({S_(""), 0, SQL_GUID});
		tryOk<double>({S_<double>(0.0), 9, SQL_DOUBLE}, 0.0);
		tryOk<double>({S_<double>(infPos), 9, SQL_DOUBLE}, infPos);
		tryOk<double>({S_<double>(infNeg), 9, SQL_DOUBLE}, infNeg);
		tryOk<double>({S_(""), 0, SQL_DOUBLE}, 0);
		tryOk<double>({S_(""), 1, SQL_DOUBLE}, 0);
	}

	void testBool()
	{
		tryIsNull<bool>({nullptr, 0, SQL_BIT});
		tryShouldFail<bool>({S_(""), 0, SQL_VARCHAR});
		tryShouldFail<bool>({S_(""), 0, SQL_TINYINT});
		tryShouldFail<bool>({S_(""), 0, SQL_SMALLINT});
		tryShouldFail<bool>({S_(""), 0, SQL_INTEGER});
		tryShouldFail<bool>({S_(""), 0, SQL_BIGINT});
		tryShouldFail<bool>({S_(""), 0, SQL_REAL});
		tryShouldFail<bool>({S_(""), 0, SQL_DOUBLE});
		tryShouldFail<bool>({S_(""), 0, SQL_VARBINARY});
		tryShouldFail<bool>({S_(""), 0, SQL_TYPE_DATE});
		tryShouldFail<bool>({S_(""), 0, SQL_TYPE_TIMESTAMP});
		tryShouldFail<bool>({S_(""), 0, SQL_GUID});
		tryOk<bool>({S_<bool>(false), 2, SQL_BIT}, false);
		tryOk<bool>({S_<bool>(true), 2, SQL_BIT}, true);
		tryOk<bool>({S_(""), 0, SQL_BIT}, false);
		tryOk<bool>({S_(""), 1, SQL_BIT}, false);
	}

	void testDate()
	{
		tryIsNull<ormDate_t>({nullptr, 0, SQL_TYPE_DATE});
		tryShouldFail<ormDate_t>({S_(""), 0, SQL_VARCHAR});
		tryShouldFail<ormDate_t>({S_(""), 0, SQL_TINYINT});
		tryShouldFail<ormDate_t>({S_(""), 0, SQL_SMALLINT});
		tryShouldFail<ormDate_t>({S_(""), 0, SQL_INTEGER});
		tryShouldFail<ormDate_t>({S_(""), 0, SQL_BIGINT});
		tryShouldFail<ormDate_t>({S_(""), 0, SQL_REAL});
		tryShouldFail<ormDate_t>({S_(""), 0, SQL_DOUBLE});
		tryShouldFail<ormDate_t>({S_(""), 0, SQL_VARBINARY});
		tryShouldFail<ormDate_t>({S_(""), 0, SQL_BIT});
		tryShouldFail<ormDate_t>({S_(""), 0, SQL_TYPE_TIMESTAMP});
		tryShouldFail<ormDate_t>({S_(""), 0, SQL_GUID});
		tryOk<ormDate_t>({S_<SQL_DATE_STRUCT>({}), 7, SQL_TYPE_DATE}, {});
		tryOk<ormDate_t>({S_<SQL_DATE_STRUCT>(asSQLType(ormDate_t{now})), 7, SQL_TYPE_DATE}, now);
		tryShouldFail<ormDate_t>({S_(""), 0, SQL_TYPE_DATE});
		tryShouldFail<ormDate_t>({S_(""), 1, SQL_TYPE_DATE});
	}

	void testDateTime()
	{
		now.nanoSecond((now.nanoSecond() / 100) * 100);
		tryIsNull<ormDateTime_t>({nullptr, 0, SQL_TYPE_TIMESTAMP});
		tryShouldFail<ormDateTime_t>({S_(""), 0, SQL_VARCHAR});
		tryShouldFail<ormDateTime_t>({S_(""), 0, SQL_TINYINT});
		tryShouldFail<ormDateTime_t>({S_(""), 0, SQL_SMALLINT});
		tryShouldFail<ormDateTime_t>({S_(""), 0, SQL_INTEGER});
		tryShouldFail<ormDateTime_t>({S_(""), 0, SQL_BIGINT});
		tryShouldFail<ormDateTime_t>({S_(""), 0, SQL_REAL});
		tryShouldFail<ormDateTime_t>({S_(""), 0, SQL_DOUBLE});
		tryShouldFail<ormDateTime_t>({S_(""), 0, SQL_VARBINARY});
		tryShouldFail<ormDateTime_t>({S_(""), 0, SQL_BIT});
		tryShouldFail<ormDateTime_t>({S_(""), 0, SQL_TYPE_DATE});
		tryShouldFail<ormDateTime_t>({S_(""), 0, SQL_GUID});
		tryOk<ormDateTime_t>({S_<SQL_TIMESTAMP_STRUCT>({}), 17, SQL_TYPE_TIMESTAMP}, {});
		tryOk<ormDateTime_t>({S_<SQL_TIMESTAMP_STRUCT>(asSQLType(now)), 17, SQL_TYPE_TIMESTAMP}, now);
		tryShouldFail<ormDateTime_t>({S_(""), 0, SQL_TYPE_TIMESTAMP});
		tryShouldFail<ormDateTime_t>({S_(""), 1, SQL_TYPE_TIMESTAMP});
	}

	void testUUID()
	{
		// Set up our UUID value.
		const auto now = systemClock_t::now().time_since_epoch();
		const uint64_t time = durationIn<milliseconds>(now);
		const auto nanoSeconds = (now - milliseconds{time}).count();

		const ormUUID_t uuidORM
		{
			uint32_t(time), uint16_t(time >> 32),
			uint16_t(0x1000 | ((time >> 48) & 0x0FFF)),
			uint16_t((nanoSeconds >> 14) | 0x8000), swapBytes(uint64_t{0x123456789ABCU}) >> 16
		};
		const SQLGUID uuidSQL
		{
			uint32_t(time), uint16_t(time >> 32),
			uint16_t(0x1000 | ((time >> 48) & 0x0FFF)),
			{
				uint8_t(0x80 | uint8_t(nanoSeconds >> 22)),
				uint8_t(nanoSeconds >> 14), 0x12,
				0x34, 0x56, 0x78, 0x9A, 0xBC
			}
		};

		tryIsNull<ormUUID_t>({nullptr, 0, SQL_GUID});
		tryShouldFail<ormUUID_t>({S_(""), 0, SQL_VARCHAR});
		tryShouldFail<ormUUID_t>({S_(""), 0, SQL_TINYINT});
		tryShouldFail<ormUUID_t>({S_(""), 0, SQL_SMALLINT});
		tryShouldFail<ormUUID_t>({S_(""), 0, SQL_INTEGER});
		tryShouldFail<ormUUID_t>({S_(""), 0, SQL_BIGINT});
		tryShouldFail<ormUUID_t>({S_(""), 0, SQL_REAL});
		tryShouldFail<ormUUID_t>({S_(""), 0, SQL_DOUBLE});
		tryShouldFail<ormUUID_t>({S_(""), 0, SQL_VARBINARY});
		tryShouldFail<ormUUID_t>({S_(""), 0, SQL_BIT});
		tryShouldFail<ormUUID_t>({S_(""), 0, SQL_TYPE_DATE});
		tryShouldFail<ormUUID_t>({S_(""), 0, SQL_TYPE_TIMESTAMP});
		tryOk<ormUUID_t>({S_<SQLGUID>({}), 33, SQL_GUID}, {});
		tryOk<ormUUID_t>({S_<SQLGUID>(uuidSQL), 33, SQL_GUID}, uuidORM);
	}

	void testError()
	{
		constexpr const char *unknownError = "An unknown error occured";
		constexpr const char *noError = "No error occured";
		assertNotEqual(tSQLValueError_t{tSQLErrorType_t::noError}.error(), unknownError);
		assertEqual(tSQLValueError_t{tSQLErrorType_t::noError}.error(), noError);
		assertNotEqual(tSQLValueError_t{tSQLErrorType_t::stringError}.error(), unknownError);
		assertNotEqual(tSQLValueError_t{tSQLErrorType_t::boolError}.error(), unknownError);
		assertNotEqual(tSQLValueError_t{tSQLErrorType_t::uint8Error}.error(), unknownError);
		assertNotEqual(tSQLValueError_t{tSQLErrorType_t::int8Error}.error(), unknownError);
		assertNotEqual(tSQLValueError_t{tSQLErrorType_t::uint16Error}.error(), unknownError);
		assertNotEqual(tSQLValueError_t{tSQLErrorType_t::int16Error}.error(), unknownError);
		assertNotEqual(tSQLValueError_t{tSQLErrorType_t::uint32Error}.error(), unknownError);
		assertNotEqual(tSQLValueError_t{tSQLErrorType_t::int32Error}.error(), unknownError);
		assertNotEqual(tSQLValueError_t{tSQLErrorType_t::uint64Error}.error(), unknownError);
		assertNotEqual(tSQLValueError_t{tSQLErrorType_t::int64Error}.error(), unknownError);
		assertNotEqual(tSQLValueError_t{tSQLErrorType_t::floatError}.error(), unknownError);
		assertNotEqual(tSQLValueError_t{tSQLErrorType_t::doubleError}.error(), unknownError);
		assertNotEqual(tSQLValueError_t{tSQLErrorType_t::binError}.error(), unknownError);
		assertNotEqual(tSQLValueError_t{tSQLErrorType_t::dateError}.error(), unknownError);
		assertNotEqual(tSQLValueError_t{tSQLErrorType_t::dateTimeError}.error(), unknownError);
		assertNotEqual(tSQLValueError_t{tSQLErrorType_t::uuidError}.error(), unknownError);
		assertEqual(tSQLValueError_t{(tSQLErrorType_t)-1}.error(), unknownError);
	}

public:
	void registerTests() final
	{
		CXX_TEST(testNull)
		CXX_TEST(testString)
		CXX_TEST(testUint8)
		CXX_TEST(testInt8)
		CXX_TEST(testUint16)
		CXX_TEST(testInt16)
		CXX_TEST(testUint32)
		CXX_TEST(testInt32)
		CXX_TEST(testUint64)
		CXX_TEST(testInt64)
		CXX_TEST(testFloat)
		CXX_TEST(testDouble)
		CXX_TEST(testBool)
		CXX_TEST(testDate)
		CXX_TEST(testDateTime)
		CXX_TEST(testUUID)
		CXX_TEST(testError)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept {registerTestClasses<testMSSQLValue_t, testMSSQL_t>(); }
