#include <new>
#include "mysql.hxx"
#include "value.hxx"

using namespace std;

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

bool mySQLClient_t::query(const char *const queryStmt, ...) noexcept
{
	(void)queryStmt;
	return true;
}

mySQLResult_t mySQLClient_t::queryResult() const noexcept { return valid() ? mySQLResult_t(con) : mySQLResult_t(); }
uint32_t mySQLClient_t::errorNum() const noexcept { return valid() ? mysql_errno(con) : 0; }
const char *mySQLClient_t::error() const noexcept { return valid() ? mysql_error(con) : nullptr; }

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

inline bool isNumber(const char x) noexcept	{ return x >= '0' && x <= '9'; }
inline bool isMinus(const char x) noexcept { return x == '-'; }

bool mySQLValue_t::isNull() const noexcept { return !data || type == MYSQL_TYPE_NULL; }

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
		if (!isNumber(data[i]))
			return mySQLValueError_t(errorType);
		num *= 10;
		if ((num / 10) < preNum)
			return mySQLValueError_t(errorType);
		preNum = num;
		num += data[i] - '0';
	}
	if (num < preNum)
		return mySQLValueError_t(errorType);
	if (sign)
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
