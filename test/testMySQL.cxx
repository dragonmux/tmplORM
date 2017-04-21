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
		tryShouldFail([]() { mySQLValue_t("1-27", 5, MYSQL_TYPE_TINY).asInt8(); });
	}

	void testUint16()
	{
		tryShouldFail([]() { mySQLValue_t(nullptr, 0, MYSQL_TYPE_SHORT).asUint16(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_NULL).asUint16(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_VARCHAR).asUint16(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_TINY).asUint16(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_LONG).asUint16(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_LONGLONG).asUint16(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_BLOB).asUint16(); });

		tryOk([this]()
		{
			assertEqual(mySQLValue_t("128", 4, MYSQL_TYPE_SHORT).asUint16(), 128);
			assertEqual(mySQLValue_t("255", 4, MYSQL_TYPE_SHORT).asUint16(), 255);
			assertEqual(mySQLValue_t("32768", 6, MYSQL_TYPE_SHORT).asUint16(), 32768);
			assertEqual(mySQLValue_t("65535", 6, MYSQL_TYPE_SHORT).asUint16(), 65535);
			assertEqual(mySQLValue_t("", 1, MYSQL_TYPE_SHORT).asUint16(), 0);
			assertEqual(mySQLValue_t("", 0, MYSQL_TYPE_SHORT).asUint16(), 0);
		});
		tryShouldFail([]() { mySQLValue_t("-1", 3, MYSQL_TYPE_SHORT).asUint16(); });
		tryShouldFail([]() { mySQLValue_t("a", 2, MYSQL_TYPE_SHORT).asUint16(); });
		tryShouldFail([]() { mySQLValue_t("65536", 6, MYSQL_TYPE_SHORT).asUint16(); });
		tryShouldFail([]() { mySQLValue_t("262144", 7, MYSQL_TYPE_SHORT).asUint16(); });
	}

	void testInt16()
	{
		tryShouldFail([]() { mySQLValue_t(nullptr, 0, MYSQL_TYPE_SHORT).asInt16(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_NULL).asInt16(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_VARCHAR).asInt16(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_TINY).asInt16(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_LONG).asInt16(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_LONGLONG).asInt16(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_BLOB).asInt16(); });

		tryOk([this]()
		{
			assertEqual(mySQLValue_t("127", 4, MYSQL_TYPE_SHORT).asInt16(), 127);
			assertEqual(mySQLValue_t("32767", 6, MYSQL_TYPE_SHORT).asInt16(), 32767);
			assertEqual(mySQLValue_t("", 1, MYSQL_TYPE_SHORT).asInt16(), 0);
			assertEqual(mySQLValue_t("", 0, MYSQL_TYPE_SHORT).asInt16(), 0);
			assertEqual(mySQLValue_t("-1", 3, MYSQL_TYPE_SHORT).asInt16(), -1);
			assertEqual(mySQLValue_t("-32767", 7, MYSQL_TYPE_SHORT).asInt16(), -32767);
			assertEqual(mySQLValue_t("-32768", 7, MYSQL_TYPE_SHORT).asInt16(), -32768);
		});
		tryShouldFail([]() { mySQLValue_t("a", 2, MYSQL_TYPE_SHORT).asInt16(); });
		tryShouldFail([]() { mySQLValue_t("65536", 6, MYSQL_TYPE_SHORT).asInt16(); });
		//tryShouldFail([]() { mySQLValue_t("-129", 5, MYSQL_TYPE_SHORT).asInt16(); });
		tryShouldFail([]() { mySQLValue_t("262144", 7, MYSQL_TYPE_SHORT).asInt16(); });
		tryShouldFail([]() { mySQLValue_t("1-27", 5, MYSQL_TYPE_SHORT).asInt16(); });
	}

	void testUint32()
	{
		tryShouldFail([]() { mySQLValue_t(nullptr, 0, MYSQL_TYPE_LONG).asUint32(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_NULL).asUint32(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_VARCHAR).asUint32(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_TINY).asUint32(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_SHORT).asUint32(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_LONGLONG).asUint32(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_BLOB).asUint32(); });

		tryOk([this]()
		{
			assertEqual(mySQLValue_t("128", 4, MYSQL_TYPE_LONG).asUint32(), 128);
			assertEqual(mySQLValue_t("255", 4, MYSQL_TYPE_LONG).asUint32(), 255);
			assertEqual(mySQLValue_t("32768", 6, MYSQL_TYPE_LONG).asUint32(), 32768);
			assertEqual(mySQLValue_t("65535", 6, MYSQL_TYPE_LONG).asUint32(), 65535);
			assertEqual(mySQLValue_t("2147483648", 11, MYSQL_TYPE_LONG).asUint32(), 2147483648);
			assertEqual(mySQLValue_t("4294967295", 11, MYSQL_TYPE_LONG).asUint32(), 4294967295);
			assertEqual(mySQLValue_t("", 1, MYSQL_TYPE_LONG).asUint32(), 0);
			assertEqual(mySQLValue_t("", 0, MYSQL_TYPE_LONG).asUint32(), 0);
		});
		tryShouldFail([]() { mySQLValue_t("-1", 3, MYSQL_TYPE_LONG).asUint32(); });
		tryShouldFail([]() { mySQLValue_t("a", 2, MYSQL_TYPE_LONG).asUint32(); });
		tryShouldFail([]() { mySQLValue_t("4294967296", 11, MYSQL_TYPE_LONG).asUint32(); });
		tryShouldFail([]() { mySQLValue_t("17179869184", 12, MYSQL_TYPE_LONG).asUint32(); });
	}

	void testInt32()
	{
		tryShouldFail([]() { mySQLValue_t(nullptr, 0, MYSQL_TYPE_LONG).asInt32(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_NULL).asInt32(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_VARCHAR).asInt32(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_TINY).asInt32(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_SHORT).asInt32(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_LONGLONG).asInt32(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_BLOB).asInt32(); });

		tryOk([this]()
		{
			assertEqual(mySQLValue_t("127", 4, MYSQL_TYPE_LONG).asInt32(), 127);
			assertEqual(mySQLValue_t("32767", 6, MYSQL_TYPE_LONG).asInt32(), 32767);
			assertEqual(mySQLValue_t("2147483647", 11, MYSQL_TYPE_LONG).asInt32(), 2147483647);
			assertEqual(mySQLValue_t("", 1, MYSQL_TYPE_LONG).asInt32(), 0);
			assertEqual(mySQLValue_t("", 0, MYSQL_TYPE_LONG).asInt32(), 0);
			assertEqual(mySQLValue_t("-1", 3, MYSQL_TYPE_LONG).asInt32(), -1);
			assertEqual(mySQLValue_t("-32767", 7, MYSQL_TYPE_LONG).asInt32(), -32767);
			assertEqual(mySQLValue_t("-32768", 7, MYSQL_TYPE_LONG).asInt32(), -32768);
		});
		tryShouldFail([]() { mySQLValue_t("a", 2, MYSQL_TYPE_LONG).asInt32(); });
		tryShouldFail([]() { mySQLValue_t("4294967296", 11, MYSQL_TYPE_LONG).asInt32(); });
		//tryShouldFail([]() { mySQLValue_t("-129", 5, MYSQL_TYPE_LONG).asInt32(); });
		tryShouldFail([]() { mySQLValue_t("17179869184", 12, MYSQL_TYPE_LONG).asInt32(); });
		tryShouldFail([]() { mySQLValue_t("1-27", 5, MYSQL_TYPE_LONG).asInt32(); });
	}

	void testUint64()
	{
		tryShouldFail([]() { mySQLValue_t(nullptr, 0, MYSQL_TYPE_LONGLONG).asUint64(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_NULL).asUint64(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_VARCHAR).asUint64(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_TINY).asUint64(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_SHORT).asUint64(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_LONG).asUint64(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_BLOB).asUint64(); });

		tryOk([this]()
		{
			assertEqual(mySQLValue_t("128", 4, MYSQL_TYPE_LONGLONG).asUint64(), 128);
			assertEqual(mySQLValue_t("255", 4, MYSQL_TYPE_LONGLONG).asUint64(), 255);
			assertEqual(mySQLValue_t("32768", 6, MYSQL_TYPE_LONGLONG).asUint64(), 32768);
			assertEqual(mySQLValue_t("65535", 6, MYSQL_TYPE_LONGLONG).asUint64(), 65535);
			assertEqual(mySQLValue_t("2147483648", 11, MYSQL_TYPE_LONGLONG).asUint64(), 2147483648);
			assertEqual(mySQLValue_t("4294967295", 11, MYSQL_TYPE_LONGLONG).asUint64(), 4294967295);
			assertEqual(mySQLValue_t("9223372036854775808", 20, MYSQL_TYPE_LONGLONG).asUint64(), 9223372036854775808U);
			assertEqual(mySQLValue_t("18446744073709551615", 21, MYSQL_TYPE_LONGLONG).asUint64(), 18446744073709551615U);
			assertEqual(mySQLValue_t("", 1, MYSQL_TYPE_LONGLONG).asUint64(), 0);
			assertEqual(mySQLValue_t("", 0, MYSQL_TYPE_LONGLONG).asUint64(), 0);
		});
		tryShouldFail([]() { mySQLValue_t("-1", 3, MYSQL_TYPE_LONGLONG).asUint64(); });
		tryShouldFail([]() { mySQLValue_t("a", 2, MYSQL_TYPE_LONGLONG).asUint64(); });
		tryShouldFail([]() { mySQLValue_t("18446744073709551616", 21, MYSQL_TYPE_LONGLONG).asUint64(); });
		tryShouldFail([]() { mySQLValue_t("73786976294838206464", 21, MYSQL_TYPE_LONGLONG).asUint64(); });
	}

	void registerTests() final override
	{
		CXX_TEST(testString)
		CXX_TEST(testUint8)
		CXX_TEST(testInt8)
		CXX_TEST(testUint16)
		CXX_TEST(testInt16)
		CXX_TEST(testUint32)
		CXX_TEST(testInt32)
		CXX_TEST(testUint64)
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
