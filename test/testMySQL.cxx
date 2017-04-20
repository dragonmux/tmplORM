#include <functional>
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

	void tryOk(const std::function<void()> tests)
	{
		try
			{ tests(); }
		catch (mySQLValueError_t &err)
			{ fail(err.error()); }
	}

	void tryShouldFail(const std::function<void()> tests)
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
		auto nullValue = mySQLValue_t(nullptr, 0, MYSQL_TYPE_VARCHAR).asString();
		assertUNull(nullValue);
		nullValue = mySQLValue_t(testData.data(), 0, MYSQL_TYPE_NULL).asString();
		assertUNull(nullValue);
		auto testStr = mySQLValue_t(testData.data(), testData.size(), MYSQL_TYPE_VARCHAR).asString();
		assertUNotNull(testStr);
		assertEqual(testStr.get(), testData.data(), testData.size());
	}

	void testUint8()
	{
		tryShouldFail([]() { mySQLValue_t(nullptr, 0, MYSQL_TYPE_TINY).asUint8(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_NULL).asUint8(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_VARCHAR).asUint8(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_SHORT).asUint8(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_LONG).asUint8(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_LONGLONG).asUint8(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_BLOB).asUint8(); });

		tryOk([this]()
		{
			assertEqual(mySQLValue_t("128", 4, MYSQL_TYPE_TINY).asUint8(), 128);
			assertEqual(mySQLValue_t("255", 4, MYSQL_TYPE_TINY).asUint8(), 255);
			assertEqual(mySQLValue_t("", 1, MYSQL_TYPE_TINY).asUint8(), 0);
			assertEqual(mySQLValue_t("", 0, MYSQL_TYPE_TINY).asUint8(), 0);
		});
		tryShouldFail([]() { mySQLValue_t("-1", 3, MYSQL_TYPE_TINY).asUint8(); });
		tryShouldFail([]() { mySQLValue_t("a", 2, MYSQL_TYPE_TINY).asUint8(); });
		tryShouldFail([]() { mySQLValue_t("256", 4, MYSQL_TYPE_TINY).asUint8(); });
		tryShouldFail([]() { mySQLValue_t("1023", 5, MYSQL_TYPE_TINY).asUint8(); });
	}

	void testInt8()
	{
		tryShouldFail([]() { mySQLValue_t(nullptr, 0, MYSQL_TYPE_TINY).asInt8(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_NULL).asInt8(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_VARCHAR).asInt8(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_SHORT).asInt8(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_LONG).asInt8(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_LONGLONG).asInt8(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_BLOB).asInt8(); });

		tryOk([this]()
		{
			assertEqual(mySQLValue_t("127", 4, MYSQL_TYPE_TINY).asInt8(), 127);
			assertEqual(mySQLValue_t("", 1, MYSQL_TYPE_TINY).asInt8(), 0);
			assertEqual(mySQLValue_t("", 0, MYSQL_TYPE_TINY).asInt8(), 0);
			assertEqual(mySQLValue_t("-1", 3, MYSQL_TYPE_TINY).asInt8(), -1);
			assertEqual(mySQLValue_t("-127", 5, MYSQL_TYPE_TINY).asInt8(), -127);
			assertEqual(mySQLValue_t("-128", 5, MYSQL_TYPE_TINY).asInt8(), -128);
		});
		tryShouldFail([]() { mySQLValue_t("a", 2, MYSQL_TYPE_TINY).asInt8(); });
		tryShouldFail([]() { mySQLValue_t("256", 4, MYSQL_TYPE_TINY).asInt8(); });
		//tryShouldFail([]() { mySQLValue_t("-129", 5, MYSQL_TYPE_TINY).asInt8(); });
		tryShouldFail([]() { mySQLValue_t("1023", 5, MYSQL_TYPE_TINY).asInt8(); });
		//tryShouldFail([]() { mySQLValue_t("1-27", 5, MYSQL_TYPE_TINY).asInt8(); });
	}

	void registerTests() final override
	{
		CXX_TEST(testString)
		CXX_TEST(testUint8)
		CXX_TEST(testInt8)
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
