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
		assertNull(testClient.get());
		testClient = makeUnique<tSQLClient_t>();
		assertNotNull(testClient.get());
		assertFalse(testClient->valid());
		const bool selected = testClient->selectDB("master");
		if (!selected)
		{
			const auto &error = testClient->error();
			printf("DB selection failed (%u): %s\n", error.errorNum(), error.error());
			printf("\tstate code: %s\n", error.state());
		}
		const bool connected = testClient->connect(driver, host, port, username, password);
		if (!connected)
		{
			const auto &error = testClient->error();
			printf("Connection failed (%u): %s\n", error.errorNum(), error.error());
			printf("\tstate code: %s\n", error.state());
		}
		assertTrue(connected);
		assertTrue(testClient->error() == tSQLExecErrorType_t::ok);
		assertTrue(testClient->valid());
	}

	void testCreateDB()
	{
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

	void testDestroyDB()
	{
		assertTrue(testClient->valid());
		tSQLResult_t result = testClient->query("DROP DATABASE [tmplORM];");
		assertTrue(result.valid());
		assertTrue(testClient->error() == tSQLExecErrorType_t::ok);
	}

	void testDisconnect()
	{
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
//		CXX_TEST(testDisconnect)
//		CXX_TEST(testSelectDB)

		CXX_TEST(testDestroyDB)
		CXX_TEST(testDisconnect)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testMSSQL_t>();
}
