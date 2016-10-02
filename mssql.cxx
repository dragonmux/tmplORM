#define UNICODE
#include <sql.h>
#include <sqlext.h>
#include "mssql.hxx"
#include "string.hxx"

namespace tmplORM
{
	namespace mssql
	{
		namespace driver
		{
tSQLExecErrorType_t translateError(const int16_t result)
{
	if (result == SQL_NEED_DATA)
		return tSQLExecErrorType_t::needData;
	else if (result == SQL_NO_DATA)
		return tSQLExecErrorType_t::noData;
#ifdef SQL_PARAM_DATA_AVAILABLE
	else if (result == SQL_PARAM_DATA_AVAILABLE)
		return tSQLExecErrorType_t::dataAvail;
#endif
	else if (result == SQL_INVALID_HANDLE)
		return tSQLExecErrorType_t::handleInv;
	else if (result == SQL_ERROR)
		return tSQLExecErrorType_t::generalError;
	else if (result != SQL_SUCCESS && result != SQL_SUCCESS_WITH_INFO)
		return tSQLExecErrorType_t::unknown;
	return tSQLExecErrorType_t::ok;
}

tSQLClient_t::tSQLClient_t() noexcept : dbHandle(nullptr), connection(nullptr), haveConnection(false), needsCommit(false), _error()
{
	if (error(SQLAllocHandle(SQL_HANDLE_ENV, nullptr, &dbHandle), SQL_HANDLE_ENV, dbHandle) || !dbHandle)
		return;

	else if (error(SQLSetEnvAttr(dbHandle, SQL_ATTR_ODBC_VERSION, reinterpret_cast<void *>(long(SQL_OV_ODBC3)), 0), SQL_HANDLE_ENV, dbHandle))
		return;
	else if (error(SQLAllocHandle(SQL_HANDLE_DBC, dbHandle, &connection), SQL_HANDLE_DBC, connection) || !connection)
		return;
}

tSQLClient_t::~tSQLClient_t() noexcept
{
	if (haveConnection)
	{
		if (needsCommit)
			rollback();
		disconnect();
	}
	SQLFreeHandle(SQL_HANDLE_DBC, connection);
	SQLFreeHandle(SQL_HANDLE_ENV, dbHandle);
}

bool tSQLClient_t::connect(const stringPtr_t &connString) const noexcept
{
	if (!dbHandle || !connection || haveConnection)
		return false;
	auto odbcString = utf16::convert(connString.get());
	int16_t temp;

	return !error(SQLBrowseConnect(connection, odbcString, utf16::length(odbcString), nullptr, 0, &temp), SQL_HANDLE_DBC, connection);
}

bool tSQLClient_t::connect(const char *const driver, const char *const host, const uint32_t port, const char *const user, const char *const passwd) const noexcept
	{ return connect(formatString("Driver=%s;Server=tcp:%s,%u;Trusted_Connection=no;UID=%s;PID=%s", driver, host, port, user, passwd)); }

bool tSQLClient_t::selectDB(const char *const db) const noexcept
	{ return connect(formatString("Database=%s", db)); }

void tSQLClient_t::disconnect() const noexcept
{
	if (haveConnection)
		haveConnection = error(SQLDisconnect(connection), SQL_HANDLE_DBC, connection);
}

		}
	}
}
