#include <memory>
#include <string>
#include <chrono>
#include <crunch++.h>
#include <mssql.hxx>
#include <string.hxx>
#include <tmplORM.mssql.hxx>
#include "constString.hxx"

/*!
 * @internal
 * @file
 * @author Rachel Mant
 * @date 2017
 * @brief Unit tests for the MSSQL driver abstraction layer
 */

using namespace tmplORM::mssql::driver;
using irqus::typestring;
using tmplORM::mssql::fieldLength;
using tmplORM::types::baseTypes::ormDateTime_t;

using systemClock_t = std::chrono::system_clock;

std::unique_ptr<tSQLClient_t> testClient{};
constString_t driver, host, username, password;

constexpr static uint32_t port = 1433;

struct data_t
{
	tmplORM::types::int32_t<typestring<>> entryID;
	tmplORM::types::unicode_t<typestring<>, 50> name;
	tmplORM::types::nullable_t<tmplORM::types::int32_t<typestring<>>> value;
	tmplORM::types::dateTime_t<typestring<>> when;
};

std::array<data_t, 2> testData
{
	data_t{0, "kevin", 50, {}},
	data_t{0, "dave", nullptr, {}}
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

ormDateTime_t now = systemClock_t::now();

class testMSSQL_t final : public testsuit
{
private:
	void printError(const char *prefix, const tSQLExecError_t &error) const noexcept
	{
		const auto errorNum = uint8_t(error.errorNum());
		printf("%s failed (%u): %s\n", prefix, errorNum, error.error());
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
		{
			const auto &error = client.error();
			printError("DB selection", error);
			printf("\tstate code: %s\n", error.state());
		}
		const bool connected = client.connect(driver, host, port, username, password);
		if (!connected)
		{
			const auto &error = client.error();
			printError("Connection", error);
			printf("\tstate code: %s\n", error.state());
		}
		assertTrue(connected);
		assertTrue(client.error() == tSQLExecErrorType_t::ok);
		assertTrue(client.valid());

		testClient = makeUnique<tSQLClient_t>();
		assertNotNull(testClient);
		assertFalse(testClient->valid());
		*testClient = std::move(client);
		assertFalse(client.valid());
		assertTrue(testClient->valid());
	}

	void testCreateDB()
	{
		assertNotNull(testClient);
		assertTrue(testClient->valid());
		tSQLResult_t result = testClient->query("CREATE DATABASE [tmplORM] COLLATE latin1_general_100_CI_AI_SC;");
		if (!result.valid())
		{
			const auto &error = testClient->error();
			printError("Query", error);
			printf("\tstate code: %s\n", error.state());
		}
		assertTrue(result.valid());
		assertTrue(testClient->error() == tSQLExecErrorType_t::ok);
	}

	void testSelectDB()
	{
		assertNotNull(testClient);
		assertTrue(testClient->valid());
		const bool selected = testClient->selectDB("tmplORM");
		if (!selected)
		{
			const auto &error = testClient->error();
			printError("DB selection", error);
			printf("\tstate code: %s\n", error.state());
		}
		assertTrue(selected);
		assertTrue(testClient->error() == tSQLExecErrorType_t::ok);
	}

	void testCreateTable()
	{
		assertNotNull(testClient);
		assertTrue(testClient->valid());
		tSQLResult_t result = testClient->query(
			"CREATE TABLE [tmplORM] ("
			"[EntryID] INT NOT NULL PRIMARY KEY IDENTITY, "
			"[Name] NVARCHAR(50) NOT NULL, "
			"[Value] INT NULL, "
			"[When] DATETIME2 NOT NULL DEFAULT CURRENT_TIMESTAMP);"
		);
		if (!result.valid())
		{
			const auto &error = testClient->error();
			printError("Query", error);
			printf("\tstate code: %s\n", error.state());
		}
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
		{
			const auto &error = testClient->error();
			printError("Bind", error);
			printf("\tstate code: %s\n", error.state());
		}
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
		assertTrue(result.valid());
		//assertEqual(result.numRows(), 2);
		assertEqual(result.numFields(), 4);

		assertEqual(result[0], testData[0].entryID);
		assertEqual(result[1], testData[0].name);
		assertFalse(result[2].isNull());
		assertEqual(result[2], testData[0].value);
		testData[0].when = result[3].asDateTime();
		auto when = testData[0].when.value();
		assertNotEqual(when.year(), 0);
		assertNotEqual(when.month(), 0);
		assertNotEqual(when.day(), 0);
		assertNotEqual(when.hour(), 0);
		assertNotEqual(when.minute(), 0);
		assertNotEqual(when.second(), 0);
		assertTrue(result.next());

		assertEqual(result[0], testData[1].entryID);
		assertEqual(result[1], testData[1].name);
		assertTrue(testData[1].value.isNull());
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

	void testDestroyDB()
	{
		assertNotNull(testClient);
		assertTrue(testClient->valid());
		assertTrue(testClient->selectDB("master"));
		tSQLResult_t result = testClient->query("DROP DATABASE [tmplORM];");
		if (!result.valid())
		{
			const auto &error = testClient->error();
			printError("Query", error);
			printf("\tstate code: %s\n", error.state());
		}
		assertTrue(result.valid());
		assertTrue(testClient->error() == tSQLExecErrorType_t::ok);
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

public:
	void registerTests() final override
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
		CXX_TEST(testDestroyDB)
		CXX_TEST(testDisconnect)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testMSSQL_t>();
}
