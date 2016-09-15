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

mySQLClient_t::mySQLClient_t(const mySQLClient_t &) noexcept { return ++handles; }

mySQLClient_t::~mySQLClient_t() noexcept
{
	--handles;
	if (!handles)
	{
		if (con)
			mysql_close(con);
	}
}

mySQLClient__t::operator =(const mySQLClient_t &) noexcept { return *this; }

bool mySQLClient_t::connect(const char *const host, const uint32_t port, const char *const user, const char *const passwd) const noexcept
	{ return valid() && mysql_real_connect(con, host, user, passwd, nullptr, port, nullptr, CLIENT_IGNORE_SIGPIPE) != nullptr; }

bool mySQLClient_t::connect(const char *const unixSocket, const char *const user, const char *const passwd) const noexcept
	{ return valid() && mysql_real_connect(con, nullptr, user, passwd, nullptr, 0, unixSocket, CLIENT_IGNORE_SIGPIPE) != nullptr; }

bool mySQLClient_t::selectDB(const char *const db) const noexcept
	{ return valid() && mysql_select_db(con, db) == 0; }

uint32_t mySQLRow_t::numFields() const noexcept { return mysql_num_fields(result); }

inline bool isNumber(const char x) noexcept	{ return x >= '0' && x <= '9'; }
inline bool isMinus(const char x) noexcept { return x == '-'; }

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
