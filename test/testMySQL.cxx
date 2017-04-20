#include <crunch++.h>
#include <mysql.hxx>

using namespace tmplORM::mysql::driver;

class testMySQLValue final : public testsuit
{
public:
	void testString()
	{
		assertTrue(mySQLValue_t(nullptr, 0, MYSQL_TYPE_VARCHAR).isNull());
	}

	void registerTests() final override
	{
		CXX_TEST(testString)
	}
};

class testMySQL final : public testsuit
{
public:
	void registerTests() final override
	{
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testMySQLValue, testMySQL>();
}
