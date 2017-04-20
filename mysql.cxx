#include <string.h>
#include "mysql.hxx"
#include "value.hxx"
#include "string.hxx"

using namespace std;
using namespace tmplORM::mysql::driver;

mySQLClient_t database;

MYSQL *mySQLClient_t::con = nullptr;
uint32_t mySQLClient_t::handles = 0;
bool mySQLClient_t::haveConnection = false;

static const bool autoReconnect = true;

mySQLClient_t::mySQLClient_t() noexcept
{
	++handles;
	if (!con)
	{
		con = mysql_init(nullptr);
		if (con)
			mysql_options(con, MYSQL_OPT_RECONNECT, &autoReconnect);
	}
}

mySQLClient_t::mySQLClient_t(const mySQLClient_t &) noexcept { ++handles; }

mySQLClient_t::~mySQLClient_t() noexcept
{
	--handles;
	if (!handles)
	{
		if (con)
			mysql_close(con);
	}
}

mySQLClient_t &mySQLClient_t::operator =(const mySQLClient_t &) noexcept { return *this; }

bool mySQLClient_t::connect(const char *const host, const uint32_t port, const char *const user, const char *const passwd) const noexcept
{
	if (haveConnection)
		return true;
	else if (!con)
		return false;
	haveConnection = mysql_real_connect(con, host, user, passwd, nullptr, port, nullptr, CLIENT_IGNORE_SIGPIPE) != nullptr;
	return haveConnection;
}

bool mySQLClient_t::connect(const char *const unixSocket, const char *const user, const char *const passwd) const noexcept
{
	if (haveConnection)
		return true;
	else if (!con)
		return false;
	haveConnection = mysql_real_connect(con, nullptr, user, passwd, nullptr, 0, unixSocket, CLIENT_IGNORE_SIGPIPE) != nullptr;
	return haveConnection;
}

void mySQLClient_t::disconnect() noexcept
{
	if (valid())
	{
		mysql_close(con);
		con = nullptr;
		haveConnection = false;
	}
}

bool mySQLClient_t::selectDB(const char *const db) const noexcept { return valid() && mysql_select_db(con, db) == 0; }

bool mySQLClient_t::query(const char *const queryStmt, ...) const noexcept
{
	if (mysql_ping(con) != 0)
		return false;
	va_list args;
	va_start(args, queryStmt);
	auto query = vaFormatString(queryStmt, args);
	va_end(args);
	return mysql_query(con, query.get()) == 0;
}

mySQLResult_t mySQLClient_t::queryResult() const noexcept { return valid() ? mySQLResult_t(con) : mySQLResult_t(); }
mySQLPreparedQuery_t mySQLClient_t::prepare(const char *const queryStmt, const size_t paramsCount) const noexcept
	{ return valid() ? mySQLPreparedQuery_t(con, queryStmt, paramsCount) : mySQLPreparedQuery_t(); }
uint32_t mySQLClient_t::errorNum() const noexcept { return valid() ? mysql_errno(con) : 0; }
const char *mySQLClient_t::error() const noexcept { return valid() ? mysql_error(con) : nullptr; }

mySQLPreparedQuery_t::mySQLPreparedQuery_t(MYSQL *const con, const char *const queryStmt, const size_t paramsCount) noexcept :
	query(mysql_stmt_init(con)), params(paramsCount ? new (std::nothrow) MYSQL_BIND[paramsCount]() : nullptr), numParams(paramsCount), executed(false)
{
	if (!query || (numParams && !params))
		return;
	else if (numParams)
	{
		for (size_t i = 0; i < numParams; ++i)
			params[i].buffer_type = MYSQL_TYPE_NULL;
	}
	if (mysql_stmt_prepare(query, queryStmt, strlen(queryStmt) + 1) != 0)
		dtor();
}

mySQLPreparedQuery_t::mySQLPreparedQuery_t(mySQLPreparedQuery_t &&qry) noexcept : mySQLPreparedQuery_t() { *this = std::move(qry); }

mySQLPreparedQuery_t::~mySQLPreparedQuery_t() noexcept
{
	if (valid())
		dtor();
}

mySQLPreparedQuery_t &mySQLPreparedQuery_t::operator =(mySQLPreparedQuery_t &&qry) noexcept
{
	std::swap(query, qry.query);
	std::swap(params, qry.params);
	std::swap(numParams, qry.numParams);
	std::swap(executed, qry.executed);
	return *this;
}

void mySQLPreparedQuery_t::dtor() noexcept
{
	mysql_stmt_close(query);
	query = nullptr;
}

bool mySQLPreparedQuery_t::execute() noexcept
{
	if (valid())
	{
		if (params)
			mysql_stmt_bind_param(query, params.get());
		executed = mysql_stmt_execute(query) == 0;
	}
	return executed;
}

uint64_t mySQLPreparedQuery_t::rowID() const noexcept { return executed ? mysql_stmt_insert_id(query) : 0; }

mySQLResult_t::mySQLResult_t(MYSQL *const con) noexcept : result(mysql_store_result(con)) { }
mySQLResult_t::mySQLResult_t(mySQLResult_t &&res) noexcept : mySQLResult_t() { std::swap(result, res.result); }
mySQLResult_t::~mySQLResult_t() noexcept { if (valid()) mysql_free_result(result); }
mySQLResult_t &mySQLResult_t::operator =(mySQLResult_t &&res) noexcept { std::swap(result, res.result); return *this; }
uint64_t mySQLResult_t::numRows() const noexcept { return valid() ? mysql_num_rows(result) : 0; }
mySQLRow_t mySQLResult_t::resultRows() const noexcept { return mySQLRow_t(result); }

mySQLRow_t::mySQLRow_t(MYSQL_RES *const res) noexcept : result(res), fields(res ? mysql_num_fields(res) : 0),
	fieldTypes(new (std::nothrow) mySQLFieldType_t[fields]())
{
	if (result)
		fetch();
}

mySQLRow_t::mySQLRow_t(mySQLRow_t &&r) noexcept : result(r.result), fields(r.fields), fieldTypes(std::move(r.fieldTypes))
{
	std::swap(row, r.row);
	std::swap(rowLengths, r.rowLengths);
}

mySQLRow_t::~mySQLRow_t() noexcept
{
	while (valid())
		fetch();
}

void mySQLRow_t::fetch() noexcept
{
	if (result && fieldTypes)
	{
		row = mysql_fetch_row(result);
		rowLengths = mysql_fetch_lengths(result);
		if (valid())
		{
			MYSQL_FIELD *field = mysql_fetch_fields(result);
			for (uint32_t i = 0; i < fields; ++i)
				fieldTypes[i] = row[i] ? field[i].type : MYSQL_TYPE_NULL;
		}
	}
}

uint32_t mySQLRow_t::numFields() const noexcept { return valid() ? fields : 0; }

bool mySQLRow_t::next() noexcept
{
	if (valid())
		fetch();
	return valid();
}

mySQLValue_t mySQLRow_t::operator [](const uint32_t idx) const noexcept
{
	if (!valid() || idx >= fields)
		return mySQLValue_t();
	return mySQLValue_t(row[idx], rowLengths[idx], fieldTypes[idx]);
}

inline bool isNumber(const char x) noexcept { return x >= '0' && x <= '9'; }
inline bool isMinus(const char x) noexcept { return x == '-'; }

mySQLValue_t::mySQLValue_t(const char *const _data, const uint64_t _len, const mySQLFieldType_t _type) noexcept :
	data(_data), len(_len), type(_type) { }
bool mySQLValue_t::isNull() const noexcept { return !data || type == MYSQL_TYPE_NULL; }

std::unique_ptr<char []> mySQLValue_t::asString() const
{
	if (isNull())
		return nullptr;
	const size_t strLen = data[len - 1] == 0 ? len : len + 1;
	std::unique_ptr<char []> ret(new char[strLen]());
	memcpy(ret.get(), data, len);
	ret[strLen - 1] = 0;
	return ret;
}

bool mySQLValue_t::asBool(const uint8_t bit) const
{
	if (isNull() || type != MYSQL_TYPE_BIT || bit >= 64)
		throw mySQLValueError_t(mySQLErrorType_t::boolError);
	const char byte = data[bit >> 3];
	return byte & (bit & 7);
}

template<typename T, mySQLErrorType_t errorType> valueOrError_t<T, mySQLValueError_t> checkedConvertInt(const char *const data, const uint64_t len) noexcept
{
	typedef typename make_unsigned<T>::type U;
	const bool sign = is_signed<T>::value && isMinus(data[0]);
	const uint64_t numLen = data[len - 1] != 0 ? len : len - 1;
	U preNum = 0, num = 0;
	for (uint64_t i = 0; i < numLen; ++i)
	{
		if (sign && i == 0)
			continue;
		else if (!isNumber(data[i]))
			return mySQLValueError_t(errorType);
		num *= 10;
		if ((num / 10) < preNum)
			return mySQLValueError_t(errorType);
		preNum = num;
		num += data[i] - '0';
	}
	if (num < preNum)
		return mySQLValueError_t(errorType);
	else if (sign)
		return T(-num);
	return T(num);
}

uint8_t mySQLValue_t::asUint8() const
{
	if (isNull() || type != MYSQL_TYPE_TINY)
		throw mySQLValueError_t(mySQLErrorType_t::uint8Error);
	auto num = checkedConvertInt<uint8_t, mySQLErrorType_t::uint8Error>(data, len);
	if (num.isError())
		throw num.error();
	return num;
}

int8_t mySQLValue_t::asInt8() const
{
	if (isNull() || type != MYSQL_TYPE_TINY)
		throw mySQLValueError_t(mySQLErrorType_t::int8Error);
	auto num = checkedConvertInt<int8_t, mySQLErrorType_t::int8Error>(data, len);
	if (num.isError())
		throw num.error();
	return num;
}

uint16_t mySQLValue_t::asUint16() const
{
	if (isNull() || type != MYSQL_TYPE_SHORT)
		throw mySQLValueError_t(mySQLErrorType_t::uint16Error);
	auto num = checkedConvertInt<uint16_t, mySQLErrorType_t::uint16Error>(data, len);
	if (num.isError())
		throw num.error();
	return num;
}

int16_t mySQLValue_t::asInt16() const
{
	if (isNull() || type != MYSQL_TYPE_SHORT)
		throw mySQLValueError_t(mySQLErrorType_t::uint16Error);
	auto num = checkedConvertInt<int16_t, mySQLErrorType_t::int16Error>(data, len);
	if (num.isError())
		throw num.error();
	return num;
}

uint32_t mySQLValue_t::asUint32() const
{
	if (isNull() || type != MYSQL_TYPE_LONG)
		throw mySQLValueError_t(mySQLErrorType_t::uint32Error);
	auto num = checkedConvertInt<uint32_t, mySQLErrorType_t::uint32Error>(data, len);
	if (num.isError())
		throw num.error();
	return num;
}

int32_t mySQLValue_t::asInt32() const
{
	if (isNull() || type != MYSQL_TYPE_LONG)
		throw mySQLValueError_t(mySQLErrorType_t::int32Error);
	auto num = checkedConvertInt<int32_t, mySQLErrorType_t::int32Error>(data, len);
	if (num.isError())
		throw num.error();
	return num;
}

uint64_t mySQLValue_t::asUint64() const
{
	if (isNull() || type != MYSQL_TYPE_LONGLONG)
		throw mySQLValueError_t(mySQLErrorType_t::uint64Error);
	auto num = checkedConvertInt<uint64_t, mySQLErrorType_t::uint64Error>(data, len);
	if (num.isError())
		throw num.error();
	return num;
}

int64_t mySQLValue_t::asInt64() const
{
	if (isNull() || type != MYSQL_TYPE_LONGLONG)
		throw mySQLValueError_t(mySQLErrorType_t::int64Error);
	auto num = checkedConvertInt<int64_t, mySQLErrorType_t::int64Error>(data, len);
	if (num.isError())
		throw num.error();
	return num;
}
