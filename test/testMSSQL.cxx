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
		CXX_TEST(testDisconnect)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testMSSQL_t>();
}
