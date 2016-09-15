#include "mysql.hxx"
#include "value.hxx"

bool mySQLClient_t::connect(const char *const host, const uint32_t port, const char *const user, const char *const passwd) const noexcept
	{ return valid() && mysql_real_connect(con, host, user, passwd, nullptr, port, nullptr, CLIENT_IGNORE_SIGPIPE) != nullptr; }

bool mySQLClient_t::connect(const char *const unixSocket, const char *const user, const char *const passwd) const noexcept
	{ return valid() && mysql_real_connect(con, nullptr, user, passwd, nullptr, 0, unixSocket, CLIENT_IGNORE_SIGPIPE) != nullptr; }

bool mySQLClient_t::selectDB(const char *const db) const noexcept
	{ return valid() && mysql_select_db(con, db) == 0; }

uint32_t mySQLRow_t::numFields() noexcept { return mysql_num_fields(result); }

inline bool isNumber(const char x) noexcept
	{ return x >= '0' && x <= '9'; }

inline bool isMinus(const char x) noexcept
	{ return x == '-'; }
