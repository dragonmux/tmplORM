#include <crunch++.h>
#include <mysql.hxx>

using namespace tmplORM::mysql::driver;

class testMySQLValue final : public testsuit
{
private:
	template<typename T> void assertUNull(std::unique_ptr<T> &value)
		{ assertNull(value.get()); }
	template<typename T> void assertUNotNull(std::unique_ptr<T> &value)
		{ assertNotNull(value.get()); }

	void tryShouldFail(void tests())
	{
		try
		{
			tests();
			fail("mySQLValueError_t not thrown when expected");
		}
		catch (mySQLValueError_t &) { }
	}

public:
	void testString()
	{
		const std::array<char, 23> testData =
		{
			'T', 'h', 'i', 's', ' ', 'i', 's', ' ',
			'\x00', '\xFF', ' ', 'o', 'n', 'l', 'y', ' ',
			'a', ' ', 't', 'e', 's', 't', '\0'
		};
		auto nullStr = mySQLValue_t(nullptr, 0, MYSQL_TYPE_VARCHAR).asString();
		assertUNull(nullStr);
		auto nullValue = mySQLValue_t(testData.data(), 0, MYSQL_TYPE_NULL).asString();
		assertUNull(nullValue);
		auto testStr = mySQLValue_t(testData.data(), testData.size(), MYSQL_TYPE_VARCHAR).asString();
		assertUNotNull(testStr);
		assertEqual(testStr.get(), testData.data(), testData.size());
	}

	void testUint8()
	{
		tryShouldFail([]() { auto value = mySQLValue_t(nullptr, 0, MYSQL_TYPE_TINY).asUint8(); });
		//assertUNull(nullValue);
	}

	void registerTests() final override
	{
		CXX_TEST(testString)
		CXX_TEST(testUint8)
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
