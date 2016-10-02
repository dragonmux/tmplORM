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
