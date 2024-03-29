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

constexpr static int32_t postgresDateEpoch{2451545};

pgSQLClient_t::pgSQLClient_t(pgSQLClient_t &&con) noexcept : pgSQLClient_t()
	{ std::swap(connection, con.connection); }
pgSQLClient_t::~pgSQLClient_t() noexcept { disconnect(); }

void pgSQLClient_t::operator=(pgSQLClient_t &&con) noexcept
{
	std::swap(connection, con.connection);
	std::swap(needsCommit, con.needsCommit);
}

bool pgSQLClient_t::connect(const char *const host, const char *const port, const char *const user,
	const char *const passwd, const char *const db) noexcept
{
	if (valid())
		return false;
	connection = PQsetdbLogin(host, port, nullptr, nullptr, db, user, passwd);
	if (PQstatus(connection) != CONNECTION_OK)
		disconnect();
	return valid();
}

void pgSQLClient_t::disconnect() noexcept
{
	if (connection)
	{
		if (needsCommit)
			rollback();
		PQfinish(connection);
		connection = nullptr;
	}
}

bool pgSQLClient_t::switchDB(const char *const db) noexcept
{
	if (!valid())
		return false;
	auto *const settings{PQconninfo(connection)};
	if (!settings)
		return false;
	disconnect();
	const auto findSetting{[&](const char *const name) -> const char *
	{
		for (size_t i{}; settings[i].keyword != nullptr; ++i)
		{
			if (strcmp(settings[i].keyword, name) == 0)
				return settings[i].val;
		}
		return nullptr;
	}};
	const auto *const host{findSetting("host")};
	const auto *const port{findSetting("port")};
	const auto *const user{findSetting("user")};
	const auto *const passwd{findSetting("password")};
	const auto result{connect(host, port, user, passwd, db)};
	PQconninfoFree(settings);
	return result;
}

bool pgSQLClient_t::beginTransact() noexcept
{
	if (!valid() || needsCommit)
		return false;
	const auto result{query("BEGIN")};
	needsCommit = result.errorNum() == PGRES_COMMAND_OK;
	return needsCommit;
}

bool pgSQLClient_t::endTransact(const bool commitSuccess) noexcept
{
	if (valid() && needsCommit)
	{
		const auto result{query(commitSuccess ? "COMMIT" : "ROLLBACK")};
		needsCommit = result.errorNum() != PGRES_COMMAND_OK;
	}
	return !needsCommit;
}

pgSQLResult_t pgSQLClient_t::query(const char *const queryStmt) const noexcept
	{ return {PQexecParams(connection, queryStmt, 0, nullptr, nullptr, nullptr, nullptr, 1)}; }

pgSQLQuery_t pgSQLClient_t::prepare(const char *const queryStmt, const size_t paramsCount) const noexcept
{
	if (!valid() || !queryStmt)
		return {};
	return {connection, queryStmt, paramsCount};
}

const char *pgSQLClient_t::error() const noexcept
	{ return valid() ? PQerrorMessage(connection) : nullptr; }

pgSQLQuery_t::pgSQLQuery_t(PGconn *const conn, const char *const queryStmt, const size_t paramsCount) noexcept :
	connection{conn}, query{queryStmt}, numParams{paramsCount}, paramTypes{paramsCount},
	params{paramsCount}, paramStorage{paramsCount}, dataLengths{paramsCount} { }

pgSQLResult_t pgSQLQuery_t::execute() const noexcept
{
	if (!valid() || numParams > static_cast<size_t>(std::numeric_limits<int>::max()))
		return {};
	fixedVector_t<int> paramFormats{numParams};
	if (!paramFormats)
		return {};
	// Postgres uses 1 to indicate binary, and 0 for string.
	for (auto &format : paramFormats)
		format = 1;
	return {PQexecParams(connection, query, static_cast<int>(numParams), paramTypes.data(),
		params.data(), dataLengths.data(), paramFormats.data(), 1)};
}

Oid pgSQLQuery_t::typeToOID(pgSQLType_t type) noexcept
{
	switch (type)
	{
		case pgSQLType_t::boolean:
			return BOOLOID;
		case pgSQLType_t::int2:
			return INT2OID;
		case pgSQLType_t::int4:
			return INT4OID;
		case pgSQLType_t::int8:
			return INT8OID;
		case pgSQLType_t::float4:
			return FLOAT4OID;
		case pgSQLType_t::float8:
			return FLOAT8OID;
		case pgSQLType_t::unicode:
			return VARCHAROID;
		case pgSQLType_t::unicodeText:
			return TEXTOID;
		case pgSQLType_t::binary:
			return BYTEAOID;
		case pgSQLType_t::date:
			return DATEOID;
		case pgSQLType_t::time:
			return TIMEOID;
		case pgSQLType_t::dateTime:
			return TIMESTAMPOID;
		case pgSQLType_t::uuid:
			return UUIDOID;
		default:
			return InvalidOid;
	}
}

void pgSQLQuery_t::swap(pgSQLQuery_t &qry) noexcept
{
	std::swap(connection, qry.connection);
	std::swap(query, qry.query);
	std::swap(numParams, qry.numParams);
	paramTypes.swap(qry.paramTypes);
	params.swap(qry.params);
	paramStorage.swap(qry.paramStorage);
	dataLengths.swap(qry.dataLengths);
}

pgSQLResult_t::pgSQLResult_t(PGresult *res) noexcept : result{res}
{
	if (!result)
		return;
	rows = static_cast<uint32_t>(PQntuples(result));
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

bool pgSQLResult_t::hasData() const noexcept
{
	const auto result{errorNum()};
	return result == PGRES_TUPLES_OK || result == PGRES_SINGLE_TUPLE;
}

bool pgSQLResult_t::next() noexcept
{
	if (row < rows)
		++row;
	return row != rows;
}

pgSQLValue_t pgSQLResult_t::operator [](const uint32_t idx) const noexcept
{
	if (!valid() || row >= rows || idx > fields)
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
	std::swap(rows, res.rows);
	std::swap(fields, res.fields);
	std::swap(row, res.row);
	fieldInfo.swap(res.fieldInfo);
}

pgSQLValue_t::pgSQLValue_t(std::nullptr_t) noexcept : type{ANYOID} { }

void pgSQLValue_t::swap(pgSQLValue_t &value) noexcept
{
	std::swap(data, value.data);
	std::swap(type, value.type);
}

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

const char *pgSQLValue_t::asString() const
{
	if (isNull() || (type != VARCHAROID && type != TEXTOID))
		throw pgSQLValueError_t{pgSQLErrorType_t::stringError};
	return static_cast<const char *>(data);
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

// *grumbles* Postgres handles floats badly by union-converting to ints, then
// sending the "ints" as big endian..
float pgSQLValue_t::asFloat() const
{
	const auto intValue{asInt<uint32_t, FLOAT4OID, pgSQLErrorType_t::floatError>()};
	float value{};
	static_assert(sizeof(float) == 4 && sizeof(uint32_t) == 4, "Platform float and uint32_t types are not of suitable size");
	std::memcpy(&value, &intValue, sizeof(float));
	return value;
}

double pgSQLValue_t::asDouble() const
{
	const auto intValue{asInt<uint64_t, FLOAT8OID, pgSQLErrorType_t::doubleError>()};
	double value{};
	static_assert(sizeof(double) == 8 && sizeof(uint64_t) == 8, "Platform double and uint64_t types are not of suitable size");
	std::memcpy(&value, &intValue, sizeof(double));
	return value;
}

// The conversion code here is ripped off wholesale from
// https://en.wikipedia.org/wiki/Julian_day#Julian_or_Gregorian_calendar_from_Julian_day_number
// It is insane. This code stinks. The naming is even ?? but there's no way around this.
// Postgres dug us into this mess
ormDate_t pgSQLValue_t::julianDateToDate(const int64_t date) noexcept
{
	const auto f{date + 1401 + (((4 * date + 274277) / 146097) * 3) / 4 - 38};
	const auto e{4 * f + 3};
	const auto g{(e % 1461) / 4};
	const auto h{5 * g + 2};
	const auto day{(h % 153) / 5 + 1};
	const auto month{((h / 153 + 2) % 12) + 1};
	const auto year{(e / 1461) - 4716 + (14 - month) / 12};
	return {static_cast<int16_t>(year), static_cast<uint8_t>(month), static_cast<uint8_t>(day)};
}

ormDate_t pgSQLValue_t::asDate() const
{
	const auto date{asInt<int32_t, DATEOID, pgSQLErrorType_t::dateError>()};
	return julianDateToDate(date +  postgresDateEpoch);
}

ormDateTime_t pgSQLValue_t::asDateTime() const
{
	auto timestamp{asInt<int64_t, TIMESTAMPOID, pgSQLErrorType_t::dateTimeError>()};
	const auto microseconds{static_cast<uint32_t>(timestamp % 1000000)};
	timestamp /= 1000000;
	const auto seconds{static_cast<uint8_t>(timestamp % 60)};
	timestamp /= 60;
	const auto minutes{static_cast<uint8_t>(timestamp % 60)};
	timestamp /= 60;
	const auto hours{static_cast<uint8_t>(timestamp % 24)};
	timestamp /= 24;
	const auto date{julianDateToDate(timestamp + postgresDateEpoch)};
	return {date.year(), date.month(), date.day(), hours, minutes, seconds, microseconds * 1000U};
}

ormUUID_t pgSQLValue_t::asUUID() const
{
	using substrate::buffer_utils::readLE;
	const auto *const buffer{static_cast<const uint8_t *>(data)};
	const auto a{readLE<uint32_t>(buffer + 0)};
	const auto b{readLE<uint16_t>(buffer + 4)};
	const auto c{readLE<uint16_t>(buffer + 6)};
	const auto d{readLE<uint16_t>(buffer + 8)};
	uint64_t e{};
	std::memcpy(&e, buffer + 10, 6);
	const guid_t result{a, b, c, d | (e << 16U)};
	return {result};
}

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
