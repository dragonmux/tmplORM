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

bool tSQLClient_t::connect(const char *const driver, const char *const host, const uint32_t port, const char *const user, const char *const passwd) const noexcept
	{ return connect(formatString("Driver=%s;Server=tcp:%s,%u;Trusted_Connection=no;UID=%s;PID=%s", driver, host, port, user, passwd)); }

bool tSQLClient_t::selectDB(const char *const db) const noexcept
	{ return connect(formatString("Database=%s", db)); }

		}
	}
}
