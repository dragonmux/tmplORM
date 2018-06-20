#include <memory>
#include <string>
#include <crunch++.h>
#include <mssql.hxx>
#include <string.hxx>
#include "constString.hxx"

/*!
 * @internal
 * @file
 * @author Rachel Mant
 * @date 2017
 * @brief Unit tests for the MSSQL driver abstraction layer
 */

using namespace tmplORM::mssql::driver;

std::unique_ptr<tSQLClient_t> testClient{};
constString_t driver, host, username, password;

constexpr static uint32_t port = 1433;

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
		tSQLResult_t result = testClient->query("CREATE DATABASE [tmplORM];");
		assertTrue(result.valid());
		assertTrue(testClient->error() == tSQLExecErrorType_t::ok);
	}

	void testSelectDB()
	{
		assertTrue(testClient->valid());
		puts("Selecting database tmplORM");
		const bool selected = testClient->selectDB("tmplORM");
		if (!selected)
		{
			const auto &error = testClient->error();
			printf("DB selection failed (%u): %s\n", error.errorNum(), error.error());
			printf("\tstate code: %s\n", error.state());
		}
		assertTrue(selected);
		puts("Checking for errors");
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
		CXX_TEST(testSelectDB)

		CXX_TEST(testDestroyDB)
		CXX_TEST(testDisconnect)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testMSSQL_t>();
}
