#include <crunch++.h>
#include <mssql.hxx>

/*!
 * @internal
 * @file
 * @author Rachel Mant
 * @date 2017
 * @brief Unit tests for the MSSQL driver abstraction layer
 */

using namespace tmplORM::mssql::driver;

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

	void registerTests() final override
	{
		CXX_TEST(testInvalid)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testMSSQL_t>();
}
