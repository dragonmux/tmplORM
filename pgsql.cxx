#include <utility>
#include "pgsql.hxx"

/*!
 * @internal
 * @file
 * @author Rachel Mant
 * @date 2022
 * @brief C++ PostgreSQL driver abstraction layer for handling client connections and query datasets
 */

using namespace tmplORM::pgsql::driver;

pgSQLClient_t::pgSQLClient_t(pgSQLClient_t &&con) noexcept : pgSQLClient_t()
	{ std::swap(connection, con.connection); }

pgSQLClient_t::~pgSQLClient_t() noexcept
{
	if (connection)
		PQfinish(connection);
}

void pgSQLClient_t::operator=(pgSQLClient_t &&con) noexcept
{
	std::swap(connection, con.connection);
}

bool pgSQLClient_t::connect(const char *const host, const char *const port, const char *const user,
	const char *const passwd, const char *const db) noexcept
{
	if (valid())
		return false;
	connection = PQsetdbLogin(host, port, nullptr, nullptr, db, user, passwd);
	return valid();
}

pgSQLResult_t pgSQLClient_t::query(const char *const queryStmt) const noexcept
	{ return {PQexec(connection, queryStmt)}; }

pgSQLResult_t::pgSQLResult_t(pgSQLResult_t &&res) noexcept : pgSQLResult_t()
	{ std::swap(result, res.result); }

pgSQLResult_t::~pgSQLResult_t() noexcept
{
	if (valid())
		PQclear(result);
}

pgSQLResult_t &pgSQLResult_t::operator =(pgSQLResult_t &&res) noexcept
{
	std::swap(result, res.result);
	return *this;
}

uint32_t pgSQLResult_t::errorNum() const noexcept
	{ return result ? PQresultStatus(result) : PGRES_COMMAND_OK; }
const char *pgSQLResult_t::error() const noexcept
	{ return result ? PQresultErrorMessage(result) : nullptr; }

bool pgSQLResult_t::successful() const noexcept
{
	const auto result{errorNum()};
	return result == PGRES_COMMAND_OK || result == PGRES_TUPLES_OK || result == PGRES_SINGLE_TUPLE;
}

uint32_t pgSQLResult_t::numRows() const noexcept { return valid() ? static_cast<uint32_t>(PQntuples(result)) : 0; }
