#include <string.h>
#include "mysql.hxx"
#include "value.hxx"
#include "string.hxx"

/*!
 * @internal
 * @file
 * @author Rachel Mant
 * @date 2016-2017
 * @brief C++ MySQL driver abstraction layer for handling client connections and query datasets
 */

using namespace std;
using namespace tmplORM::mysql::driver;

// General documentation block (used to document mysql.hxx stuff cleanly
/*!
 * @internal
 * @typedef mySQLFieldType_t
 * @brief Type representing MySQL field types
 * @enum mySQLErrorType_t
 * @brief Defines the possible error types that can occur in mySQLValue_t conversions
 * @var mySQLErrorType_t::stringError
 * @brief Error converting a value to a string
 * @var mySQLErrorType_t::boolError
 * @brief Error converting a value to a boolean
 * @var mySQLErrorType_t::uint32Error
 * @brief Error converting a value to a 32-bit unsigned integer
 * @var mySQLErrorType_t::int32Error
 * @brief Error converting a value to a 32-bit signed integer
 * @var mySQLErrorType_t::uint64Error
 * @brief Error converting a value to a 64-bit unsigned integer
 * @var mySQLErrorType_t::int64Error
 * @brief Error converting a value to a 64-bit signed integer
 */

/*! @brief Global MySQL connection handle */
MYSQL *mySQLClient_t::con = nullptr;
/*! @brief Counter for the number of handles in existance to the master container */
uint32_t mySQLClient_t::handles = 0;
/*! @brief Variable that defines if mySQLClient_t::con actually refers to a valid, connected, connection */
bool mySQLClient_t::haveConnection = false;

/*! @brief Constant used for telling MySQL Client that we really want it to auto-reconnect */
static const bool autoReconnect = true;

/*! @brief Constructs a fresh MySQL client connection container */
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

/*! @brief Constructs a MySQL client connection container reference */
mySQLClient_t::mySQLClient_t(const mySQLClient_t &) noexcept { ++handles; }

/*! @brief Destructor for MySQL client connection container references */
mySQLClient_t::~mySQLClient_t() noexcept
{
	--handles;
	if (!handles)
	{
		if (con)
			mysql_close(con);
	}
}

/*! @brief "Copies" a MySQL client connection container reference over another */
mySQLClient_t &mySQLClient_t::operator =(const mySQLClient_t &) noexcept { return *this; }

/*!
 * @brief Creates a client connection based on a TCP/IP connection
 * @param host The host string of the server housing the MySQL instance to connect to
 * @param port The port number of the desired MySQL instance on the server to connect to
 * @param user The user to connect in with
 * @param passwd The password for the user to connect in with
 * @returns true if the connection was successful (or pre-existing), false otherwise
 */
bool mySQLClient_t::connect(const char *const host, const uint32_t port, const char *const user, const char *const passwd) const noexcept
{
	if (haveConnection)
		return true;
	else if (!con)
		return false;
	haveConnection = mysql_real_connect(con, host, user, passwd, nullptr, port, nullptr, CLIENT_IGNORE_SIGPIPE) != nullptr;
	return haveConnection;
}

/*!
 * @brief Creates a connection based on a Unix Socket
 * @param unixSocket The path to the Unix Socket of the MySQL instance to connect to
 * @param user The user to connect in with
 * @param passwd The password for the user to connect in with
 * @returns true if the connection was successful (or pre-existing), false otherwise
 */
bool mySQLClient_t::connect(const char *const unixSocket, const char *const user, const char *const passwd) const noexcept
{
	if (haveConnection)
		return true;
	else if (!con)
		return false;
	haveConnection = mysql_real_connect(con, nullptr, user, passwd, nullptr, 0, unixSocket, CLIENT_IGNORE_SIGPIPE) != nullptr;
	return haveConnection;
}

/*! @brief Disconnects from the current MySQL server and prepares us to connect to a new one */
void mySQLClient_t::disconnect() noexcept
{
	if (valid())
	{
		mysql_close(con);
		haveConnection = false;
		con = mysql_init(nullptr);
		if (con)
			mysql_options(con, MYSQL_OPT_RECONNECT, &autoReconnect);
	}
}

/*!
 * @brief Select a database on the current MySQL server
 * @param db The database to select
 */
bool mySQLClient_t::selectDB(const char *const db) const noexcept { return valid() && mysql_select_db(con, db) == 0; }

/*!
 * @brief Construct a query to run, and execute it
 * @param queryStmt The printf() style query statement to run
 * @param ... The parameters (if any) for the query statement
 * @returns true if the query was successful, false otherwise
 */
bool mySQLClient_t::query(const char *const queryStmt, ...) const noexcept
{
	if (mysql_ping(con))
		return false;
	va_list args;
	va_start(args, queryStmt);
	const auto query = vaFormatString(queryStmt, args);
	va_end(args);
	if (!query)
		return false;
	return mysql_query(con, query.get()) == 0;
}

/*!
 * @brief Gets the mySQLResult_t for any active query
 * @returns a mySQLResult_t that represents the result of the most recent query on the connection (if there is one)
 */
mySQLResult_t mySQLClient_t::queryResult() const noexcept { return valid() ? mySQLResult_t(con) : mySQLResult_t(); }
/*!
 * @brief Construct a prepared query to run and return that
 * @returns a mySQLPreparedQuery_t that represents the query to run for further prep and execution
 */
mySQLPreparedQuery_t mySQLClient_t::prepare(const char *const queryStmt, const size_t paramsCount) const noexcept
	{ return valid() ? mySQLPreparedQuery_t(con, queryStmt, paramsCount) : mySQLPreparedQuery_t(); }
/*!
 * @brief MySQL calls can result in an error outside this driver layer, this allows you to know what that error is if something fails
 * @returns The current MySQL errno error number code
 */
uint32_t mySQLClient_t::errorNum() const noexcept { return valid() ? mysql_errno(con) : 0; }
/*!
 * @brief MySQL calls can result in an error outside this driver layer, this allows you to know the human readable error string
 * @returns The current MySQL error string
 */
const char *mySQLClient_t::error() const noexcept { return valid() ? mysql_error(con) : nullptr; }

/*!
 * @internal
 * @brief Constructor for prepared queries from MySQL query statements
 * @param con The connection for which to prepare the query against
 * @param queryStmt The query statement to prepare
 * @param paramsCount The count of the number of parameters that the query statement contains
 */
mySQLPreparedQuery_t::mySQLPreparedQuery_t(MYSQL *const con, const char *const queryStmt, const size_t paramsCount) noexcept :
	query(mysql_stmt_init(con)), params(paramsCount), paramStorage(paramsCount), numParams(paramsCount), executed(false)
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

// General documentation block for mySQLPreparedQuery_t
/*!
 * @internal
 * @var mySQLPreparedQuery_t::query
 * @brief The MYSQL_STMT statement pointer for this prepared query object (or nullptr if invalid)
 */

/*!
 * @brief Move constructor for the context of a MySQL prepared query
 * @param qry The original prepared query who's context we will make our own in trade
 */
mySQLPreparedQuery_t::mySQLPreparedQuery_t(mySQLPreparedQuery_t &&qry) noexcept : mySQLPreparedQuery_t() { *this = std::move(qry); }

/*! @brief Destructor for MySQL result objects */
mySQLPreparedQuery_t::~mySQLPreparedQuery_t() noexcept
{
	if (valid())
		dtor();
}

/*! @internal @brief The real function for destroying the context we encapsulate */
void mySQLPreparedQuery_t::dtor() noexcept
{
	mysql_stmt_close(query);
	query = nullptr;
}

/*!
 * @brief Move assignment operator for the context of a MySQL prepared query
 * @param qry The original prepared query who's context we will make our own in trade
 */
mySQLPreparedQuery_t &mySQLPreparedQuery_t::operator =(mySQLPreparedQuery_t &&qry) noexcept
{
	std::swap(query, qry.query);
	std::swap(params, qry.params);
	std::swap(numParams, qry.numParams);
	std::swap(executed, qry.executed);
	return *this;
}

/*! @brief Executes the prepared query */
bool mySQLPreparedQuery_t::execute() noexcept
{
	if (!executed && valid())
	{
		if (params)
			mysql_stmt_bind_param(query, params.data());
		executed = mysql_stmt_execute(query) == 0;
	}
	return executed;
}

/*! @brief Returns the ID of a freshly inserted row for this prepared query, or 0 otherwise */
uint64_t mySQLPreparedQuery_t::rowID() const noexcept { return executed ? mysql_stmt_insert_id(query) : 0; }
/*!
 * @internal
 * @brief Constructor for results from MySQL queries
 * @param con The connection for which to retrieve results from
 */
mySQLResult_t::mySQLResult_t(MYSQL *const con) noexcept : result(mysql_store_result(con)) { }
/*!
 * @brief Move constructor for the results of a MySQL query
 * @param res The original result object who's results we will make our own in trade
 */
mySQLResult_t::mySQLResult_t(mySQLResult_t &&res) noexcept : mySQLResult_t() { std::swap(result, res.result); }

// General documentation block for mySQLResult_t
/*!
 * @internal
 * @var mySQLResult_t::result
 * @brief The MYSQL_RES result pointer for this result object (or nullptr if invalid)
 */

/*! @brief Destructor for MySQL result objects */
mySQLResult_t::~mySQLResult_t() noexcept { if (valid()) mysql_free_result(result); }
/*!
 * @brief Move assignment operator for the results of a MySQL query
 * @param res The original result who's results we will make our own in trade
 */
mySQLResult_t &mySQLResult_t::operator =(mySQLResult_t &&res) noexcept { std::swap(result, res.result); return *this; }
/*! @brief Returns the number of rows this result object represents, or 0 if this is an invalid result object */
uint64_t mySQLResult_t::numRows() const noexcept { return valid() ? mysql_num_rows(result) : 0; }
/*! @brief Creates a row object representing result rows for this result object */
mySQLRow_t mySQLResult_t::resultRows() const noexcept { return valid() ? mySQLRow_t(result) : mySQLRow_t(); }
/*!
 * @internal
 * @brief Constructor for the result rows from a MySQL query
 * @param result The result from which to fetch the result rows from
 */
mySQLRow_t::mySQLRow_t(MYSQL_RES *res) noexcept : result(res), row(nullptr), fields(0), rowLengths(nullptr) { fetch(); }

/*! @brief Move constructs a result rows object for a MySQL query */
mySQLRow_t::mySQLRow_t(mySQLRow_t &&r) noexcept : result(r.result), row(nullptr), fields(r.fields), rowLengths(nullptr), fieldTypes(std::move(r.fieldTypes))
{
	std::swap(row, r.row);
	// set r.fields = 0?
	std::swap(rowLengths, r.rowLengths);
}

/*!
 * @brief Destroys a result rows object, ensuring that no rows are left to fetch
 * @note This may not actually be required - maybe we can get away with invalidating the result set rather than expensively processing our way through any remaining rows?
 */
mySQLRow_t::~mySQLRow_t() noexcept { while (row) fetch(); }

/*!
 * @internal
 * @brief Fetches the next result row for this result set, or sets row to nullptr if there are no results left
 */
void mySQLRow_t::fetch() noexcept
{
	if (result)
	{
		if (!fieldTypes)
		{
			const_cast<uint32_t &>(fields) = mysql_num_fields(result);
			if (fields)
				fieldTypes = makeUnique<mySQLFieldType_t []>(fields);
			if (!fieldTypes)
				return;
			for (uint32_t i = 0; i < fields; ++i)
				fieldTypes[i] = mysql_fetch_field(result)->type;
		}
		row = mysql_fetch_row(result);
		rowLengths = mysql_fetch_lengths(result);
	}
}

/*! @brief MySQL row objects can be any number of fields wide, so this allows you to know how many are represented in this result rows object */
uint32_t mySQLRow_t::numFields() const noexcept { return valid() ? fields : 0; }

/*!
 * @brief Fetches the next row if possible
 * @returns true if the next row was successfully fetched, false for all errors
 */
bool mySQLRow_t::next() noexcept
{
	if (valid())
		fetch();
	return row;
}

/*!
 * @brief Assuming a valid field index (0-based), returns the value of that field for the current row
 * @param idx The desired row index
 * @result a null mySQLValue_t if the index was out of range or row was invalid, else the value of the field for the current row
 * @note Use numFields() to determine how many fields there are to ensure your indexing is correct
 */
mySQLValue_t mySQLRow_t::operator [](const uint32_t idx) const noexcept
{
	if (!valid() || idx >= fields)
		return mySQLValue_t();
	return mySQLValue_t(row[idx], rowLengths[idx], fieldTypes[idx]);
}

/*! @internal @brief Returns a boolean indicating if the given character is a number of not */
inline bool isNumber(const char x) noexcept { return x >= '0' && x <= '9'; }
/*! @internal @brief Returns a boolean indicating if the given character is a '-' or not */
inline bool isMinus(const char x) noexcept { return x == '-'; }

/*!
 * @internal
 * @brief Constructor for representing the value of a field in a result row
 * @param \_data The data to wrap for auto-conversion
 * @param \_len The length of the data to wrap
 * @param \_type The type of the data as a MYSQL_TYPE_* value
 */
mySQLValue_t::mySQLValue_t(const char *const _data, const uint64_t _len, const mySQLFieldType_t _type) noexcept :
	data(_data), len(_len), type(_type) { }
/*! @internal @brief Returns a boolean indicating if the value contained represents nullptr or not */
bool mySQLValue_t::isNull() const noexcept { return !data || type == MYSQL_TYPE_NULL; }

void mySQLValue_t::swap(mySQLValue_t &value) noexcept
{
	std::swap(data, value.data);
	std::swap(len, value.len);
	std::swap(type, value.type);
}

/*!
 * @throws mySQLValueError_t
 */
std::unique_ptr<char []> mySQLValue_t::asString() const
{
	if (isNull())
		return nullptr;
	// Not sure if I want this check..
	//else if (type != MYSQL_TYPE_STRING && type != MYSQL_TYPE_VAR_STRING)
	//	throw mySQLValueError_t(mySQLErrorType_t::stringError);
	const size_t dataLen = data[len - 1] ? len + 1 : len;
	auto str = makeUnique<char []>(dataLen);
	if (!str)
		return nullptr;
	memcpy(str.get(), data, len);
	str[dataLen - 1] = 0;
	return str;
}

/*!
 * @throws mySQLValueError_t
 */
bool mySQLValue_t::asBool(const uint8_t bit) const
{
	if (isNull() || type != MYSQL_TYPE_BIT || bit >= 64)
		throw mySQLValueError_t(mySQLErrorType_t::boolError);
	const char byte = data[bit >> 3];
	return byte & (1 << (bit & 7));
}

template<typename T, mySQLErrorType_t errorType> valueOrError_t<T, mySQLValueError_t> checkedConvertInt(const char *const data, const uint64_t len) noexcept
{
	using U = typename std::make_unsigned<T>::type;
	if (!len)
		return 0;
	const bool sign = is_signed<T>::value && isMinus(data[0]);
	const uint64_t numLen = data[len - 1] ? len : len - 1;
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
		return -T(num);
	return T(num);
}

/*!
 * @throws mySQLValueError_t
 */
uint8_t mySQLValue_t::asUint8() const
{
	if (isNull() || type != MYSQL_TYPE_TINY)
		throw mySQLValueError_t(mySQLErrorType_t::uint8Error);
	auto num = checkedConvertInt<uint8_t, mySQLErrorType_t::uint8Error>(data, len);
	if (num.isError())
		throw num.error();
	return num;
}

/*!
 * @throws mySQLValueError_t
 */
int8_t mySQLValue_t::asInt8() const
{
	if (isNull() || type != MYSQL_TYPE_TINY)
		throw mySQLValueError_t(mySQLErrorType_t::int8Error);
	auto num = checkedConvertInt<int8_t, mySQLErrorType_t::int8Error>(data, len);
	if (num.isError())
		throw num.error();
	return num;
}

/*!
 * @throws mySQLValueError_t
 */
uint16_t mySQLValue_t::asUint16() const
{
	if (isNull() || type != MYSQL_TYPE_SHORT)
		throw mySQLValueError_t(mySQLErrorType_t::uint16Error);
	auto num = checkedConvertInt<uint16_t, mySQLErrorType_t::uint16Error>(data, len);
	if (num.isError())
		throw num.error();
	return num;
}

/*!
 * @throws mySQLValueError_t
 */
int16_t mySQLValue_t::asInt16() const
{
	if (isNull() || type != MYSQL_TYPE_SHORT)
		throw mySQLValueError_t(mySQLErrorType_t::int16Error);
	auto num = checkedConvertInt<int16_t, mySQLErrorType_t::int16Error>(data, len);
	if (num.isError())
		throw num.error();
	return num;
}

/*!
 * @throws mySQLValueError_t
 */
uint32_t mySQLValue_t::asUint32() const
{
	if (isNull() || type != MYSQL_TYPE_LONG)
		throw mySQLValueError_t(mySQLErrorType_t::uint32Error);
	auto num = checkedConvertInt<uint32_t, mySQLErrorType_t::uint32Error>(data, len);
	if (num.isError())
		throw num.error();
	return num;
}

/*!
 * @throws mySQLValueError_t
 */
int32_t mySQLValue_t::asInt32() const
{
	if (isNull() || type != MYSQL_TYPE_LONG)
		throw mySQLValueError_t(mySQLErrorType_t::int32Error);
	auto num = checkedConvertInt<int32_t, mySQLErrorType_t::int32Error>(data, len);
	if (num.isError())
		throw num.error();
	return num;
}

/*!
 * @throws mySQLValueError_t
 */
uint64_t mySQLValue_t::asUint64() const
{
	if (isNull() || type != MYSQL_TYPE_LONGLONG)
		throw mySQLValueError_t(mySQLErrorType_t::uint64Error);
	auto num = checkedConvertInt<uint64_t, mySQLErrorType_t::uint64Error>(data, len);
	if (num.isError())
		throw num.error();
	return num;
}

/*!
 * @throws mySQLValueError_t
 */
int64_t mySQLValue_t::asInt64() const
{
	if (isNull() || type != MYSQL_TYPE_LONGLONG)
		throw mySQLValueError_t(mySQLErrorType_t::int64Error);
	auto num = checkedConvertInt<int64_t, mySQLErrorType_t::int64Error>(data, len);
	if (num.isError())
		throw num.error();
	return num;
}

const char *mySQLValueError_t::error() const noexcept
{
	switch (errorType)
	{
		case mySQLErrorType_t::noError:
			return "No error occured";
		case mySQLErrorType_t::queryError:
			return "Query failed";
		case mySQLErrorType_t::stringError:
			return "Error converting value to a string";
		case mySQLErrorType_t::boolError:
			return "Error converting value to a boolean";
		case mySQLErrorType_t::uint8Error:
			return "Error converting value to a unsigned 8-bit integer";
		case mySQLErrorType_t::int8Error:
			return "Error converting value to a signed 8-bit integer";
		case mySQLErrorType_t::uint16Error:
			return "Error converting value to a unsigned 16-bit integer";
		case mySQLErrorType_t::int16Error:
			return "Error converting value to a signed 16-bit integer";
		case mySQLErrorType_t::uint32Error:
			return "Error converting value to a unsigned 32-bit integer";
		case mySQLErrorType_t::int32Error:
			return "Error converting value to a signed 32-bit integer";
		case mySQLErrorType_t::uint64Error:
			return "Error converting value to a unsigned 64-bit integer";
		case mySQLErrorType_t::int64Error:
			return "Error converting value to a signed 64-bit integer";
	}
	return "An unknown error occured";
}
