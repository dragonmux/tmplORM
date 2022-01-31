#ifndef _WINDOWS
#include <unistd.h>
#else
#include <io.h>
#endif
#define UNICODE
#include <sql.h>
#include <sqlext.h>
#include <substrate/utility>
#include "mssql.hxx"

/*!
 * @internal
 * @file
 * @author Rachel Mant
 * @date 2016-2020
 * @brief C++ MSSQL/T-SQL driver abstraction layer for handling client connections and query datasets
 */

// We'd use <locale> here with its std::codecvt/std::wstring_convert functionality, however
// this causes many numerous reallocations (which can fail and throw), heap fragmentation,
// and general slowness when we already know the bounds of our string.
// Instead of this, we have our own recoder that performs a single allocation
// and returns either the completely recoded string, or nullptr.

using substrate::vectorStateException_t;
using namespace tmplORM::mssql::driver;

inline tSQLExecErrorType_t translateError(const SQLRETURN result) noexcept
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

inline int16_t odbcToCType(const int16_t typeODBC) noexcept
{
	switch (typeODBC)
	{
		case SQL_CHAR:
		case SQL_VARCHAR:
		case SQL_LONGVARCHAR:
			return SQL_C_CHAR;
		case SQL_WCHAR:
		case SQL_WVARCHAR:
		case SQL_WLONGVARCHAR:
			return SQL_C_WCHAR;
		case SQL_BINARY:
		case SQL_VARBINARY:
		case SQL_LONGVARBINARY:
			return SQL_C_BINARY;
		case SQL_BIGINT:
			return SQL_C_SBIGINT;
		case SQL_INTEGER:
			return SQL_C_SLONG;
		case SQL_SMALLINT:
			return SQL_C_SSHORT;
		case SQL_TINYINT:
			return SQL_C_STINYINT;
		case SQL_GUID:
			return SQL_C_GUID;
		case SQL_REAL:
			return SQL_C_FLOAT;
		case SQL_FLOAT:
		case SQL_DOUBLE:
			return SQL_C_DOUBLE;
	}
	return typeODBC;
}

tSQLClient_t::tSQLClient_t() noexcept : dbHandle(nullptr), connection(nullptr), haveConnection(false), needsCommit(false), _error()
{
	if (SQLAllocHandle(SQL_HANDLE_ENV, nullptr, &dbHandle) != SQL_SUCCESS || !dbHandle)
		error(tSQLExecErrorType_t::connect, SQL_HANDLE_ENV, dbHandle);
	else if (error(SQLSetEnvAttr(dbHandle, SQL_ATTR_ODBC_VERSION, reinterpret_cast<void *>(SQL_OV_ODBC3), 0), SQL_HANDLE_ENV, dbHandle)) // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast) lgtm [cpp/reinterpret-cast]
		return;
	else if (SQLAllocHandle(SQL_HANDLE_DBC, dbHandle, &connection) != SQL_SUCCESS || !connection)
		error(tSQLExecErrorType_t::connect, SQL_HANDLE_DBC, connection);
}

tSQLClient_t::~tSQLClient_t() noexcept
{
	disconnect();
	SQLFreeHandle(SQL_HANDLE_DBC, connection);
	SQLFreeHandle(SQL_HANDLE_ENV, dbHandle);
}

void tSQLClient_t::operator =(tSQLClient_t &&con) noexcept
{
	std::swap(dbHandle, con.dbHandle);
	std::swap(connection, con.connection);
	std::swap(haveConnection, con.haveConnection);
	std::swap(needsCommit, con.needsCommit);
	std::swap(_error, con._error);
}

void tSQLClient_t::disconnect() const noexcept
{
	if (haveConnection)
	{
		if (needsCommit)
			rollback();
		haveConnection = error(SQLDisconnect(connection), SQL_HANDLE_DBC, connection);
	}
}

bool tSQLClient_t::connect(const char *const driver, const char *const host, const uint32_t port,
	const char *const user, const char *const passwd) const noexcept
{
	if (!connection || haveConnection)
		return !error(tSQLExecErrorType_t::connect);

	auto connString = formatString("DRIVER=%s;SERVER=tcp:%s,%u;UID=%s;PWD=%s;TRUSTED_CONNECTION=no", driver, host, port ? port : 1433, user, passwd);
	if (!connString)
		return !error(tSQLExecErrorType_t::connect);
	auto odbcString = utf16::convert(connString.get());
	auto odbcStringLen = utf16::length(odbcString);
	int16_t resultLen{};

	if (odbcStringLen > std::numeric_limits<SQLSMALLINT>::max())
		return false;
	return haveConnection = !error(SQLDriverConnect(
			connection, nullptr, odbcString, static_cast<SQLSMALLINT>(odbcStringLen),
			nullptr, 0, &resultLen, SQL_DRIVER_NOPROMPT
		), SQL_HANDLE_DBC, connection
	);
}

bool tSQLClient_t::selectDB(const char *const db) const noexcept
{
	if (!connection || !db)
		return false;
	auto odbcString = utf16::convert(db);
	auto odbcStringLen = utf16::length(odbcString);
	if (odbcStringLen > std::numeric_limits<SQLINTEGER>::max())
		return false;
	return !error(SQLSetConnectAttr(
			connection, SQL_ATTR_CURRENT_CATALOG, odbcString, static_cast<SQLINTEGER>(utf16::length(odbcString) * 2)
		), SQL_HANDLE_DBC, connection);
}

tSQLQuery_t tSQLClient_t::prepare(const char *const queryStmt, const size_t paramsCount) const noexcept
{
	void *queryHandle = nullptr;
	if (!valid() || error(SQLAllocHandle(SQL_HANDLE_STMT, connection, &queryHandle), SQL_HANDLE_STMT, queryHandle) || !queryHandle)
		return {};
	return {this, queryHandle, queryStmt, paramsCount};
}

tSQLResult_t tSQLClient_t::query(const char *const queryStmt) const noexcept
{
	const auto query = prepare(queryStmt, 0);
	if (query.valid())
		return query.execute();
	return {};
}

bool tSQLClient_t::beginTransact() const noexcept
{
	if (needsCommit || !valid() || error(SQLSetConnectAttr(connection, SQL_ATTR_AUTOCOMMIT,
		reinterpret_cast<void *>(SQL_AUTOCOMMIT_OFF), 0), SQL_HANDLE_DBC, connection)) // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast) lgtm [cpp/reinterpret-cast]
		return false;
	return needsCommit = true;
}

bool tSQLClient_t::endTransact(const bool commitSuccess) const noexcept
{
	if (needsCommit && valid())
	{
		needsCommit = error(SQLEndTran(SQL_HANDLE_DBC, connection, commitSuccess ? SQL_COMMIT : SQL_ROLLBACK),
			SQL_HANDLE_DBC, connection) || error(SQLSetConnectAttr(connection, SQL_ATTR_AUTOCOMMIT,
			reinterpret_cast<void *>(SQL_AUTOCOMMIT_ON), 0), SQL_HANDLE_DBC, connection); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast) lgtm [cpp/reinterpret-cast]
	}
	return !needsCommit;
}

bool tSQLClient_t::error(const int16_t err, const int16_t handleType, void *const handle) const noexcept
	{ return error(translateError(err), handleType, handle); }
bool tSQLClient_t::error(const tSQLExecErrorType_t err) const noexcept
	{ return error(err, 0, nullptr); }

bool tSQLClient_t::error(const tSQLExecErrorType_t err, const int16_t handleType, void *const handle) const noexcept
{
	_error = tSQLExecError_t(err, handleType, handle);
	return _error != tSQLExecErrorType_t::ok;
}

tSQLQuery_t::tSQLQuery_t(const tSQLClient_t *const parent, void *handle, const char *const queryStmt, const size_t paramsCount) noexcept :
	client(parent), queryHandle(handle), numParams(paramsCount), paramStorage(paramsCount), dataLengths(paramsCount), executed(false)
{
	if (!queryHandle || (numParams && !dataLengths) || !client || !queryStmt)
	{
		client = nullptr;
		return;
	}
	const auto query = utf16::convert(queryStmt);
	const auto queryLength{utf16::length(query)};
	if (queryLength > std::numeric_limits<SQLINTEGER>::max())
		error(SQL_ERROR);
	else
		error(SQLPrepare(queryHandle, query, static_cast<SQLINTEGER>(queryLength)));
}

tSQLQuery_t::~tSQLQuery_t() noexcept
{
	if (queryHandle && !executed)
		error(SQLFreeHandle(SQL_HANDLE_STMT, queryHandle));
}

void tSQLQuery_t::operator =(tSQLQuery_t &&qry) noexcept
{
	std::swap(client, qry.client);
	std::swap(queryHandle, qry.queryHandle);
	std::swap(numParams, qry.numParams);
	std::swap(paramStorage, qry.paramStorage);
	std::swap(dataLengths, qry.dataLengths);
	std::swap(executed, qry.executed);
}

tSQLResult_t tSQLQuery_t::execute() const noexcept
{
	if (!valid() || !queryHandle || !client || executed)
		return {};
	else if (error(SQLExecute(queryHandle)) && client->error() != tSQLExecErrorType_t::dataAvail &&
		client->error() != tSQLExecErrorType_t::noData)
		return {};
	executed = true;
	return {client, queryHandle, client->error() == tSQLExecErrorType_t::ok};
}

bool tSQLQuery_t::error(const int16_t err) const noexcept
	{ return !client || client->error(err, SQL_HANDLE_STMT, queryHandle); }

tSQLResult_t::tSQLResult_t(const tSQLClient_t *const _client, void *handle, const bool hasData, const bool freeHandle) noexcept :
	client{_client}, queryHandle{handle}, _hasData{hasData}, _freeHandle{freeHandle}, fields{0}, fieldInfo{}, valueCache{}
{
	int16_t _fields;
	if (error(SQLNumResultCols(queryHandle, &_fields)) || (uint16_t(_fields) & 0x8000U))
		return;
	else if (_fields)
	{
		fields = uint16_t(_fields);
		fieldInfo = substrate::make_unique_nothrow<fieldType_t []>(fields);
		valueCache = fixedVector_t<tSQLValue_t>{fields};
		if (!fieldInfo || !valueCache.valid())
			return;
		for (uint16_t i = 0; i < fields; ++i)
		{
			long type = 0, length = 0;
			if (error(
					SQLColAttribute(queryHandle, static_cast<SQLUSMALLINT>(i + 1U), SQL_DESC_CONCISE_TYPE,
						nullptr, 0, nullptr, &type)
				) || !type || error(
					SQLColAttribute(queryHandle, static_cast<SQLUSMALLINT>(i + 1U), SQL_DESC_OCTET_LENGTH,
						nullptr, 0, nullptr, &length)
				) || length < 0)
			{
				fieldInfo.reset();
				return;
			}
			fieldInfo[i] = {int16_t(type), uint32_t(length)};
		}
		if (hasData)
			next();
	}
}

tSQLResult_t::~tSQLResult_t() noexcept
{
	if (_freeHandle)
		SQLFreeHandle(SQL_HANDLE_STMT, queryHandle);
}

void tSQLResult_t::operator =(tSQLResult_t &&res) noexcept
{
	std::swap(client, res.client);
	std::swap(queryHandle, res.queryHandle);
	std::swap(_hasData, res._hasData);
	std::swap(_freeHandle, res._freeHandle);
	std::swap(fields, res.fields);
	std::swap(fieldInfo, res.fieldInfo);
	valueCache.swap(res.valueCache);
}

uint64_t tSQLResult_t::numRows() const noexcept
{
	long rows = 0;
	if (!valid() || error(SQLRowCount(queryHandle, &rows)) || rows < 0)
		return 0;
	return uint64_t(rows);
}

bool tSQLResult_t::next() const noexcept
{
	if (!valid())
		return false;
	for (auto &value : valueCache)
		value = tSQLValue_t{};
	return !error(SQLFetch(queryHandle));
}

inline bool isCharType(const int16_t type) noexcept { return type == SQL_LONGVARCHAR || type ==  SQL_VARCHAR || type == SQL_CHAR; }
inline bool isWCharType(const int16_t type) noexcept { return type == SQL_WLONGVARCHAR || type == SQL_WVARCHAR || type == SQL_WCHAR; }
inline bool isBinType(const int16_t type) noexcept { return type == SQL_LONGVARBINARY || type == SQL_VARBINARY || type == SQL_BINARY; }
tSQLValue_t tSQLResult_t::nullValue{};

tSQLValue_t &tSQLResult_t::operator [](const uint16_t idx) const noexcept try
{
	if (idx >= fields || !valid())
		return nullValue;
	else if (!valueCache[idx].isNull())
		return valueCache[idx];
	const auto column{static_cast<uint16_t>(idx + 1U)};

	// Pitty this can't use C++17 syntax: [const int16_t type, const uint32_t valueLength] = fieldInfo[idx];
	int16_t type;
	uint32_t valueLength;
	std::tie(type, valueLength) = fieldInfo[idx];
	const int16_t cType = odbcToCType(type);
	if (isCharType(type) || isWCharType(type) || isBinType(type))
	{
		uint32_t temp = 0;
		long length = 0;
		if (error(SQLGetData(queryHandle, column, cType, &temp, 0, &length)) || length == SQL_NULL_DATA || length == SQL_NO_TOTAL)
			return nullValue;
		valueLength = uint32_t(length) + 1;
		if (isWCharType(type))
			++valueLength;
	}

	auto valueStorage = substrate::make_unique_nothrow<char []>(valueLength);
	if (!valueStorage)
		return nullValue;
	valueStorage[valueLength - 1] = 0;
	if (isWCharType(type))
		valueStorage[valueLength - 2] = 0;
	// Hmm.. I think this might actually be wrong.. needs more thought.
	if (!(isBinType(type) && valueLength == 1))
	{
		if (error(SQLGetData(queryHandle, column, cType, valueStorage.get(), valueLength, nullptr)))
			return nullValue;
	}
	valueCache[idx] = {valueStorage.release(), valueLength, type};
	return valueCache[idx];
}
catch (const vectorStateException_t &error)
{
	const std::string desc{"tmplORM FATAL: Failed to allocate parameters - "_s};
	const std::string what{error.what()};
	write(STDERR_FILENO, desc.data(), desc.size());
	write(STDERR_FILENO, what.data(), what.size());
	write(STDERR_FILENO, "\n", 1);
	std::terminate();
}
catch (const std::out_of_range &error)
{
	const std::string desc{"tmplORM FATAL: Indexing failure - "_s};
	const std::string what{error.what()};
	write(STDERR_FILENO, desc.data(), desc.size());
	write(STDERR_FILENO, what.data(), what.size());
	write(STDERR_FILENO, "\n", 1);
	std::terminate();
}

bool tSQLResult_t::error(const int16_t err) const noexcept
	{ return !client || client->error(err, SQL_HANDLE_STMT, queryHandle); }

tSQLValue_t::tSQLValue_t(const void *const _data, const uint64_t _length, const int16_t _type) noexcept :
	data{static_cast<const char *>(_data)}, length{_length}, type{_type}
{
	if (isWCharType(type))
	{
		auto newData{utf16::convert(static_cast<const char16_t *>(_data))};
		data.reset(newData.release());
	}
}

void tSQLValue_t::operator =(tSQLValue_t &&value) noexcept
{
	std::swap(data, value.data);
	std::swap(length, value.length);
	std::swap(type, value.type);
}

template<typename T> T reinterpret(const stringPtr_t &data) noexcept
{
	T value{};
	memcpy(&value, data.get(), sizeof(T));
	return value;
}

std::unique_ptr<const char []> tSQLValue_t::asString(const bool release) const
{
	if (isNull() || (!isCharType(type) && !isWCharType(type)))
		throw tSQLValueError_t(tSQLErrorType_t::stringError);
	return release ? std::move(data) : stringDup(data);
}

template<int16_t rawType, int16_t, tSQLErrorType_t error, typename T> T asInt(const tSQLValue_t &val, const stringPtr_t &data, const int16_t type)
{
	if (val.isNull() || type != rawType)
		throw tSQLValueError_t(error);
	return reinterpret<T>(data);
}

uint8_t tSQLValue_t::asUint8() const
	{ return asInt<SQL_TINYINT, SQL_C_UTINYINT, tSQLErrorType_t::uint8Error, uint8_t>(*this, data, type); }
int8_t tSQLValue_t::asInt8() const
	{ return asInt<SQL_TINYINT, SQL_C_STINYINT, tSQLErrorType_t::int8Error, int8_t>(*this, data, type); }
uint16_t tSQLValue_t::asUint16() const
	{ return asInt<SQL_SMALLINT, SQL_C_USHORT, tSQLErrorType_t::uint16Error, uint16_t>(*this, data, type); }
int16_t tSQLValue_t::asInt16() const
	{ return asInt<SQL_SMALLINT, SQL_C_SSHORT, tSQLErrorType_t::int16Error, int16_t>(*this, data, type); }
uint32_t tSQLValue_t::asUint32() const
	{ return asInt<SQL_INTEGER, SQL_C_ULONG, tSQLErrorType_t::uint32Error, uint32_t>(*this, data, type); }
int32_t tSQLValue_t::asInt32() const
	{ return asInt<SQL_INTEGER, SQL_C_SLONG, tSQLErrorType_t::int32Error, int32_t>(*this, data, type); }
uint64_t tSQLValue_t::asUint64() const
	{ return asInt<SQL_BIGINT, SQL_C_UBIGINT, tSQLErrorType_t::uint64Error, uint64_t>(*this, data, type); }
int64_t tSQLValue_t::asInt64() const
	{ return asInt<SQL_BIGINT, SQL_C_SBIGINT, tSQLErrorType_t::int64Error, int64_t>(*this, data, type); }
float tSQLValue_t::asFloat() const
	{ return asInt<SQL_REAL, SQL_C_FLOAT, tSQLErrorType_t::floatError, float>(*this, data, type); }

double tSQLValue_t::asDouble() const
{
	if (isNull() || (type != SQL_FLOAT && type != SQL_DOUBLE))
		throw tSQLValueError_t(tSQLErrorType_t::doubleError);
	return reinterpret<double>(data);
}

bool tSQLValue_t::asBool() const
{
	if (isNull() || type != SQL_BIT)
		throw tSQLValueError_t(tSQLErrorType_t::boolError);
	return reinterpret<uint8_t>(data);
}

const void *tSQLValue_t::asBuffer(size_t &bufferLength, const bool release) const
{
	if (isNull() || !isBinType(type))
		throw tSQLValueError_t(tSQLErrorType_t::binError);
	bufferLength = length;
	return release ? data.release() : data.get();
}

ormDate_t tSQLValue_t::asDate() const
{
	if (isNull() || type != SQL_TYPE_DATE || length < sizeof(SQL_TYPE_DATE))
		throw tSQLValueError_t(tSQLErrorType_t::dateError);
	const auto date = reinterpret<SQL_DATE_STRUCT>(data);
	return {date.year, uint8_t(date.month), uint8_t(date.day)};
}

// TODO: ormTime_t tSQLValue_t::asTime() const => SQL_TYPE_TIME, SQL_TIME_STRUCT

ormDateTime_t tSQLValue_t::asDateTime() const
{
	if (isNull() || type != SQL_TYPE_TIMESTAMP || length < sizeof(SQL_TYPE_TIMESTAMP))
		throw tSQLValueError_t(tSQLErrorType_t::dateTimeError);
	const auto dateTime = reinterpret<SQL_TIMESTAMP_STRUCT>(data);
	return {dateTime.year, uint8_t(dateTime.month), uint8_t(dateTime.day),
		dateTime.hour, 	dateTime.minute, dateTime.second, dateTime.fraction};
}

ormUUID_t tSQLValue_t::asUUID() const
{
	if (isNull() || type != SQL_GUID)
		throw tSQLValueError_t(tSQLErrorType_t::uuidError);
	return {reinterpret<guid_t>(data), true};
}

tSQLExecError_t::tSQLExecError_t(const tSQLExecErrorType_t error, const int16_t handleType, void *const handle) noexcept : _error(error), _state{{}}, _message()
{
	if (handle)
	{
		int16_t length = 0;
		static_assert(sqlState_t{}.size() == SQL_SQLSTATE_SIZE + 1, "sqlState_t not the correct length for this ODBC interface");

		SQLGetDiagFieldA(handleType, handle, 1, SQL_DIAG_SQLSTATE, _state.data(),
			static_cast<SQLSMALLINT>(_state.size()), nullptr);
		SQLGetDiagFieldA(handleType, handle, 1, SQL_DIAG_MESSAGE_TEXT, nullptr, 0, &length);
		if (length < 0)
			return;
		const auto messageLen{static_cast<uint16_t>(length + 1)};
		_message = substrate::make_unique_nothrow<char []>(messageLen);
		if (_message)
		{
			SQLGetDiagFieldA(handleType, handle, 1, SQL_DIAG_MESSAGE_TEXT, _message.get(),
				static_cast<SQLSMALLINT>(messageLen), nullptr);
			_message[messageLen - 1U] = 0;
		}
	}
}

void tSQLExecError_t::operator =(tSQLExecError_t &&err) noexcept
{
	std::swap(_error, err._error);
	std::swap(_state, err._state);
	std::swap(_message, err._message);
}

const char *tSQLExecError_t::error() const noexcept
{
	switch(_error)
	{
		case tSQLExecErrorType_t::ok:
			return "No error";
		case tSQLExecErrorType_t::connect:
			return "Could not connect to database";
		case tSQLExecErrorType_t::query:
			return "Could not execute query";
		case tSQLExecErrorType_t::handleInv:
			return "Invalid handle returned";
		case tSQLExecErrorType_t::generalError:
			return "General error from the SQL server";
		case tSQLExecErrorType_t::needData:
			return "SQL server is requesting more data for call";
		case tSQLExecErrorType_t::noData:
			return "SQL server could not return data for call";
		case tSQLExecErrorType_t::dataAvail:
			return "SQL server is claiming more data is available";
		default:
			return "Unknown error";
	}
}

const char *tSQLValueError_t::error() const noexcept
{
	switch (errorType)
	{
		case tSQLErrorType_t::noError:
			return "No error occured";
		case tSQLErrorType_t::stringError:
			return "Error converting value to a string";
		case tSQLErrorType_t::boolError:
			return "Error converting value to a boolean";
		case tSQLErrorType_t::uint8Error:
			return "Error converting value to an unsigned 8-bit integer";
		case tSQLErrorType_t::int8Error:
			return "Error converting value to a signed 8-bit integer";
		case tSQLErrorType_t::uint16Error:
			return "Error converting value to an unsigned 16-bit integer";
		case tSQLErrorType_t::int16Error:
			return "Error converting value to a signed 16-bit integer";
		case tSQLErrorType_t::uint32Error:
			return "Error converting value to an unsigned 32-bit integer";
		case tSQLErrorType_t::int32Error:
			return "Error converting value to a signed 32-bit integer";
		case tSQLErrorType_t::uint64Error:
			return "Error converting value to an unsigned 64-bit integer";
		case tSQLErrorType_t::int64Error:
			return "Error converting value to a signed 64-bit integer";
		case tSQLErrorType_t::floatError:
			return "Error converting value to a single-precision floating point number";
		case tSQLErrorType_t::doubleError:
			return "Error converting value to a double-precision floating point number";
		case tSQLErrorType_t::binError:
			return "Error converting value to binary buffer";
		case tSQLErrorType_t::dateError:
			return "Error converting value to a date quantity";
		case tSQLErrorType_t::dateTimeError:
			return "Error converting value to a date and time quantity";
		case tSQLErrorType_t::uuidError:
			return "Error converting value to a UUID";
	}
	return "An unknown error occured";
}
