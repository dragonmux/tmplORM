#include <crunch++.h>
#include <tmplORM.mssql.hxx>
#include "models.hxx"

class testMSSQLMapper final : public testsuite
{
public:
	void testInsertGen()
	{
	}

	void testUpdateGen()
	{
	}

	void registerTests() final override
	{
		CXX_TEST(testInsertGen)
		CXX_TEST(testUpdateGen)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testMSSQLMapper>();
}
