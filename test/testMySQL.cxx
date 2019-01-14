#include <chrono>
#include <crunch++.h>
#include <mysql.hxx>

#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <tmplORM.mysql.hxx>
#include "constString.hxx"

/*!
 * @internal
 * @file
 * @author Rachel Mant
 * @date 2016-2017
 * @brief Unit tests for the MySQL driver abstraction layer
 */

using namespace tmplORM::mysql::driver;
using irqus::typestring;
using tmplORM::mysql::fieldLength;
using tmplORM::types::baseTypes::ormDateTime_t;

using systemClock_t = std::chrono::system_clock;
#define u64(n)		UINT64_C(n)
#define i64(n)		INT64_C(n)
using std::chrono::milliseconds;
using tmplORM::types::chrono::durationIn;

std::unique_ptr<mySQLClient_t> testClient{};
constString_t host, username, password;

constexpr static uint32_t port = 3306;
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
	host = getenv("MYSQL_HOST");
	// port?
	username = getenv("MYSQL_USERNAME");
	password = getenv("MYSQL_PASSWORD");
	return !(host.empty() || username.empty() || password.empty());
}

class testMySQL_t final : public testsuit
{
private:
	void printError(const char *prefix, const mySQLClient_t &client)
	{
		const auto errorStr = client.error();
		const auto errorNum = client.errorNum();
		printf("%s failed (%u): %s\n", prefix, errorNum, errorStr);
	}

	void printError(const char *prefix, const mySQLPreparedQuery_t &query)
	{
		const auto errorStr = query.error();
		const auto errorNum = query.errorNum();
		printf("%s failed (%u): %s\n", prefix, errorNum, errorStr);
	}

	void testInvalid()
	{
		mySQLClient_t testClient;
		assertFalse(testClient.valid());
		assertFalse(testClient.queryResult().valid());
		assertEqual(testClient.errorNum(), 0);
		assertEqual(testClient.error(), "");
		mySQLPreparedQuery_t testQuery = testClient.prepare("", 0);
		assertFalse(testQuery.valid());
		assertFalse(testQuery.execute());
		assertEqual(testQuery.rowID(), 0);
		mySQLPreparedResult_t testPrepResult = testQuery.queryResult(0);
		assertTrue(testPrepResult.valid());
		assertEqual(testPrepResult.numRows(), 0);
		mySQLResult_t testResult;
		assertFalse(testResult.valid());
		assertEqual(testResult.numRows(), 0);
		assertFalse(testResult.resultRows().valid());
		mySQLRow_t testRow;
		assertFalse(testRow.valid());
		assertEqual(testRow.numFields(), 0);
		assertFalse(testRow.next());
		assertTrue(testRow[0].isNull());
		mySQLBind_t testBind;
		assertTrue(testBind.valid());
		assertFalse(testBind.haveData());
		assertNull(testBind.data());
	}

	void testClientType()
	{
		mySQLClient_t client1;
		assertFalse(client1.valid());
		mySQLClient_t client2(client1);
		assertFalse(client1.valid());
		mySQLClient_t client3;
		assertFalse(client3.valid());
		client3 = client2;
		assertFalse(client3.valid());
	}

	void testConnect()
	{
		assertNull(testClient);
		mySQLClient_t client;
		assertFalse(client.valid());
		const bool connected = client.connect(host, port, username, password);
		if (!connected)
			printError("Connection", client);
		assertTrue(connected);
		assertEqual(client.errorNum(), 0);
		assertTrue(client.valid());

		testClient = makeUnique<mySQLClient_t>();
		assertNotNull(testClient);
		assertTrue(testClient->valid());
		// If we try to connect again while already connected, it should no-op.
		assertTrue(client.connect(host, port, username, password));
	}

	void testCreateDB()
	{
		assertNotNull(testClient);
		assertTrue(testClient->valid());
		const bool result = testClient->query("CREATE DATABASE `tmplORM` CHARACTER SET utf8 COLLATE utf8_unicode_ci;");
		if (!result)
			printError("Query", *testClient);
		assertTrue(result);
		assertEqual(testClient->errorNum(), 0);
	}

	void testSelectDB()
	{
		assertNotNull(testClient);
		assertTrue(testClient->valid());
		const bool selected = testClient->selectDB("tmplORM");
		if (!selected)
			printError("DB selection", *testClient);
		assertTrue(selected);
		assertEqual(testClient->errorNum(), 0);
	}

	void testCreateTable()
	{
		assertNotNull(testClient);
		assertTrue(testClient->valid());
		bool result{false};
		assertFalse(result);

		result = testClient->query(
			"CREATE TABLE `tmplORM` ("
			"`EntryID` INT NOT NULL PRIMARY KEY AUTO_INCREMENT, "
			"`Name` VARCHAR(50) NOT NULL, "
			"`Value` INT NULL, "
			"`When` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP"
			") CHARACTER SET utf8;"
		);
		if (!result)
			printError("Query", *testClient);
		assertTrue(result);
		assertEqual(testClient->errorNum(), 0);

		result = testClient->query(
			"CREATE TABLE `TypeTest` ("
			"`EntryID` INT NOT NULL PRIMARY KEY AUTO_INCREMENT, "
			"`Int64` BIGINT NOT NULL, "
			"`Int32` INT NOT NULL, "
			"`Int16` SMALLINT NOT NULL, "
			"`Int8` TINYINT NOT NULL, "
			"`Bool` BIT NOT NULL, "
			"`String` VARCHAR(50) NOT NULL, "
			"`Text` TEXT NOT NULL, "
			"`Float` FLOAT NOT NULL, "
			"`Double` DOUBLE NOT NULL, "
			"`Date` DATE NOT NULL, "
			"`DateTime` DATETIME NOT NULL, "
			"`UUID` CHAR(32) NOT NULL"
			") CHARACTER SET utf8;"
		);
		if (!result)
			printError("Query", *testClient);
		assertTrue(result);
		assertEqual(testClient->errorNum(), 0);
	}

	void testPreparedQuery()
	{
		assertNotNull(testClient);
		assertTrue(testClient->valid());
		mySQLPreparedQuery_t query;
		assertFalse(query.valid());
		bool result{};
		assertFalse(result);

		query = testClient->prepare("INSERT INTO `tmplORM` (`Name`, `Value`) VALUES (?, ?);", 2);
		assertTrue(query.valid());
		query.bind(0, testData[0].name.value(), fieldLength(testData[0].name));
		assertEqual(testClient->errorNum(), 0);
		query.bind(1, testData[0].value.value(), fieldLength(testData[0].value));
		assertEqual(testClient->errorNum(), 0);
		result = query.execute();
		if (!result)
			printError("Prepared exec", query);
		assertTrue(result);

		testData[0].entryID = query.rowID();
		assertEqual(testData[0].entryID, 1);

		testData[1].when = now;
		query = testClient->prepare("INSERT INTO `tmplORM` (`Name`, `Value`, `When`) VALUES (?, ?, ?);", 3);
		assertTrue(query.valid());
		query.bind(0, testData[1].name.value(), fieldLength(testData[1].name));
		assertEqual(testClient->errorNum(), 0);
		query.bind<const char *>(1, nullptr, fieldLength(testData[1].value));
		assertEqual(testClient->errorNum(), 0);
		query.bind(2, testData[1].when.value(), fieldLength(testData[1].when));
		assertEqual(testClient->errorNum(), 0);
		result = query.execute();
		if (!result)
			printError("Prepared exec", query);
		assertTrue(result);

		testData[1].entryID = query.rowID();
		assertEqual(testData[1].entryID, 2);

		query = testClient->prepare("", 1);
		assertFalse(query.valid());
		query = testClient->prepare("", 0);
		assertFalse(query.valid());
	}

	void testResult() try
	{
		assertNotNull(testClient);
		assertTrue(testClient->valid());
		const bool query = testClient->query("SELECT `EntryID`, `Name`, `Value`, `When` FROM `tmplORM`;");
		if (!query)
			printError("Query", *testClient);
		assertTrue(query);
		mySQLResult_t result = testClient->queryResult();
		assertTrue(result.valid());
		assertEqual(result.numRows(), 2);
		mySQLRow_t row = result.resultRows();
		assertTrue(row.valid());
		assertEqual(row.numFields(), 4);

		assertEqual(row[0], testData[0].entryID);
		assertEqual(row[1].asString().get(), testData[0].name);
		assertFalse(row[2].isNull());
		testData[0].when = row[3];
		auto when = testData[0].when.value();
		assertNotEqual(when.year(), 0);
		assertNotEqual(when.month(), 0);
		assertNotEqual(when.day(), 0);
		assertTrue(row.next());

		assertEqual(row[0], testData[1].entryID);
		assertEqual(row[1].asString().get(), testData[1].name);
		assertTrue(row[2].isNull());
		// MySQL discards the fractional part..
		now.nanoSecond(0);
		assertTrue(row[3].asDateTime() == now);
		assertFalse(row.next());

		assertTrue(result.valid());
		mySQLResult_t testResult;
		assertFalse(testResult.valid());
		testResult = std::move(result);
		assertFalse(result.valid());
		assertTrue(testResult.valid());
	}
	catch (const mySQLValueError_t &error)
	{
		puts(error.error());
		fail("Exception thrown while converting value");
	}

	template<typename field_t> void bind(mySQLPreparedResult_t &result, const size_t index, field_t &value) noexcept
		{ result.template bind<typename field_t::type>(index, fieldLength(value)); }

	void testPreparedResult()
	{
		data_t data;
		assertNotNull(testClient);
		assertTrue(testClient->valid());
		mySQLPreparedQuery_t query;
		assertFalse(query.valid());

		query = testClient->prepare("SELECT `EntryID`, `Name`, `Value`, `When` "
			"FROM `tmplORM` WHERE `EntryID` = ?;", 1);
		assertTrue(query.valid());
		query.bind(0, testData[0].entryID.value(), fieldLength(testData[0].entryID));
		assertEqual(testClient->errorNum(), 0);
		const bool queryResult = query.execute();
		if (!queryResult)
			printError("Prepared exec", query);
		assertTrue(queryResult);

		mySQLPreparedResult_t result = query.queryResult(4);
		assertTrue(result.valid());
		//assertEqual(result.numRows(), 1);
		bind(result, 0, data.entryID);
		assertEqual(testClient->errorNum(), 0);
		result.bindForBuffer(1);
		assertEqual(testClient->errorNum(), 0);
		bind(result, 2, data.value);
		assertEqual(testClient->errorNum(), 0);
		bind(result, 3, data.when);
		assertEqual(testClient->errorNum(), 0);

		// TODO: Continue this test to pull back real data and play.. needs the rest of the type written first.
	}

	void testBind() try
	{
		assertNotNull(testClient);
		assertTrue(testClient->valid());
		bool execResult{false};
		assertFalse(execResult);

		// Set up our UUID value.
		const auto now = systemClock_t::now().time_since_epoch();
		const uint64_t time = durationIn<milliseconds>(now);
		const auto nanoSeconds = (now - milliseconds{time}).count();
		typeData.uuid.value(ormUUID_t{uint32_t(time), uint16_t(time >> 32),
			uint16_t(0x1000 | ((time >> 48) & 0x0FFF)),
			uint16_t((nanoSeconds >> 14) | 0x8000), swapBytes(uint64_t{0x123456789ABCU}) >> 16});

		mySQLPreparedQuery_t query = testClient->prepare(
			"INSERT INTO `TypeTest` (`Int64`, `Int32`, `Int16`, `Int8`, `Bool`, "
			"`String`, `Text`, `Float`, `Double`, `Date`, `DateTime`, `UUID`) "
			"VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);", 12
		);
		assertTrue(query.valid());
		query.bind(0, typeData.int64.value(), fieldLength(typeData.int64));
		assertEqual(testClient->errorNum(), 0);
		query.bind(1, typeData.int32.value(), fieldLength(typeData.int32));
		assertEqual(testClient->errorNum(), 0);
		query.bind(2, typeData.int16.value(), fieldLength(typeData.int16));
		assertEqual(testClient->errorNum(), 0);
		query.bind(3, typeData.int8.value(), fieldLength(typeData.int8));
		assertEqual(testClient->errorNum(), 0);
		query.bind(4, typeData.boolean.value(), fieldLength(typeData.boolean));
		assertEqual(testClient->errorNum(), 0);
		query.bind(5, typeData.string.value(), fieldLength(typeData.string));
		assertEqual(testClient->errorNum(), 0);
		query.bind(6, typeData.text.value(), fieldLength(typeData.text));
		assertEqual(testClient->errorNum(), 0);
		query.bind(7, typeData.decimalF.value(), fieldLength(typeData.decimalF));
		assertEqual(testClient->errorNum(), 0);
		query.bind(8, typeData.decimalD.value(), fieldLength(typeData.decimalD));
		assertEqual(testClient->errorNum(), 0);
		query.bind(9, typeData.date.value(), fieldLength(typeData.date));
		assertEqual(testClient->errorNum(), 0);
		query.bind(10, typeData.dateTime.value(), fieldLength(typeData.dateTime));
		assertEqual(testClient->errorNum(), 0);
		query.bind(11, typeData.uuid.value(), fieldLength(typeData.uuid));
		assertEqual(testClient->errorNum(), 0);

		execResult = query.execute();
		if (!execResult)
			printError("Prepared exec", query);
		assertTrue(execResult);
		typeData.entryID = query.rowID();
		assertEqual(typeData.entryID, 1);

		execResult = testClient->query("SELECT `EntryID`, `Int64`, `Int32`, `Int16`, `Int8`, "
			"`Bool`, `String`, `Text`, `Float`, `Double`, `Date`, `DateTime`, `UUID` "
			"FROM `TypeTest`;");
		if (!execResult)
			printError("Query", *testClient);
		assertTrue(execResult);
		const mySQLResult_t result = testClient->queryResult();
		assertTrue(result.valid());
		assertEqual(result.numRows(), 1);
		mySQLRow_t row = result.resultRows();
		assertTrue(row.valid());
		assertEqual(row.numFields(), 13);

		ormDateTime_t dateTime = typeData.dateTime;
		// MySQL doesn't store this part of the ormDateTime_t..
		dateTime.nanoSecond(0);

		assertEqual(row[0], typeData.entryID);
		assertEqual(row[1], typeData.int64);
		assertEqual(row[2], typeData.int32);
		assertEqual(row[3], typeData.int16);
		assertEqual(row[4], typeData.int8);
		assertTrue(bool{row[5]} == typeData.boolean);
		assertEqual(row[6].asString().get(), typeData.string);
		assertEqual(row[7].asString().get(), typeData.text);
		//assertEqual(row[8].asFloat(), typeData.decimalF);
		//assertEqual(row[9], typeData.decimalD);
		assertTrue(row[10].asDate() == typeData.date);
		assertTrue(row[11] == dateTime);
		assertTrue(row[12] == typeData.uuid);
		assertFalse(row.next());
	}
	catch (const mySQLValueError_t &error)
	{
		puts(error.error());
		fail("Exception thrown while converting value");
	}

	void testDestroyDB()
	{
		assertNotNull(testClient);
		assertTrue(testClient->valid());
		assertTrue(testClient->selectDB("mysql"));
		const bool result = testClient->query("DROP DATABASE `tmplORM`;");
		if (!result)
			printError("Query", *testClient);
		assertTrue(result);
		assertEqual(testClient->errorNum(), 0);
	}

	void testDisconnect()
	{
		assertNotNull(testClient);
		if (testClient->valid())
			testClient->disconnect();
		assertFalse(testClient->valid());
		testClient = nullptr;
		assertNull(testClient.get());
	}

	// TODO: Build a test that checks that connecting to an invalid server fails the way it should here.

public:
	void registerTests() final override
	{
		if (!haveEnvironment())
			skip("No suitable environment found, refusing to run");
		CXX_TEST(testInvalid)
		CXX_TEST(testClientType)
		CXX_TEST(testConnect)
		CXX_TEST(testCreateDB)
		CXX_TEST(testSelectDB)
		CXX_TEST(testCreateTable)
		CXX_TEST(testPreparedQuery)
		CXX_TEST(testResult)
		CXX_TEST(testPreparedResult)
		CXX_TEST(testBind)
		CXX_TEST(testDestroyDB)
		CXX_TEST(testDisconnect)
	}
};

class testMySQLValue_t final : public testsuit
{
private:
	template<typename T> void checkValue(const T &var, const T &expected)
		{ assertEqual(var, expected); }
	void checkValue(const ormDate_t &var, const ormDate_t &expected)
		{ assertTrue(var == expected); }
	void checkValue(const ormDateTime_t &var, const ormDateTime_t &expected)
		{ assertTrue(var == expected); }
	void checkValue(const ormUUID_t &var, const ormUUID_t &expected)
		{ assertTrue(var == expected); }

	template<typename T> T tryOkConversion(const mySQLValue_t &value)
	{
		try { return T(value); }
		catch (const mySQLValueError_t &error)
			{ fail(error.error()); }
		return {};
	}

	template<typename T> void tryFailConversion(const mySQLValue_t &value)
	{
		try
		{
			T v(value);
			printf("Conversion for %s: ", typeid(T).name());
			fail("Conversion succeeded (expected to fail)");
		}
		catch (const mySQLValueError_t &) { }
	}

	template<typename T> void tryOk(const mySQLValue_t value, const T expected)
	{
		assertFalse(value.isNull());
		T var = tryOkConversion<T>(value);
		checkValue(var, expected);
	}

	template<typename T> void tryIsNull(const mySQLValue_t value)
	{
		assertTrue(value.isNull());
		tryFailConversion<T>(value);
	}

	template<typename T> void tryShouldFail(const mySQLValue_t value)
	{
		assertFalse(value.isNull());
		tryFailConversion<T>(value);
	}

public:
	void testNull()
	{
		assertTrue(mySQLValue_t{}.isNull());
		assertFalse(mySQLValue_t{"", 1, MYSQL_TYPE_STRING}.isNull());
		tryIsNull<uint8_t>({});
		tryIsNull<int8_t>({});
		tryIsNull<uint16_t>({});
		tryIsNull<int16_t>({});
		tryIsNull<uint32_t>({});
		tryIsNull<int32_t>({});
		tryIsNull<uint64_t>({});
		tryIsNull<int64_t>({});
		//tryIsNull<float>({});
		//tryIsNull<double>({});
		tryIsNull<bool>({});
		tryIsNull<ormDate_t>({});
		tryIsNull<ormDateTime_t>({});
		tryIsNull<ormUUID_t>({});
	}

	void testString()
	{
		static const std::array<char, 23> testData =
		{
			'T', 'h', 'i', 's', ' ', 'i', 's', ' ',
			'\x00', '\xFF', ' ', 'o', 'n', 'l', 'y', ' ',
			'a', ' ', 't', 'e', 's', 't', '\0'
		};
		auto nullValue = mySQLValue_t{nullptr, 0, MYSQL_TYPE_VARCHAR}.asString();
		assertNull(nullValue);
		nullValue = mySQLValue_t{testData.data(), 0, MYSQL_TYPE_NULL}.asString();
		assertNull(nullValue);
		auto testStr = mySQLValue_t{testData.data(), testData.size(), MYSQL_TYPE_VARCHAR}.asString();
		assertNotNull(testStr);
		assertEqual(testStr.get(), testData.data(), testData.size());
	}

	void testUint8()
	{
		tryIsNull<uint8_t>({nullptr, 0, MYSQL_TYPE_TINY});
		tryIsNull<uint8_t>({"", 0, MYSQL_TYPE_NULL});
		tryShouldFail<uint8_t>({"", 0, MYSQL_TYPE_VARCHAR});
		tryShouldFail<uint8_t>({"", 0, MYSQL_TYPE_SHORT});
		tryShouldFail<uint8_t>({"", 0, MYSQL_TYPE_LONG});
		tryShouldFail<uint8_t>({"", 0, MYSQL_TYPE_LONGLONG});
		tryShouldFail<uint8_t>({"", 0, MYSQL_TYPE_BLOB});
		tryShouldFail<uint8_t>({"", 0, MYSQL_TYPE_DATE});
		tryShouldFail<uint8_t>({"", 0, MYSQL_TYPE_DATETIME});
		tryShouldFail<uint8_t>({"", 0, MYSQL_TYPE_BIT});
		tryShouldFail<uint8_t>({"", 0, MYSQL_TYPE_STRING});
		tryOk<uint8_t>({"128", 4, MYSQL_TYPE_TINY}, 128);
		tryOk<uint8_t>({"255", 4, MYSQL_TYPE_TINY}, 255);
		tryOk<uint8_t>({"", 1, MYSQL_TYPE_TINY}, 0);
		tryOk<uint8_t>({"", 0, MYSQL_TYPE_TINY}, 0);
		tryShouldFail<uint8_t>({"-1", 3, MYSQL_TYPE_TINY});
		tryShouldFail<uint8_t>({"a", 2, MYSQL_TYPE_TINY});
		tryShouldFail<uint8_t>({"256", 4, MYSQL_TYPE_TINY});
		tryShouldFail<uint8_t>({"1023", 5, MYSQL_TYPE_TINY});
	}

	void testInt8()
	{
		tryIsNull<int8_t>({nullptr, 0, MYSQL_TYPE_TINY});
		tryIsNull<int8_t>({"", 0, MYSQL_TYPE_NULL});
		tryShouldFail<int8_t>({"", 0, MYSQL_TYPE_VARCHAR});
		tryShouldFail<int8_t>({"", 0, MYSQL_TYPE_SHORT});
		tryShouldFail<int8_t>({"", 0, MYSQL_TYPE_LONG});
		tryShouldFail<int8_t>({"", 0, MYSQL_TYPE_LONGLONG});
		tryShouldFail<int8_t>({"", 0, MYSQL_TYPE_BLOB});
		tryShouldFail<int8_t>({"", 0, MYSQL_TYPE_DATE});
		tryShouldFail<int8_t>({"", 0, MYSQL_TYPE_DATETIME});
		tryShouldFail<int8_t>({"", 0, MYSQL_TYPE_BIT});
		tryShouldFail<int8_t>({"", 0, MYSQL_TYPE_STRING});
		tryOk<int8_t>({"127", 4, MYSQL_TYPE_TINY}, 127);
		tryOk<int8_t>({"", 1, MYSQL_TYPE_TINY}, 0);
		tryOk<int8_t>({"", 0, MYSQL_TYPE_TINY}, 0);
		tryOk<int8_t>({"-1", 3, MYSQL_TYPE_TINY}, -1);
		tryOk<int8_t>({"-127", 5, MYSQL_TYPE_TINY}, -127);
		tryOk<int8_t>({"-128", 5, MYSQL_TYPE_TINY}, -128);
		tryShouldFail<int8_t>({"a", 2, MYSQL_TYPE_TINY});
		tryShouldFail<int8_t>({"256", 4, MYSQL_TYPE_TINY});
		//tryShouldFail<int8_t>({"-129", 5, MYSQL_TYPE_TINY});
		tryShouldFail<int8_t>({"1023", 5, MYSQL_TYPE_TINY});
		tryShouldFail<int8_t>({"1-27", 5, MYSQL_TYPE_TINY});
	}

	void testUint16()
	{
		tryIsNull<uint16_t>({nullptr, 0, MYSQL_TYPE_SHORT});
		tryIsNull<uint16_t>({"", 0, MYSQL_TYPE_NULL});
		tryShouldFail<uint16_t>({"", 0, MYSQL_TYPE_VARCHAR});
		tryShouldFail<uint16_t>({"", 0, MYSQL_TYPE_TINY});
		tryShouldFail<uint16_t>({"", 0, MYSQL_TYPE_LONG});
		tryShouldFail<uint16_t>({"", 0, MYSQL_TYPE_LONGLONG});
		tryShouldFail<uint16_t>({"", 0, MYSQL_TYPE_BLOB});
		tryShouldFail<uint16_t>({"", 0, MYSQL_TYPE_DATE});
		tryShouldFail<uint16_t>({"", 0, MYSQL_TYPE_DATETIME});
		tryShouldFail<uint16_t>({"", 0, MYSQL_TYPE_BIT});
		tryShouldFail<uint16_t>({"", 0, MYSQL_TYPE_STRING});
		tryOk<uint16_t>({"128", 4, MYSQL_TYPE_SHORT}, 128);
		tryOk<uint16_t>({"255", 4, MYSQL_TYPE_SHORT}, 255);
		tryOk<uint16_t>({"32768", 6, MYSQL_TYPE_SHORT}, 32768);
		tryOk<uint16_t>({"65535", 6, MYSQL_TYPE_SHORT}, 65535);
		tryOk<uint16_t>({"", 1, MYSQL_TYPE_SHORT}, 0);
		tryOk<uint16_t>({"", 0, MYSQL_TYPE_SHORT}, 0);
		tryShouldFail<uint16_t>({"-1", 3, MYSQL_TYPE_SHORT});
		tryShouldFail<uint16_t>({"a", 2, MYSQL_TYPE_SHORT});
		tryShouldFail<uint16_t>({"65536", 6, MYSQL_TYPE_SHORT});
		tryShouldFail<uint16_t>({"262144", 7, MYSQL_TYPE_SHORT});
	}

	void testInt16()
	{
		tryIsNull<int16_t>({nullptr, 0, MYSQL_TYPE_SHORT});
		tryIsNull<int16_t>({"", 0, MYSQL_TYPE_NULL});
		tryShouldFail<int16_t>({"", 0, MYSQL_TYPE_VARCHAR});
		tryShouldFail<int16_t>({"", 0, MYSQL_TYPE_TINY});
		tryShouldFail<int16_t>({"", 0, MYSQL_TYPE_LONG});
		tryShouldFail<int16_t>({"", 0, MYSQL_TYPE_LONGLONG});
		tryShouldFail<int16_t>({"", 0, MYSQL_TYPE_BLOB});
		tryShouldFail<int16_t>({"", 0, MYSQL_TYPE_DATE});
		tryShouldFail<int16_t>({"", 0, MYSQL_TYPE_DATETIME});
		tryShouldFail<int16_t>({"", 0, MYSQL_TYPE_BIT});
		tryShouldFail<int16_t>({"", 0, MYSQL_TYPE_STRING});
		tryOk<int16_t>({"127", 4, MYSQL_TYPE_SHORT}, 127);
		tryOk<int16_t>({"32767", 6, MYSQL_TYPE_SHORT}, 32767);
		tryOk<int16_t>({"", 1, MYSQL_TYPE_SHORT}, 0);
		tryOk<int16_t>({"", 0, MYSQL_TYPE_SHORT}, 0);
		tryOk<int16_t>({"-1", 3, MYSQL_TYPE_SHORT}, -1);
		tryOk<int16_t>({"-32767", 7, MYSQL_TYPE_SHORT}, -32767);
		tryOk<int16_t>({"-32768", 7, MYSQL_TYPE_SHORT}, -32768);
		tryShouldFail<int16_t>({"a", 2, MYSQL_TYPE_SHORT});
		tryShouldFail<int16_t>({"65536", 6, MYSQL_TYPE_SHORT});
		//tryShouldFail<int16_t>({"-32769", 5, MYSQL_TYPE_SHORT});
		tryShouldFail<int16_t>({"262144", 7, MYSQL_TYPE_SHORT});
		tryShouldFail<int16_t>({"1-27", 5, MYSQL_TYPE_SHORT});
	}

	void testUint32()
	{
		tryIsNull<uint32_t>({nullptr, 0, MYSQL_TYPE_LONG});
		tryIsNull<uint32_t>({"", 0, MYSQL_TYPE_NULL});
		tryShouldFail<uint32_t>({"", 0, MYSQL_TYPE_VARCHAR});
		tryShouldFail<uint32_t>({"", 0, MYSQL_TYPE_TINY});
		tryShouldFail<uint32_t>({"", 0, MYSQL_TYPE_SHORT});
		tryShouldFail<uint32_t>({"", 0, MYSQL_TYPE_LONGLONG});
		tryShouldFail<uint32_t>({"", 0, MYSQL_TYPE_BLOB});
		tryShouldFail<uint32_t>({"", 0, MYSQL_TYPE_DATE});
		tryShouldFail<uint32_t>({"", 0, MYSQL_TYPE_DATETIME});
		tryShouldFail<uint32_t>({"", 0, MYSQL_TYPE_BIT});
		tryShouldFail<uint32_t>({"", 0, MYSQL_TYPE_STRING});
		tryOk<uint32_t>({"128", 4, MYSQL_TYPE_LONG}, 128);
		tryOk<uint32_t>({"255", 4, MYSQL_TYPE_LONG}, 255);
		tryOk<uint32_t>({"32768", 6, MYSQL_TYPE_LONG}, 32768);
		tryOk<uint32_t>({"65535", 6, MYSQL_TYPE_LONG}, 65535);
		tryOk<uint32_t>({"2147483648", 11, MYSQL_TYPE_LONG}, 2147483648);
		tryOk<uint32_t>({"4294967295", 11, MYSQL_TYPE_LONG}, 4294967295);
		tryOk<uint32_t>({"", 1, MYSQL_TYPE_LONG}, 0);
		tryOk<uint32_t>({"", 0, MYSQL_TYPE_LONG}, 0);
		tryShouldFail<uint32_t>({"-1", 3, MYSQL_TYPE_LONG});
		tryShouldFail<uint32_t>({"a", 2, MYSQL_TYPE_LONG});
		tryShouldFail<uint32_t>({"4294967296", 11, MYSQL_TYPE_LONG});
		tryShouldFail<uint32_t>({"17179869184", 12, MYSQL_TYPE_LONG});
	}

	void testInt32()
	{
		tryIsNull<int32_t>({nullptr, 0, MYSQL_TYPE_LONG});
		tryIsNull<int32_t>({"", 0, MYSQL_TYPE_NULL});
		tryShouldFail<int32_t>({"", 0, MYSQL_TYPE_VARCHAR});
		tryShouldFail<int32_t>({"", 0, MYSQL_TYPE_TINY});
		tryShouldFail<int32_t>({"", 0, MYSQL_TYPE_SHORT});
		tryShouldFail<int32_t>({"", 0, MYSQL_TYPE_LONGLONG});
		tryShouldFail<int32_t>({"", 0, MYSQL_TYPE_BLOB});
		tryShouldFail<int32_t>({"", 0, MYSQL_TYPE_DATE});
		tryShouldFail<int32_t>({"", 0, MYSQL_TYPE_DATETIME});
		tryShouldFail<int32_t>({"", 0, MYSQL_TYPE_BIT});
		tryShouldFail<int32_t>({"", 0, MYSQL_TYPE_STRING});
		tryOk<int32_t>({"127", 4, MYSQL_TYPE_LONG}, 127);
		tryOk<int32_t>({"32767", 6, MYSQL_TYPE_LONG}, 32767);
		tryOk<int32_t>({"2147483647", 11, MYSQL_TYPE_LONG}, 2147483647);
		tryOk<int32_t>({"", 1, MYSQL_TYPE_LONG}, 0);
		tryOk<int32_t>({"", 0, MYSQL_TYPE_LONG}, 0);
		tryOk<int32_t>({"-1", 3, MYSQL_TYPE_LONG}, -1);
		tryOk<int32_t>({"-2147483647", 12, MYSQL_TYPE_LONG}, -2147483647);
		tryOk<int32_t>({"-2147483648", 12, MYSQL_TYPE_LONG}, -2147483648);
		tryShouldFail<int32_t>({"a", 2, MYSQL_TYPE_LONG});
		tryShouldFail<int32_t>({"4294967296", 11, MYSQL_TYPE_LONG});
		//tryShouldFail<int32_t>({"-2147483649", 5, MYSQL_TYPE_LONG});
		tryShouldFail<int32_t>({"17179869184", 12, MYSQL_TYPE_LONG});
		tryShouldFail<int32_t>({"1-27", 5, MYSQL_TYPE_LONG});
	}

	void testUint64()
	{
		tryIsNull<uint64_t>({nullptr, 0, MYSQL_TYPE_LONGLONG});
		tryIsNull<uint64_t>({"", 0, MYSQL_TYPE_NULL});
		tryShouldFail<uint64_t>({"", 0, MYSQL_TYPE_VARCHAR});
		tryShouldFail<uint64_t>({"", 0, MYSQL_TYPE_TINY});
		tryShouldFail<uint64_t>({"", 0, MYSQL_TYPE_SHORT});
		tryShouldFail<uint64_t>({"", 0, MYSQL_TYPE_LONG});
		tryShouldFail<uint64_t>({"", 0, MYSQL_TYPE_BLOB});
		tryShouldFail<uint64_t>({"", 0, MYSQL_TYPE_DATE});
		tryShouldFail<uint64_t>({"", 0, MYSQL_TYPE_DATETIME});
		tryShouldFail<uint64_t>({"", 0, MYSQL_TYPE_BIT});
		tryShouldFail<uint64_t>({"", 0, MYSQL_TYPE_STRING});
		tryOk<uint64_t>({"128", 4, MYSQL_TYPE_LONGLONG}, 128);
		tryOk<uint64_t>({"255", 4, MYSQL_TYPE_LONGLONG}, 255);
		tryOk<uint64_t>({"32768", 6, MYSQL_TYPE_LONGLONG}, 32768);
		tryOk<uint64_t>({"65535", 6, MYSQL_TYPE_LONGLONG}, 65535);
		tryOk<uint64_t>({"2147483648", 11, MYSQL_TYPE_LONGLONG}, 2147483648);
		tryOk<uint64_t>({"4294967295", 11, MYSQL_TYPE_LONGLONG}, 4294967295);
		tryOk<uint64_t>({"9223372036854775808", 20, MYSQL_TYPE_LONGLONG}, u64(9223372036854775808));
		tryOk<uint64_t>({"18446744073709551615", 21, MYSQL_TYPE_LONGLONG}, u64(18446744073709551615));
		tryOk<uint64_t>({"", 1, MYSQL_TYPE_LONGLONG}, 0);
		tryOk<uint64_t>({"", 0, MYSQL_TYPE_LONGLONG}, 0);
		tryShouldFail<uint64_t>({"-1", 3, MYSQL_TYPE_LONGLONG});
		tryShouldFail<uint64_t>({"a", 2, MYSQL_TYPE_LONGLONG});
		tryShouldFail<uint64_t>({"18446744073709551616", 21, MYSQL_TYPE_LONGLONG});
		tryShouldFail<uint64_t>({"73786976294838206464", 21, MYSQL_TYPE_LONGLONG});
	}

	void testInt64()
	{
		tryIsNull<int64_t>({nullptr, 0, MYSQL_TYPE_LONGLONG});
		tryIsNull<int64_t>({"", 0, MYSQL_TYPE_NULL});
		tryShouldFail<int64_t>({"", 0, MYSQL_TYPE_VARCHAR});
		tryShouldFail<int64_t>({"", 0, MYSQL_TYPE_TINY});
		tryShouldFail<int64_t>({"", 0, MYSQL_TYPE_SHORT});
		tryShouldFail<int64_t>({"", 0, MYSQL_TYPE_LONG});
		tryShouldFail<int64_t>({"", 0, MYSQL_TYPE_BLOB});
		tryShouldFail<int64_t>({"", 0, MYSQL_TYPE_DATE});
		tryShouldFail<int64_t>({"", 0, MYSQL_TYPE_DATETIME});
		tryShouldFail<int64_t>({"", 0, MYSQL_TYPE_BIT});
		tryShouldFail<int64_t>({"", 0, MYSQL_TYPE_STRING});
		tryOk<int64_t>({"127", 4, MYSQL_TYPE_LONGLONG}, 127);
		tryOk<int64_t>({"32767", 6, MYSQL_TYPE_LONGLONG}, 32767);
		tryOk<int64_t>({"2147483647", 11, MYSQL_TYPE_LONGLONG}, 2147483647);
		tryOk<int64_t>({"9223372036854775807", 20, MYSQL_TYPE_LONGLONG}, i64(9223372036854775807));
		tryOk<int64_t>({"", 1, MYSQL_TYPE_LONGLONG}, 0);
		tryOk<int64_t>({"", 0, MYSQL_TYPE_LONGLONG}, 0);
		tryOk<int64_t>({"-1", 3, MYSQL_TYPE_LONGLONG}, -1);
		tryOk<int64_t>({"-9223372036854775807", 21, MYSQL_TYPE_LONGLONG}, i64(-9223372036854775807));
		tryOk<int64_t>({"-9223372036854775808", 21, MYSQL_TYPE_LONGLONG}, i64(-9223372036854775807) - 1);
		//tryShouldFail<int64_t>({"-9223372036854775808", 21, MYSQL_TYPE_LONGLONG});
		tryShouldFail<int64_t>({"a", 2, MYSQL_TYPE_LONGLONG});
		tryShouldFail<int64_t>({"18446744073709551616", 21, MYSQL_TYPE_LONGLONG});
		//tryShouldFail<int64_t>({"-9223372036854775809", 5, MYSQL_TYPE_LONGLONG});
		tryShouldFail<int64_t>({"73786976294838206464", 21, MYSQL_TYPE_LONGLONG});
		tryShouldFail<int64_t>({"1-27", 5, MYSQL_TYPE_LONGLONG});
	}

	void testBool()
	{
		tryIsNull<bool>({nullptr, 0, MYSQL_TYPE_BIT});
		tryIsNull<bool>({"", 0, MYSQL_TYPE_NULL});
		tryShouldFail<bool>({"", 0, MYSQL_TYPE_VARCHAR});
		tryShouldFail<bool>({"", 0, MYSQL_TYPE_TINY});
		tryShouldFail<bool>({"", 0, MYSQL_TYPE_SHORT});
		tryShouldFail<bool>({"", 0, MYSQL_TYPE_LONG});
		tryShouldFail<bool>({"", 0, MYSQL_TYPE_LONGLONG});
		tryShouldFail<bool>({"", 0, MYSQL_TYPE_BLOB});
		tryShouldFail<bool>({"", 0, MYSQL_TYPE_DATE});
		tryShouldFail<bool>({"", 0, MYSQL_TYPE_DATETIME});
		tryShouldFail<bool>({"", 0, MYSQL_TYPE_STRING});
		tryOk<bool>({"\x01", 2, MYSQL_TYPE_BIT}, true);
		tryOk<bool>({"\x00", 2, MYSQL_TYPE_BIT}, false);
		tryOk<bool>({"", 1, MYSQL_TYPE_BIT}, 0);
		tryOk<bool>({"", 0, MYSQL_TYPE_BIT}, 0);
	}

	void testDate()
	{
		auto nowString = formatString("%04u-%02u-%02u", now.year(), now.month(), now.day());

		tryIsNull<ormDate_t>({nullptr, 0, MYSQL_TYPE_DATE});
		tryIsNull<ormDate_t>({"", 0, MYSQL_TYPE_NULL});
		tryShouldFail<ormDate_t>({"", 0, MYSQL_TYPE_VARCHAR});
		tryShouldFail<ormDate_t>({"", 0, MYSQL_TYPE_TINY});
		tryShouldFail<ormDate_t>({"", 0, MYSQL_TYPE_SHORT});
		tryShouldFail<ormDate_t>({"", 0, MYSQL_TYPE_LONG});
		tryShouldFail<ormDate_t>({"", 0, MYSQL_TYPE_LONGLONG});
		tryShouldFail<ormDate_t>({"", 0, MYSQL_TYPE_BLOB});
		tryShouldFail<ormDate_t>({"", 0, MYSQL_TYPE_DATETIME});
		tryShouldFail<ormDate_t>({"", 0, MYSQL_TYPE_BIT});
		tryShouldFail<ormDate_t>({"", 0, MYSQL_TYPE_STRING});
		tryOk<ormDate_t>({"0000-00-00", 10, MYSQL_TYPE_DATE}, {});
		tryOk<ormDate_t>({nowString.get(), 10, MYSQL_TYPE_DATE}, now);
		tryShouldFail<ormDate_t>({"", 0, MYSQL_TYPE_DATE});
		tryShouldFail<ormDate_t>({"", 1, MYSQL_TYPE_DATE});
	}

	void testDateTime()
	{
		auto nowString = formatString("%04u-%02u-%02u %02u:%02u:%02u", now.year(), now.month(),
			now.day(), now.hour(), now.minute(), now.second());
		// MySQL doesn't store this part of the ormDateTime_t..
		now.nanoSecond(0);

		tryIsNull<ormDateTime_t>({nullptr, 0, MYSQL_TYPE_DATETIME});
		tryIsNull<ormDateTime_t>({"", 0, MYSQL_TYPE_NULL});
		tryShouldFail<ormDateTime_t>({"", 0, MYSQL_TYPE_VARCHAR});
		tryShouldFail<ormDateTime_t>({"", 0, MYSQL_TYPE_TINY});
		tryShouldFail<ormDateTime_t>({"", 0, MYSQL_TYPE_SHORT});
		tryShouldFail<ormDateTime_t>({"", 0, MYSQL_TYPE_LONG});
		tryShouldFail<ormDateTime_t>({"", 0, MYSQL_TYPE_LONGLONG});
		tryShouldFail<ormDateTime_t>({"", 0, MYSQL_TYPE_BLOB});
		tryShouldFail<ormDateTime_t>({"", 0, MYSQL_TYPE_DATE});
		tryShouldFail<ormDateTime_t>({"", 0, MYSQL_TYPE_BIT});
		tryShouldFail<ormDateTime_t>({"", 0, MYSQL_TYPE_STRING});
		tryOk<ormDateTime_t>({"0000-00-00 00:00:00", 19, MYSQL_TYPE_DATETIME}, {});
		tryOk<ormDateTime_t>({nowString.get(), 19, MYSQL_TYPE_DATETIME}, now);
		tryShouldFail<ormDateTime_t>({"", 0, MYSQL_TYPE_DATETIME});
		tryShouldFail<ormDateTime_t>({"", 1, MYSQL_TYPE_DATETIME});
	}

	void testUUID()
	{
		// Set up our UUID value.
		const auto now = systemClock_t::now().time_since_epoch();
		const uint64_t time = durationIn<milliseconds>(now);
		const auto nanoSeconds = (now - milliseconds{time}).count();
		const ormUUID_t uuid
		{
			uint32_t(time), uint16_t(time >> 32),
			uint16_t(0x1000 | ((time >> 48) & 0x0FFF)),
			uint16_t((nanoSeconds >> 14) | 0x8000), swapBytes(uint64_t{0x123456789ABCU}) >> 16
		};
		const auto uuidString = uuid.asPackedString();

		tryIsNull<ormUUID_t>({nullptr, 0, MYSQL_TYPE_STRING});
		tryIsNull<ormUUID_t>({"", 0, MYSQL_TYPE_NULL});
		tryShouldFail<ormUUID_t>({"", 0, MYSQL_TYPE_VARCHAR});
		tryShouldFail<ormUUID_t>({"", 0, MYSQL_TYPE_TINY});
		tryShouldFail<ormUUID_t>({"", 0, MYSQL_TYPE_SHORT});
		tryShouldFail<ormUUID_t>({"", 0, MYSQL_TYPE_LONG});
		tryShouldFail<ormUUID_t>({"", 0, MYSQL_TYPE_LONGLONG});
		tryShouldFail<ormUUID_t>({"", 0, MYSQL_TYPE_BLOB});
		tryShouldFail<ormUUID_t>({"", 0, MYSQL_TYPE_DATE});
		tryShouldFail<ormUUID_t>({"", 0, MYSQL_TYPE_DATETIME});
		tryShouldFail<ormUUID_t>({"", 0, MYSQL_TYPE_BIT});
		tryOk<ormUUID_t>({"00000000000000000000000000000000", 32, MYSQL_TYPE_STRING}, {});
		tryOk<ormUUID_t>({"FFFFFFFF000000000000000000000000", 32, MYSQL_TYPE_STRING}, {0xFFFFFFFF, 0, 0, 0, 0});
		tryOk<ormUUID_t>({uuidString.get(), 32, MYSQL_TYPE_STRING}, uuid);
		tryShouldFail<ormUUID_t>({"", 0, MYSQL_TYPE_STRING});
		tryShouldFail<ormUUID_t>({"", 1, MYSQL_TYPE_STRING});
		tryShouldFail<ormUUID_t>({"G0000000000000000000000000000000", 32, MYSQL_TYPE_STRING});
	}

	void testError()
	{
		const char *const unknownError = "An unknown error occured";
		assertNotEqual(mySQLValueError_t(mySQLErrorType_t::noError).error(), unknownError);
		assertNotEqual(mySQLValueError_t(mySQLErrorType_t::queryError).error(), unknownError);
		assertNotEqual(mySQLValueError_t(mySQLErrorType_t::stringError).error(), unknownError);
		assertNotEqual(mySQLValueError_t(mySQLErrorType_t::boolError).error(), unknownError);
		assertNotEqual(mySQLValueError_t(mySQLErrorType_t::uint8Error).error(), unknownError);
		assertNotEqual(mySQLValueError_t(mySQLErrorType_t::int8Error).error(), unknownError);
		assertNotEqual(mySQLValueError_t(mySQLErrorType_t::uint16Error).error(), unknownError);
		assertNotEqual(mySQLValueError_t(mySQLErrorType_t::int16Error).error(), unknownError);
		assertNotEqual(mySQLValueError_t(mySQLErrorType_t::uint32Error).error(), unknownError);
		assertNotEqual(mySQLValueError_t(mySQLErrorType_t::int32Error).error(), unknownError);
		assertNotEqual(mySQLValueError_t(mySQLErrorType_t::uint64Error).error(), unknownError);
		assertNotEqual(mySQLValueError_t(mySQLErrorType_t::int64Error).error(), unknownError);
		assertNotEqual(mySQLValueError_t(mySQLErrorType_t::dateError).error(), unknownError);
		assertNotEqual(mySQLValueError_t(mySQLErrorType_t::dateTimeError).error(), unknownError);
		assertNotEqual(mySQLValueError_t(mySQLErrorType_t::uuidError).error(), unknownError);
		assertEqual(mySQLValueError_t((mySQLErrorType_t)-1).error(), unknownError);
	}

	void registerTests() final override
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
		CXX_TEST(testBool)
		CXX_TEST(testDate)
		CXX_TEST(testDateTime)
		CXX_TEST(testUUID)
		CXX_TEST(testError)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept { registerTestClasses<testMySQLValue_t, testMySQL_t>(); }
