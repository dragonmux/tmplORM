#include <functional>
#include <crunch++.h>
#include <mysql.hxx>

#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

using namespace tmplORM::mysql::driver;

class testMySQLValue final : public testsuit
{
private:
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
		assertNull(nullValue);
		nullValue = mySQLValue_t(testData.data(), 0, MYSQL_TYPE_NULL).asString();
		assertNull(nullValue);
		auto testStr = mySQLValue_t(testData.data(), testData.size(), MYSQL_TYPE_VARCHAR).asString();
		assertNotNull(testStr);
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
			assertEqual(mySQLValue_t("-2147483647", 12, MYSQL_TYPE_LONG).asInt32(), -2147483647);
			assertEqual(mySQLValue_t("-2147483648", 12, MYSQL_TYPE_LONG).asInt32(), -2147483648);
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

	void testInt64()
	{
		tryShouldFail([]() { mySQLValue_t(nullptr, 0, MYSQL_TYPE_LONGLONG).asInt64(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_NULL).asInt64(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_VARCHAR).asInt64(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_TINY).asInt64(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_SHORT).asInt64(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_LONG).asInt64(); });
		tryShouldFail([]() { mySQLValue_t("", 0, MYSQL_TYPE_BLOB).asInt64(); });

		tryOk([this]()
		{
			assertEqual(mySQLValue_t("127", 4, MYSQL_TYPE_LONGLONG).asInt64(), 127);
			assertEqual(mySQLValue_t("32767", 6, MYSQL_TYPE_LONGLONG).asInt64(), 32767);
			assertEqual(mySQLValue_t("2147483647", 11, MYSQL_TYPE_LONGLONG).asInt64(), 2147483647);
			assertEqual(mySQLValue_t("9223372036854775807", 20, MYSQL_TYPE_LONGLONG).asInt64(), 9223372036854775807U);
			assertEqual(mySQLValue_t("", 1, MYSQL_TYPE_LONGLONG).asInt64(), 0);
			assertEqual(mySQLValue_t("", 0, MYSQL_TYPE_LONGLONG).asInt64(), 0);
			assertEqual(mySQLValue_t("-1", 3, MYSQL_TYPE_LONGLONG).asInt64(), -1);
			assertEqual(mySQLValue_t("-9223372036854775807", 21, MYSQL_TYPE_LONGLONG).asInt64(), -9223372036854775807);
			assertEqual(mySQLValue_t("-9223372036854775808", 21, MYSQL_TYPE_LONGLONG).asInt64(), -9223372036854775808);
		});
		tryShouldFail([]() { mySQLValue_t("a", 2, MYSQL_TYPE_LONGLONG).asInt64(); });
		tryShouldFail([]() { mySQLValue_t("18446744073709551616", 21, MYSQL_TYPE_LONGLONG).asInt64(); });
		//tryShouldFail([]() { mySQLValue_t("-129", 5, MYSQL_TYPE_LONGLONG).asInt64(); });
		tryShouldFail([]() { mySQLValue_t("73786976294838206464", 21, MYSQL_TYPE_LONGLONG).asInt64(); });
		tryShouldFail([]() { mySQLValue_t("1-27", 5, MYSQL_TYPE_LONGLONG).asInt64(); });
	}

	void testError()
	{
		const char *const unknownError = "An unknown error occured";
		assertNotEqual(mySQLValueError_t(mySQLErrorType_t::noError).error(), unknownError);
		assertNotEqual(mySQLValueError_t(mySQLErrorType_t::queryError).error(), unknownError);
		assertNotEqual(mySQLValueError_t(mySQLErrorType_t::stringError).error(), unknownError);
		assertNotEqual(mySQLValueError_t(mySQLErrorType_t::boolError).error(), unknownError);
		assertNotEqual(mySQLValueError_t(mySQLErrorType_t::uint8Error).error(), unknownError);
		assertNotEqual(mySQLValueError_t(mySQLErrorType_t::int8Error).error(), unknownError);
		assertNotEqual(mySQLValueError_t(mySQLErrorType_t::uint16Error).error(), unknownError);
		assertNotEqual(mySQLValueError_t(mySQLErrorType_t::int16Error).error(), unknownError);
		assertNotEqual(mySQLValueError_t(mySQLErrorType_t::uint32Error).error(), unknownError);
		assertNotEqual(mySQLValueError_t(mySQLErrorType_t::int32Error).error(), unknownError);
		assertNotEqual(mySQLValueError_t(mySQLErrorType_t::uint64Error).error(), unknownError);
		assertNotEqual(mySQLValueError_t(mySQLErrorType_t::int64Error).error(), unknownError);
		assertEqual(mySQLValueError_t((mySQLErrorType_t)-1).error(), unknownError);
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
		CXX_TEST(testInt64)
		CXX_TEST(testError)
	}
};

class testMySQL final : public testsuit
{
	const std::array<const char *const, 3> embeddedOptions =
		{{"mysql_tests", "--defaults-file=test.ini", nullptr}};
	const std::array<const char *const, 3> embeddedGroups =
		{{"libmysqld_server", "libmysqld_client", nullptr}};

	void deleteDir(const char *const dir)
	{
		DIR *dataDir = opendir(dir);
		if (!dataDir)
			return;
		dirent *file;
		do
		{
			file = readdir(dataDir);
			if (!file)
				break;
			unlink(file->d_name);
		}
		while (file);
		closedir(dataDir);
		rmdir(dir);
	}

public:
	void start()
	{
		mysql_server_init(embeddedOptions.size(), const_cast<char **>(embeddedOptions.data()), const_cast<char **>(embeddedGroups.data()));
	}

	void testInvalid()
	{
		mySQLClient_t testClient;
		assertFalse(testClient.valid());
		assertFalse(testClient.queryResult().valid());
		mySQLPreparedQuery_t testQuery = testClient.prepare("", 0);
		assertFalse(testQuery.valid());
		assertFalse(testQuery.execute());
		assertEqual(testQuery.rowID(), 0);
		mySQLResult_t testResult;
		assertFalse(testResult.valid());
		assertEqual(testResult.numRows(), 0);
		assertFalse(testResult.resultRows().valid());
		mySQLRow_t testRow;
		assertFalse(testRow.valid());
		assertEqual(testRow.numFields(), 0);
		assertFalse(testRow.next());
		assertTrue(testRow[0].isNull());
	}

	void testClient()
	{
		mySQLClient_t client1;
		assertFalse(client1.valid());
		mySQLClient_t client2(client1);
		assertFalse(client1.valid());
		mySQLClient_t client3;
		assertFalse(client3.valid());
		client3 = client2;
		assertFalse(client3.valid());
	}

	void stop()
	{
		mysql_server_end();
		deleteDir("data");
		deleteDir("english");
	}

	void registerTests() final override
	{
		CXX_TEST(start)
		CXX_TEST(testInvalid)
		CXX_TEST(testClient)
		CXX_TEST(stop)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testMySQLValue, testMySQL>();
}
