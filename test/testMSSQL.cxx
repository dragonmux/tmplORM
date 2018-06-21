#include <memory>
#include <string>
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

class testMSSQL_t final : public testsuit
{
public:
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
			printf("DB selection failed (%u): %s\n", error.errorNum(), error.error());
			printf("\tstate code: %s\n", error.state());
		}
		const bool connected = client.connect(driver, host, port, username, password);
		if (!connected)
		{
			const auto &error = client.error();
			printf("Connection failed (%u): %s\n", error.errorNum(), error.error());
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
			printf("Query failed (%u): %s\n", error.errorNum(), error.error());
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
			printf("DB selection failed (%u): %s\n", error.errorNum(), error.error());
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
			"[When] DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP);"
		);
		if (!result.valid())
		{
			const auto &error = testClient->error();
			printf("Query failed (%u): %s\n", error.errorNum(), error.error());
			printf("\tstate code: %s\n", error.state());
		}
		assertTrue(result.valid());
		assertTrue(testClient->error() == tSQLExecErrorType_t::ok);
	}

	void testPrepared() try
	{
		assertNotNull(testClient);
		assertTrue(testClient->valid());
		tSQLQuery_t query = testClient->prepare(
			"INSERT INTO [tmplORM] ([Name], [Value]) "
			"OUTPUT INSERTED.[EntryID] VALUES (?, ?)", 2
		);
		assertTrue(query.valid());

		query.bind(0, testData[0].name.value(), fieldLength(testData[0].name));
		query.bind(1, testData[0].value.value(), fieldLength(testData[0].value));
		tSQLResult_t result = query.execute();
		assertTrue(result.valid());

		assertTrue(result.hasData());
		assertEqual(result.numRows(), 0);
		assertEqual(result.numFields(), 1);
		// XXX: This should but doesn't work due to a strange edge-case in msodbcsql17
		// The edge case causes the first tSQLValue_t to construct properly, via a call to SQLGetData()
		// but when the second index to the same data occurs, the underlying driver always returns
		// SQL NULL - we need to cache the constructed tSQLValue_t in the tSQLResult_t and clean the cache
		// on each .next()
		//assertFalse(result[0].isNull());
		//testData[0].entryID = result[0];
		tSQLValue_t value = result[0];
		assertFalse(value.isNull());
		testData[0].entryID = value;
		assertEqual(testData[0].entryID, 1);
	}
	catch (const tSQLValueError_t &error)
	{
		puts(error.error());
		fail("Exception thrown while converting value");
	}

	void testResult()
	{
		assertNotNull(testClient);
		assertTrue(testClient->valid());
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
			printf("Query failed (%u): %s\n", error.errorNum(), error.error());
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
