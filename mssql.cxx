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
template<typename T> void swap(const T &a, const T &b) noexcept//(std::swap(const_cast<T &>(a), const_cast<T &>(b)))
	{ std::swap(const_cast<T &>(a), const_cast<T &>(b)); }

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
	}
	return typeODBC;
}

tSQLClient_t::tSQLClient_t() noexcept : dbHandle(nullptr), connection(nullptr), haveConnection(false), needsCommit(false), _error()
{
	if (SQLAllocHandle(SQL_HANDLE_ENV, nullptr, &dbHandle) != SQL_SUCCESS || !dbHandle)
	{
		error(tSQLExecErrorType_t::connect, SQL_HANDLE_ENV, dbHandle);
		return;
	}
	else if (error(SQLSetEnvAttr(dbHandle, SQL_ATTR_ODBC_VERSION, reinterpret_cast<void *>(long(SQL_OV_ODBC3)), 0), SQL_HANDLE_ENV, dbHandle))
		return;
	else if (SQLAllocHandle(SQL_HANDLE_DBC, dbHandle, &connection) != SQL_SUCCESS || !connection)
	{
		error(tSQLExecErrorType_t::connect, SQL_HANDLE_DBC, connection);
		return;
	}
}

tSQLClient_t::~tSQLClient_t() noexcept
{
	disconnect();
	SQLFreeHandle(SQL_HANDLE_DBC, connection);
	SQLFreeHandle(SQL_HANDLE_ENV, dbHandle);
}

tSQLClient_t &tSQLClient_t::operator =(tSQLClient_t &&con) noexcept
{
	std::swap(dbHandle, con.dbHandle);
	std::swap(connection, con.connection);
	std::swap(haveConnection, con.haveConnection);
	std::swap(needsCommit, con.needsCommit);
	std::swap(_error, con._error);
	return *this;
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

bool tSQLClient_t::connect(const stringPtr_t &connString) const noexcept
{
	if (!dbHandle || !connection || haveConnection)
		return !error(tSQLExecErrorType_t::connect);
	auto odbcString = utf16::convert(connString.get());
	int16_t resultLen = 0;

	return !error(SQLBrowseConnect(connection, odbcString, utf16::length(odbcString), nullptr, 0, &resultLen), SQL_HANDLE_DBC, connection);
}

bool tSQLClient_t::connect(const char *const driver, const char *const host, const uint32_t port, const char *const user, const char *const passwd) const noexcept
{
	return connect(formatString("DRIVER=%s;SERVER=tcp:%s,%u;UID=%s;PID=%s;TRUSTED_CONNECTION=no", driver, host, port ? port : 1433, user, passwd)) ||
		_error == tSQLExecErrorType_t::needData;
}

bool tSQLClient_t::selectDB(const char *const db) const noexcept
	{ return haveConnection = connect(formatString("DATABASE=%s", db)); }

tSQLQuery_t tSQLClient_t::prepare(const char *const queryStmt, const size_t paramsCount) const noexcept
{
	void *queryHandle = nullptr;
	if (!valid() || error(SQLAllocHandle(SQL_HANDLE_STMT, connection, &queryHandle), SQL_HANDLE_STMT, queryHandle) || !queryHandle)
		return tSQLQuery_t();
	return tSQLQuery_t(this, queryHandle, queryStmt, paramsCount);
}

tSQLResult_t tSQLClient_t::query(const char *const queryStmt) const noexcept
{
	auto query = prepare(queryStmt, 0);
	if (query.valid())
		return query.execute();
	return tSQLResult_t();
}

bool tSQLClient_t::beginTransact() const noexcept
{
	if (needsCommit || !valid() || error(SQLSetConnectAttr(connection, SQL_ATTR_AUTOCOMMIT,
		reinterpret_cast<void *>(long(SQL_AUTOCOMMIT_OFF)), 0), SQL_HANDLE_DBC, connection))
		return false;
	return needsCommit = true;
}

bool tSQLClient_t::endTransact(const bool commitSuccess) const noexcept
{
	if (needsCommit && valid())
		needsCommit = error(SQLEndTran(SQL_HANDLE_DBC, connection, commitSuccess ? SQL_COMMIT : SQL_ROLLBACK), SQL_HANDLE_DBC, connection);
	return !needsCommit;
}

bool tSQLClient_t::error(const int16_t err, const int16_t handleType, void *const handle) const noexcept
	{ return error(translateError(err), handleType, handle); }
bool tSQLClient_t::error(const tSQLExecErrorType_t err) const noexcept
	{ return error(err, 0, nullptr); }

bool tSQLClient_t::error(const tSQLExecErrorType_t err, const int16_t handleType, void *const handle) const noexcept
{
	_error = tSQLExecError_t(err, handleType, handle);
	return err != tSQLExecErrorType_t::ok;
}

tSQLQuery_t::tSQLQuery_t(const tSQLClient_t *const parent, void *const handle, const char *const queryStmt, const size_t paramsCount) noexcept :
	client(parent), queryHandle(handle), numParams(paramsCount), dataLengths(numParams ? makeUnique<long []>(numParams) : nullptr), executed(false)
{
	if (!queryHandle || (numParams && !dataLengths) || !client)
		return;
	auto query = utf16::convert(queryStmt);
	error(SQLPrepare(queryHandle, query, utf16::length(query)));
}

tSQLQuery_t::~tSQLQuery_t() noexcept
{
	if (queryHandle && !executed)
		error(SQLFreeHandle(SQL_HANDLE_STMT, queryHandle));
}

tSQLQuery_t &tSQLQuery_t::operator =(tSQLQuery_t &&qry) noexcept
{
	swap(client, qry.client);
	swap(queryHandle, qry.queryHandle);
	std::swap(numParams, qry.numParams);
	std::swap(dataLengths, qry.dataLengths);
	std::swap(executed, qry.executed);
	return *this;
}

tSQLResult_t tSQLQuery_t::execute() const noexcept
{
	if (!valid() || !queryHandle || !client || executed)
		return tSQLResult_t();
	else if (error(SQLExecute(queryHandle)) && client->error() != tSQLExecErrorType_t::dataAvail &&
		client->error() != tSQLExecErrorType_t::noData)
		return tSQLResult_t();
	executed = true;
	return tSQLResult_t(client, std::move(queryHandle), client->error() == tSQLExecErrorType_t::ok);
}

bool tSQLQuery_t::error(const int16_t err) const noexcept
	{ return !client || client->error(err, SQL_HANDLE_STMT, queryHandle); }

tSQLResult_t::tSQLResult_t(const tSQLClient_t *const _client, void *const &&handle, const bool hasData, const bool freeHandle) noexcept :
	client(_client), queryHandle(), _hasData(hasData), _freeHandle(freeHandle), fields(0), fieldInfo()
{
	uint16_t &_fields = const_cast<uint16_t &>(fields);
	swap(queryHandle, handle);
	if (error(SQLNumResultCols(queryHandle, reinterpret_cast<int16_t *>(&_fields))) || (fields & 0x8000))
	{
		_fields = 0;
		return;
	}
	else if (fields)
	{
		fieldInfo = makeUnique<fieldType_t []>(fields);
		if (!fieldInfo)
			return;
		for (uint16_t i = 0; i < fields; ++i)
		{
			long type = 0, length = 0;
			if (error(SQLColAttribute(queryHandle, i + 1, SQL_DESC_TYPE, nullptr, 0, nullptr, &type)) || !type ||
				error(SQLColAttribute(queryHandle, i + 1, SQL_DESC_OCTET_LENGTH, nullptr, 0, nullptr, &length)) || length < 0)
			{
				fieldInfo.reset();
				return;
			}
			fieldInfo[i] = std::make_pair(int16_t(type), uint32_t(length));
		}
	}
}

tSQLResult_t::~tSQLResult_t() noexcept
{
	if (_freeHandle)
		SQLFreeHandle(SQL_HANDLE_STMT, queryHandle);
}

tSQLResult_t &tSQLResult_t::operator =(tSQLResult_t &&res) noexcept
{
	swap(client, res.client);
	swap(queryHandle, res.queryHandle);
	swap(_hasData, res._hasData);
	swap(_freeHandle, res._freeHandle);
	swap(fields, res.fields);
	return *this;
}

uint64_t tSQLResult_t::numRows() const noexcept
{
	long rows = 0;
	if (!valid() || error(SQLRowCount(queryHandle, &rows)) || rows < 0)
		return 0;
	return uint64_t(rows);
}

bool tSQLResult_t::next() const noexcept { return valid() && !error(SQLFetch(queryHandle)); }
inline bool isCharType(const int16_t type) noexcept { return type == SQL_LONGVARCHAR || type ==  SQL_VARCHAR || type == SQL_CHAR; }
inline bool isWCharType(const int16_t type) noexcept { return type == SQL_WLONGVARCHAR || type == SQL_WVARCHAR || type == SQL_WCHAR; }
inline bool isBinType(const int16_t type) noexcept { return type == SQL_LONGVARBINARY || type == SQL_VARBINARY || type == SQL_BINARY; }

tSQLValue_t tSQLResult_t::operator [](const uint16_t idx) const noexcept
{
	if (idx >= fields || !valid())
		return tSQLValue_t();
	const uint16_t column = idx + 1;

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
			return tSQLValue_t();
		valueLength = uint32_t(length) + 1;
		if (isWCharType(type))
			++valueLength;
	}

	auto valueStorage = makeUnique<char []>(valueLength);
	if (!valueStorage)
		return tSQLValue_t();
	valueStorage[valueLength - 1] = 0;
	if (isWCharType(type))
		valueStorage[valueLength - 2] = 0;
	// Hmm.. I think this might actually be wrong.. needs more thought.
	if (!(isBinType(type) && valueLength == 1))
	{
		if (error(SQLGetData(queryHandle, column, cType, valueStorage.get(), valueLength, nullptr)))
			return tSQLValue_t();
	}
	return tSQLValue_t(valueStorage.release(), valueLength, type);
}

bool tSQLResult_t::error(const int16_t err) const noexcept
	{ return !client || client->error(err, SQL_HANDLE_STMT, queryHandle); }
		}
	}
}
