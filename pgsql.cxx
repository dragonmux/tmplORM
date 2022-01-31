#include <utility>
#include <substrate/index_sequence>
#include <substrate/buffer_utils>
// AAAAAAAAGGGGHHH.. this should be in the libpq headers, but no distro puts it where they should.
#include <catalog/pg_type_d.h>
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
	{
		if (needsCommit)
			rollback();
		PQfinish(connection);
	}
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

bool pgSQLClient_t::beginTransact() noexcept
{
	if (needsCommit || !valid())
		return false;
	const auto result{query("BEGIN")};
	needsCommit = result.errorNum() == PGRES_COMMAND_OK;
	return needsCommit;
}

bool pgSQLClient_t::endTransact(const bool commitSuccess) noexcept
{
	if (needsCommit && valid())
	{
		const auto result{query(commitSuccess ? "COMMIT" : "ROLLBACK")};
		needsCommit = result.errorNum() != PGRES_COMMAND_OK;
	}
	return !needsCommit;
}

pgSQLResult_t pgSQLClient_t::query(const char *const queryStmt) const noexcept
	{ return {PQexec(connection, queryStmt)}; }

pgSQLResult_t::pgSQLResult_t(PGresult *res) noexcept : result{res}
{
	if (!result)
		return;
	fields = static_cast<uint32_t>(PQnfields(result));
	fieldInfo = {fields};
	if (!fieldInfo.valid())
		return;
	for (const auto i : substrate::indexSequence_t{fields})
		// This would use fieldInfo[i] but that can throw.
		fieldInfo.data()[i] = PQftype(result, static_cast<int>(i));
}

pgSQLResult_t::~pgSQLResult_t() noexcept
{
	if (result)
		PQclear(result);
}

pgSQLResult_t &pgSQLResult_t::operator =(pgSQLResult_t &&res) noexcept
{
	swap(res);
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

bool pgSQLResult_t::next() noexcept
{
	++row;
	return false;
}

pgSQLValue_t pgSQLResult_t::operator [](const uint32_t idx) const noexcept
{
	if (idx > fields || !valid())
		return {};
	else if (PQgetisnull(result, static_cast<int>(row), static_cast<int>(idx)) == 1)
		return {nullptr};
	else
		// This would use fieldInfo[i] but that can throw.
		return {PQgetvalue(result, static_cast<int>(row), static_cast<int>(idx)), fieldInfo.data()[idx]};
}

void pgSQLResult_t::swap(pgSQLResult_t &res) noexcept
{
	std::swap(result, res.result);
	std::swap(fields, res.fields);
	std::swap(row, res.row);
	fieldInfo.swap(res.fieldInfo);
}

pgSQLValue_t::pgSQLValue_t(std::nullptr_t) noexcept : type{ANYOID} { }

template<typename T> inline T pgSQLValue_t::reinterpret() const noexcept
{
	T value{};
	memcpy(&value, data, sizeof(T));
	return value;
}

template<typename T, Oid oid, pgSQLErrorType_t error> inline T pgSQLValue_t::asInt() const
{
	if (isNull() || type != oid)
		throw pgSQLValueError_t{error};
	// Postgres guarantees "network byte order" (big endian)
	return substrate::buffer_utils::readBE<T>(data);
}

bool pgSQLValue_t::asBool() const
{
	if (isNull() || type != BOOLOID)
		throw pgSQLValueError_t{pgSQLErrorType_t::boolError};
	const auto value = reinterpret<uint8_t>();
	return value != 0;
}

uint8_t pgSQLValue_t::asUint8() const
	{ return static_cast<uint8_t>(asInt<uint16_t, INT2OID, pgSQLErrorType_t::uint8Error>()); }
int8_t pgSQLValue_t::asInt8() const
	{ return static_cast<int8_t>(asInt<int16_t, INT2OID, pgSQLErrorType_t::int8Error>()); }
uint16_t pgSQLValue_t::asUint16() const
	{ return asInt<uint16_t, INT2OID, pgSQLErrorType_t::uint16Error>(); }
int16_t pgSQLValue_t::asInt16() const
	{ return asInt<int16_t, INT2OID, pgSQLErrorType_t::int16Error>(); }
uint32_t pgSQLValue_t::asUint32() const
	{ return asInt<uint32_t, INT4OID, pgSQLErrorType_t::uint32Error>(); }
int32_t pgSQLValue_t::asInt32() const
	{ return asInt<int32_t, INT4OID, pgSQLErrorType_t::int32Error>(); }
uint64_t pgSQLValue_t::asUint64() const
	{ return asInt<uint64_t, INT8OID, pgSQLErrorType_t::uint64Error>(); }
int64_t pgSQLValue_t::asInt64() const
	{ return asInt<int64_t, INT8OID, pgSQLErrorType_t::int64Error>(); }

const char *pgSQLValueError_t::error() const noexcept
{
	switch (errorType)
	{
		case pgSQLErrorType_t::noError:
			return "No error occured";
		case pgSQLErrorType_t::stringError:
			return "Error converting value to a string";
		case pgSQLErrorType_t::boolError:
			return "Error converting value to a boolean";
		case pgSQLErrorType_t::uint8Error:
			return "Error converting value to an unsigned 8-bit integer";
		case pgSQLErrorType_t::int8Error:
			return "Error converting value to a signed 8-bit integer";
		case pgSQLErrorType_t::uint16Error:
			return "Error converting value to an unsigned 16-bit integer";
		case pgSQLErrorType_t::int16Error:
			return "Error converting value to a signed 16-bit integer";
		case pgSQLErrorType_t::uint32Error:
			return "Error converting value to an unsigned 32-bit integer";
		case pgSQLErrorType_t::int32Error:
			return "Error converting value to a signed 32-bit integer";
		case pgSQLErrorType_t::uint64Error:
			return "Error converting value to an unsigned 64-bit integer";
		case pgSQLErrorType_t::int64Error:
			return "Error converting value to a signed 64-bit integer";
		case pgSQLErrorType_t::floatError:
			return "Error converting value to a single-precision floating point number";
		case pgSQLErrorType_t::doubleError:
			return "Error converting value to a double-precision floating point number";
		case pgSQLErrorType_t::binError:
			return "Error converting value to binary buffer";
		case pgSQLErrorType_t::dateError:
			return "Error converting value to a date quantity";
		case pgSQLErrorType_t::dateTimeError:
			return "Error converting value to a date and time quantity";
		case pgSQLErrorType_t::uuidError:
			return "Error converting value to a UUID";
	}
	return "An unknown error occured";
}
