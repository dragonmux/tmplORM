#include <functional>
#include <crunch++.h>
#include <mysql.hxx>

#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

/*!
 * @internal
 * @file
 * @author Rachel Mant
 * @date 2016-2017
 * @brief Unit tests for the MySQL driver abstraction layer
 */

using namespace tmplORM::mysql::driver;

class testMySQLValue_t final : public testsuit
{
private:
	template<typename T> T tryOkConversion(const mySQLValue_t &value)
	{
		try { return value; }
		catch (const mySQLValueError_t &error)
			{ fail(error.error()); }
		return {};
	}

	template<typename T> void tryFailConversion(const mySQLValue_t &value)
	{
		try
		{
			T v = value;
			printf("Conversion for %s: ", typeid(T).name());
			fail("Conversion succeeded (expected to fail)");
		}
		catch (const mySQLValueError_t &) { }
	}

	template<typename T> void tryOk(const mySQLValue_t &value, const T expected)
	{
		assertFalse(value.isNull());
		T var = tryOkConversion<T>(value);
		assertEqual(var, expected);
	}

	template<typename T> void tryIsNull(const mySQLValue_t &value)
	{
		assertTrue(value.isNull());
		tryFailConversion<T>(value);
	}

	template<typename T> void tryShouldFail(const mySQLValue_t &value)
	{
		assertFalse(value.isNull());
		tryFailConversion<T>(value);
	}

public:
	void testNull()
	{
		assertTrue(mySQLValue_t().isNull());
		assertFalse(mySQLValue_t("", 1, MYSQL_TYPE_STRING).isNull());
		tryIsNull<uint8_t>(mySQLValue_t());
		tryIsNull<int8_t>(mySQLValue_t());
		tryIsNull<uint16_t>(mySQLValue_t());
		tryIsNull<int16_t>(mySQLValue_t());
		tryIsNull<uint32_t>(mySQLValue_t());
		tryIsNull<int32_t>(mySQLValue_t());
		tryIsNull<uint64_t>(mySQLValue_t());
		tryIsNull<int64_t>(mySQLValue_t());
	}

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
		tryIsNull<uint8_t>(mySQLValue_t(nullptr, 0, MYSQL_TYPE_TINY));
		tryIsNull<uint8_t>(mySQLValue_t("", 0, MYSQL_TYPE_NULL));
		tryShouldFail<uint8_t>(mySQLValue_t("", 0, MYSQL_TYPE_VARCHAR));
		tryShouldFail<uint8_t>(mySQLValue_t("", 0, MYSQL_TYPE_SHORT));
		tryShouldFail<uint8_t>(mySQLValue_t("", 0, MYSQL_TYPE_LONG));
		tryShouldFail<uint8_t>(mySQLValue_t("", 0, MYSQL_TYPE_LONGLONG));
		tryShouldFail<uint8_t>(mySQLValue_t("", 0, MYSQL_TYPE_BLOB));
		tryOk<uint8_t>(mySQLValue_t("128", 4, MYSQL_TYPE_TINY), 128);
		tryOk<uint8_t>(mySQLValue_t("255", 4, MYSQL_TYPE_TINY), 255);
		tryOk<uint8_t>(mySQLValue_t("", 1, MYSQL_TYPE_TINY), 0);
		tryOk<uint8_t>(mySQLValue_t("", 0, MYSQL_TYPE_TINY), 0);
		tryShouldFail<uint8_t>(mySQLValue_t("-1", 3, MYSQL_TYPE_TINY));
		tryShouldFail<uint8_t>(mySQLValue_t("a", 2, MYSQL_TYPE_TINY));
		tryShouldFail<uint8_t>(mySQLValue_t("256", 4, MYSQL_TYPE_TINY));
		tryShouldFail<uint8_t>(mySQLValue_t("1023", 5, MYSQL_TYPE_TINY));
	}

	void testInt8()
	{
		tryIsNull<int8_t>(mySQLValue_t(nullptr, 0, MYSQL_TYPE_TINY));
		tryIsNull<int8_t>(mySQLValue_t("", 0, MYSQL_TYPE_NULL));
		tryShouldFail<int8_t>(mySQLValue_t("", 0, MYSQL_TYPE_VARCHAR));
		tryShouldFail<int8_t>(mySQLValue_t("", 0, MYSQL_TYPE_SHORT));
		tryShouldFail<int8_t>(mySQLValue_t("", 0, MYSQL_TYPE_LONG));
		tryShouldFail<int8_t>(mySQLValue_t("", 0, MYSQL_TYPE_LONGLONG));
		tryShouldFail<int8_t>(mySQLValue_t("", 0, MYSQL_TYPE_BLOB));
		tryOk<int8_t>(mySQLValue_t("127", 4, MYSQL_TYPE_TINY), 127);
		tryOk<int8_t>(mySQLValue_t("", 1, MYSQL_TYPE_TINY), 0);
		tryOk<int8_t>(mySQLValue_t("", 0, MYSQL_TYPE_TINY), 0);
		tryOk<int8_t>(mySQLValue_t("-1", 3, MYSQL_TYPE_TINY), -1);
		tryOk<int8_t>(mySQLValue_t("-127", 5, MYSQL_TYPE_TINY), -127);
		tryOk<int8_t>(mySQLValue_t("-128", 5, MYSQL_TYPE_TINY), -128);
		tryShouldFail<int8_t>(mySQLValue_t("a", 2, MYSQL_TYPE_TINY));
		tryShouldFail<int8_t>(mySQLValue_t("256", 4, MYSQL_TYPE_TINY));
		//tryShouldFail<int8_t>(mySQLValue_t("-129", 5, MYSQL_TYPE_TINY));
		tryShouldFail<int8_t>(mySQLValue_t("1023", 5, MYSQL_TYPE_TINY));
		tryShouldFail<int8_t>(mySQLValue_t("1-27", 5, MYSQL_TYPE_TINY));
	}

	void testUint16()
	{
		tryIsNull<uint16_t>(mySQLValue_t(nullptr, 0, MYSQL_TYPE_SHORT));
		tryIsNull<uint16_t>(mySQLValue_t("", 0, MYSQL_TYPE_NULL));
		tryShouldFail<uint16_t>(mySQLValue_t("", 0, MYSQL_TYPE_VARCHAR));
		tryShouldFail<uint16_t>(mySQLValue_t("", 0, MYSQL_TYPE_TINY));
		tryShouldFail<uint16_t>(mySQLValue_t("", 0, MYSQL_TYPE_LONG));
		tryShouldFail<uint16_t>(mySQLValue_t("", 0, MYSQL_TYPE_LONGLONG));
		tryShouldFail<uint16_t>(mySQLValue_t("", 0, MYSQL_TYPE_BLOB));
		tryOk<uint16_t>(mySQLValue_t("128", 4, MYSQL_TYPE_SHORT), 128);
		tryOk<uint16_t>(mySQLValue_t("255", 4, MYSQL_TYPE_SHORT), 255);
		tryOk<uint16_t>(mySQLValue_t("32768", 6, MYSQL_TYPE_SHORT), 32768);
		tryOk<uint16_t>(mySQLValue_t("65535", 6, MYSQL_TYPE_SHORT), 65535);
		tryOk<uint16_t>(mySQLValue_t("", 1, MYSQL_TYPE_SHORT), 0);
		tryOk<uint16_t>(mySQLValue_t("", 0, MYSQL_TYPE_SHORT), 0);
		tryShouldFail<uint16_t>(mySQLValue_t("-1", 3, MYSQL_TYPE_SHORT));
		tryShouldFail<uint16_t>(mySQLValue_t("a", 2, MYSQL_TYPE_SHORT));
		tryShouldFail<uint16_t>(mySQLValue_t("65536", 6, MYSQL_TYPE_SHORT));
		tryShouldFail<uint16_t>(mySQLValue_t("262144", 7, MYSQL_TYPE_SHORT));
	}

	void testInt16()
	{
		tryIsNull<int16_t>(mySQLValue_t(nullptr, 0, MYSQL_TYPE_SHORT));
		tryIsNull<int16_t>(mySQLValue_t("", 0, MYSQL_TYPE_NULL));
		tryShouldFail<int16_t>(mySQLValue_t("", 0, MYSQL_TYPE_VARCHAR));
		tryShouldFail<int16_t>(mySQLValue_t("", 0, MYSQL_TYPE_TINY));
		tryShouldFail<int16_t>(mySQLValue_t("", 0, MYSQL_TYPE_LONG));
		tryShouldFail<int16_t>(mySQLValue_t("", 0, MYSQL_TYPE_LONGLONG));
		tryShouldFail<int16_t>(mySQLValue_t("", 0, MYSQL_TYPE_BLOB));
		tryOk<int16_t>(mySQLValue_t("127", 4, MYSQL_TYPE_SHORT), 127);
		tryOk<int16_t>(mySQLValue_t("32767", 6, MYSQL_TYPE_SHORT), 32767);
		tryOk<int16_t>(mySQLValue_t("", 1, MYSQL_TYPE_SHORT), 0);
		tryOk<int16_t>(mySQLValue_t("", 0, MYSQL_TYPE_SHORT), 0);
		tryOk<int16_t>(mySQLValue_t("-1", 3, MYSQL_TYPE_SHORT), -1);
		tryOk<int16_t>(mySQLValue_t("-32767", 7, MYSQL_TYPE_SHORT), -32767);
		tryOk<int16_t>(mySQLValue_t("-32768", 7, MYSQL_TYPE_SHORT), -32768);
		tryShouldFail<int16_t>(mySQLValue_t("a", 2, MYSQL_TYPE_SHORT));
		tryShouldFail<int16_t>(mySQLValue_t("65536", 6, MYSQL_TYPE_SHORT));
		//tryShouldFail<int16_t>(mySQLValue_t("-129", 5, MYSQL_TYPE_SHORT));
		tryShouldFail<int16_t>(mySQLValue_t("262144", 7, MYSQL_TYPE_SHORT));
		tryShouldFail<int16_t>(mySQLValue_t("1-27", 5, MYSQL_TYPE_SHORT));
	}

	void testUint32()
	{
		tryIsNull<uint32_t>(mySQLValue_t(nullptr, 0, MYSQL_TYPE_LONG));
		tryIsNull<uint32_t>(mySQLValue_t("", 0, MYSQL_TYPE_NULL));
		tryShouldFail<uint32_t>(mySQLValue_t("", 0, MYSQL_TYPE_VARCHAR));
		tryShouldFail<uint32_t>(mySQLValue_t("", 0, MYSQL_TYPE_TINY));
		tryShouldFail<uint32_t>(mySQLValue_t("", 0, MYSQL_TYPE_SHORT));
		tryShouldFail<uint32_t>(mySQLValue_t("", 0, MYSQL_TYPE_LONGLONG));
		tryShouldFail<uint32_t>(mySQLValue_t("", 0, MYSQL_TYPE_BLOB));
		tryOk<uint32_t>(mySQLValue_t("128", 4, MYSQL_TYPE_LONG), 128);
		tryOk<uint32_t>(mySQLValue_t("255", 4, MYSQL_TYPE_LONG), 255);
		tryOk<uint32_t>(mySQLValue_t("32768", 6, MYSQL_TYPE_LONG), 32768);
		tryOk<uint32_t>(mySQLValue_t("65535", 6, MYSQL_TYPE_LONG), 65535);
		tryOk<uint32_t>(mySQLValue_t("2147483648", 11, MYSQL_TYPE_LONG), 2147483648);
		tryOk<uint32_t>(mySQLValue_t("4294967295", 11, MYSQL_TYPE_LONG), 4294967295);
		tryOk<uint32_t>(mySQLValue_t("", 1, MYSQL_TYPE_LONG), 0);
		tryOk<uint32_t>(mySQLValue_t("", 0, MYSQL_TYPE_LONG), 0);
		tryShouldFail<uint32_t>(mySQLValue_t("-1", 3, MYSQL_TYPE_LONG));
		tryShouldFail<uint32_t>(mySQLValue_t("a", 2, MYSQL_TYPE_LONG));
		tryShouldFail<uint32_t>(mySQLValue_t("4294967296", 11, MYSQL_TYPE_LONG));
		tryShouldFail<uint32_t>(mySQLValue_t("17179869184", 12, MYSQL_TYPE_LONG));
	}

	void testInt32()
	{
		tryIsNull<int32_t>(mySQLValue_t(nullptr, 0, MYSQL_TYPE_LONG));
		tryIsNull<int32_t>(mySQLValue_t("", 0, MYSQL_TYPE_NULL));
		tryShouldFail<int32_t>(mySQLValue_t("", 0, MYSQL_TYPE_VARCHAR));
		tryShouldFail<int32_t>(mySQLValue_t("", 0, MYSQL_TYPE_TINY));
		tryShouldFail<int32_t>(mySQLValue_t("", 0, MYSQL_TYPE_SHORT));
		tryShouldFail<int32_t>(mySQLValue_t("", 0, MYSQL_TYPE_LONGLONG));
		tryShouldFail<int32_t>(mySQLValue_t("", 0, MYSQL_TYPE_BLOB));
		tryOk<int32_t>(mySQLValue_t("127", 4, MYSQL_TYPE_LONG), 127);
		tryOk<int32_t>(mySQLValue_t("32767", 6, MYSQL_TYPE_LONG), 32767);
		tryOk<int32_t>(mySQLValue_t("2147483647", 11, MYSQL_TYPE_LONG), 2147483647);
		tryOk<int32_t>(mySQLValue_t("", 1, MYSQL_TYPE_LONG), 0);
		tryOk<int32_t>(mySQLValue_t("", 0, MYSQL_TYPE_LONG), 0);
		tryOk<int32_t>(mySQLValue_t("-1", 3, MYSQL_TYPE_LONG), -1);
		tryOk<int32_t>(mySQLValue_t("-2147483647", 12, MYSQL_TYPE_LONG), -2147483647);
		tryOk<int32_t>(mySQLValue_t("-2147483648", 12, MYSQL_TYPE_LONG), -2147483648);
		tryShouldFail<int32_t>(mySQLValue_t("a", 2, MYSQL_TYPE_LONG));
		tryShouldFail<int32_t>(mySQLValue_t("4294967296", 11, MYSQL_TYPE_LONG));
		//tryShouldFail<int32_t>(mySQLValue_t("-129", 5, MYSQL_TYPE_LONG));
		tryShouldFail<int32_t>(mySQLValue_t("17179869184", 12, MYSQL_TYPE_LONG));
		tryShouldFail<int32_t>(mySQLValue_t("1-27", 5, MYSQL_TYPE_LONG));
	}

	void testUint64()
	{
		tryIsNull<uint64_t>(mySQLValue_t(nullptr, 0, MYSQL_TYPE_LONGLONG));
		tryIsNull<uint64_t>(mySQLValue_t("", 0, MYSQL_TYPE_NULL));
		tryShouldFail<uint64_t>(mySQLValue_t("", 0, MYSQL_TYPE_VARCHAR));
		tryShouldFail<uint64_t>(mySQLValue_t("", 0, MYSQL_TYPE_TINY));
		tryShouldFail<uint64_t>(mySQLValue_t("", 0, MYSQL_TYPE_SHORT));
		tryShouldFail<uint64_t>(mySQLValue_t("", 0, MYSQL_TYPE_LONG));
		tryShouldFail<uint64_t>(mySQLValue_t("", 0, MYSQL_TYPE_BLOB));
		tryOk<uint64_t>(mySQLValue_t("128", 4, MYSQL_TYPE_LONGLONG), 128);
		tryOk<uint64_t>(mySQLValue_t("255", 4, MYSQL_TYPE_LONGLONG), 255);
		tryOk<uint64_t>(mySQLValue_t("32768", 6, MYSQL_TYPE_LONGLONG), 32768);
		tryOk<uint64_t>(mySQLValue_t("65535", 6, MYSQL_TYPE_LONGLONG), 65535);
		tryOk<uint64_t>(mySQLValue_t("2147483648", 11, MYSQL_TYPE_LONGLONG), 2147483648);
		tryOk<uint64_t>(mySQLValue_t("4294967295", 11, MYSQL_TYPE_LONGLONG), 4294967295);
		tryOk<uint64_t>(mySQLValue_t("9223372036854775808", 20, MYSQL_TYPE_LONGLONG), UINT64_C(9223372036854775808));
		tryOk<uint64_t>(mySQLValue_t("18446744073709551615", 21, MYSQL_TYPE_LONGLONG), UINT64_C(18446744073709551615));
		tryOk<uint64_t>(mySQLValue_t("", 1, MYSQL_TYPE_LONGLONG), 0);
		tryOk<uint64_t>(mySQLValue_t("", 0, MYSQL_TYPE_LONGLONG), 0);
		tryShouldFail<uint64_t>(mySQLValue_t("-1", 3, MYSQL_TYPE_LONGLONG));
		tryShouldFail<uint64_t>(mySQLValue_t("a", 2, MYSQL_TYPE_LONGLONG));
		tryShouldFail<uint64_t>(mySQLValue_t("18446744073709551616", 21, MYSQL_TYPE_LONGLONG));
		tryShouldFail<uint64_t>(mySQLValue_t("73786976294838206464", 21, MYSQL_TYPE_LONGLONG));
	}

	void testInt64()
	{
		tryIsNull<int64_t>(mySQLValue_t(nullptr, 0, MYSQL_TYPE_LONGLONG));
		tryIsNull<int64_t>(mySQLValue_t("", 0, MYSQL_TYPE_NULL));
		tryShouldFail<int64_t>(mySQLValue_t("", 0, MYSQL_TYPE_VARCHAR));
		tryShouldFail<int64_t>(mySQLValue_t("", 0, MYSQL_TYPE_TINY));
		tryShouldFail<int64_t>(mySQLValue_t("", 0, MYSQL_TYPE_SHORT));
		tryShouldFail<int64_t>(mySQLValue_t("", 0, MYSQL_TYPE_LONG));
		tryShouldFail<int64_t>(mySQLValue_t("", 0, MYSQL_TYPE_BLOB));
		tryOk<int64_t>(mySQLValue_t("127", 4, MYSQL_TYPE_LONGLONG), 127);
		tryOk<int64_t>(mySQLValue_t("32767", 6, MYSQL_TYPE_LONGLONG), 32767);
		tryOk<int64_t>(mySQLValue_t("2147483647", 11, MYSQL_TYPE_LONGLONG), 2147483647);
		tryOk<int64_t>(mySQLValue_t("9223372036854775807", 20, MYSQL_TYPE_LONGLONG), 9223372036854775807U);
		tryOk<int64_t>(mySQLValue_t("", 1, MYSQL_TYPE_LONGLONG), 0);
		tryOk<int64_t>(mySQLValue_t("", 0, MYSQL_TYPE_LONGLONG), 0);
		tryOk<int64_t>(mySQLValue_t("-1", 3, MYSQL_TYPE_LONGLONG), -1);
		tryOk<int64_t>(mySQLValue_t("-9223372036854775807", 21, MYSQL_TYPE_LONGLONG), -9223372036854775807);
		//tryShouldFail<int64_t>(mySQLValue_t("-9223372036854775808", 21, MYSQL_TYPE_LONGLONG));
		tryShouldFail<int64_t>(mySQLValue_t("a", 2, MYSQL_TYPE_LONGLONG));
		tryShouldFail<int64_t>(mySQLValue_t("18446744073709551616", 21, MYSQL_TYPE_LONGLONG));
		//tryShouldFail<int64_t>(mySQLValue_t("-129", 5, MYSQL_TYPE_LONGLONG));
		tryShouldFail<int64_t>(mySQLValue_t("73786976294838206464", 21, MYSQL_TYPE_LONGLONG));
		tryShouldFail<int64_t>(mySQLValue_t("1-27", 5, MYSQL_TYPE_LONGLONG));
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
		CXX_TEST(testNull)
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

class testMySQL_t final : public testsuit
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
		assertFalse(mysql_server_init(embeddedOptions.size(), const_cast<char **>(embeddedOptions.data()),
			const_cast<char **>(embeddedGroups.data())));
	}

	void testInvalid()
	{
		/*mySQLClient_t testClient;
		assertFalse(testClient.valid());
		assertFalse(testClient.queryResult().valid());
		mySQLPreparedQuery_t testQuery = testClient.prepare("", 0);
		assertFalse(testQuery.valid());
		assertFalse(testQuery.execute());
		assertEqual(testQuery.rowID(), 0);*/
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
		//CXX_TEST(start)
		CXX_TEST(testInvalid)
		//CXX_TEST(testClient)
		//CXX_TEST(stop)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testMySQLValue_t, testMySQL_t>();
}
