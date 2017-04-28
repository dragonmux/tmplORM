#ifndef MYSQL__HXX
#define MYSQL__HXX

#include <stdint.h>
#include <mysql.h>
#include <utility>
#include "fixedVector.hxx"
#include "managedPtr.hxx"
#include "tmplORM.hxx"

namespace tmplORM
{
	namespace mysql
	{
		namespace driver
		{
using std::nullptr_t;
typedef unsigned long sql_ulong_t;
#define MySQL_FORMAT_ARGS(n, m) __attribute__((format(printf, n, m)))
using mySQLFieldType_t = enum enum_field_types;
using tmplORM::common::fieldLength_t;

struct mySQLValue_t final
{
private:
	const char *data;
	uint64_t len;
	mySQLFieldType_t type;

public:
	constexpr mySQLValue_t() noexcept : data(nullptr), len(0), type(MYSQL_TYPE_NULL) { }
	mySQLValue_t(const char *const data, const uint64_t len, const mySQLFieldType_t type) noexcept;
	mySQLValue_t(mySQLValue_t &&value) noexcept : mySQLValue_t() { swap(value); }
	~mySQLValue_t() noexcept { }
	void operator =(mySQLValue_t &&value) noexcept { swap(value); }
	void swap(mySQLValue_t &value) noexcept;

	bool isNull() const noexcept;
	std::unique_ptr<char []> asString() const;
	bool asBool(const uint8_t bit) const;
	uint8_t asUint8() const;
	int8_t asInt8() const;
	uint16_t asUint16() const;
	int16_t asInt16() const;
	uint32_t asUint32() const;
	int32_t asInt32() const;
	uint64_t asUint64() const;
	int64_t asInt64() const;

	operator std::unique_ptr<char []>() const { return asString(); }
	operator const char *() const { return data; }
	explicit operator bool() const { return asBool(0); }
	operator uint8_t() const { return asUint8(); }
	operator int8_t() const { return asInt8(); }
	operator uint16_t() const { return asUint16(); }
	operator int16_t() const { return asInt16(); }
	operator uint32_t() const { return asUint32(); }
	operator int32_t() const { return asInt32(); }
	operator uint64_t() const { return asUint64(); }
	operator int64_t() const { return asInt64(); }

	mySQLValue_t(const mySQLValue_t &) = delete;
	mySQLValue_t &operator =(const mySQLValue_t &) = delete;
};

inline void swap(mySQLValue_t &x, mySQLValue_t &y) noexcept { x.swap(y); }

struct mySQLRow_t final
{
private:
	MYSQL_RES *const result;
	MYSQL_ROW row;
	const uint32_t fields;
	sql_ulong_t *rowLengths;
	std::unique_ptr<mySQLFieldType_t []> fieldTypes;

	mySQLRow_t(MYSQL_RES *const result) noexcept;
	void fetch() noexcept;
	friend struct mySQLResult_t;

public:
	mySQLRow_t() noexcept : result(nullptr), row(nullptr), fields(0), rowLengths(nullptr), fieldTypes() { }
	mySQLRow_t(mySQLRow_t &&r) noexcept;
	~mySQLRow_t() noexcept;

	bool valid() const noexcept { return row && rowLengths && fieldTypes; }
	uint32_t numFields() const noexcept;
	bool next() noexcept;
	mySQLValue_t operator [](const uint32_t idx) const noexcept;

	mySQLRow_t(const mySQLRow_t &) = delete;
	mySQLRow_t &operator =(const mySQLRow_t &) = delete;
};

struct mySQLResult_t final
{
private:
	MYSQL_RES *result;

protected:
	mySQLResult_t(MYSQL *const con) noexcept;
	friend struct mySQLClient_t;

public:
	constexpr mySQLResult_t() noexcept : result(nullptr) { }
	mySQLResult_t(mySQLResult_t &&res) noexcept;
	~mySQLResult_t() noexcept;
	mySQLResult_t &operator =(mySQLResult_t &&res) noexcept;

	bool valid() const noexcept { return result; }
	uint64_t numRows() const noexcept;
	mySQLRow_t resultRows() const noexcept;

	mySQLResult_t(const mySQLResult_t &) = delete;
	mySQLResult_t &operator =(const mySQLResult_t &) = delete;
};

struct mySQLPreparedQuery_t final
{
private:
	MYSQL_STMT *query;
	fixedVector_t<MYSQL_BIND> params;
	fixedVector_t<managedPtr_t<void>> paramStorage;
	size_t numParams;
	bool executed;

	void dtor() noexcept;

protected:
	mySQLPreparedQuery_t(MYSQL *const con, const char *const queryStmt, const size_t paramsCount) noexcept;
	friend struct mySQLClient_t;

public:
	mySQLPreparedQuery_t() noexcept : query(nullptr), params(), numParams(0), executed(false) { }
	mySQLPreparedQuery_t(mySQLPreparedQuery_t &&qry) noexcept;
	~mySQLPreparedQuery_t() noexcept;
	mySQLPreparedQuery_t &operator =(mySQLPreparedQuery_t &&qry) noexcept;

	bool valid() const noexcept { return query; }
	bool execute() noexcept;
	uint64_t rowID() const noexcept;
	template<typename T> void bind(const size_t index, const T &value, const fieldLength_t length) noexcept;
	template<typename T> void bind(const size_t index, const nullptr_t, const fieldLength_t length) noexcept;

	mySQLPreparedQuery_t(const mySQLPreparedQuery_t &) = delete;
	mySQLPreparedQuery_t &operator =(const mySQLPreparedQuery_t &) = delete;
};

struct mySQLClient_t final
{
private:
	static MYSQL *con;
	static uint32_t handles;
	static bool haveConnection;

public:
	mySQLClient_t() noexcept;
	mySQLClient_t(const mySQLClient_t &) noexcept;
	~mySQLClient_t() noexcept;
	mySQLClient_t &operator =(const mySQLClient_t &) noexcept;

	bool valid() const noexcept { return con && haveConnection; }
	bool connect(const char *const host, const uint32_t port, const char *const user, const char *const passwd) const noexcept;
	bool connect(const char *const unixSocket, const char *const user, const char *const passwd) const noexcept;
	void disconnect() noexcept;
	bool selectDB(const char *const db) const noexcept;
	bool query(const char *const queryStmt, ...) const noexcept MySQL_FORMAT_ARGS(2, 3);
	mySQLResult_t queryResult() const noexcept;
	mySQLPreparedQuery_t prepare(const char *const queryStmt, const size_t paramsCount) const noexcept;
	uint32_t errorNum() const noexcept;
	const char *error() const noexcept;

	mySQLClient_t(mySQLClient_t &&) = delete;
	mySQLClient_t &operator =(mySQLClient_t &&) = delete;
};

enum class mySQLErrorType_t : uint8_t
{
	noError, queryError,
	stringError, boolError,
	uint8Error, int8Error,
	uint16Error, int16Error,
	uint32Error, int32Error,
	uint64Error, int64Error
};

struct mySQLValueError_t final
{
private:
	mySQLErrorType_t errorType;

public:
	constexpr mySQLValueError_t() noexcept : errorType(mySQLErrorType_t::noError) { }
	constexpr mySQLValueError_t(mySQLErrorType_t type) noexcept : errorType(type) { }
	const char *error() const noexcept;

	bool operator ==(const mySQLValueError_t &error) const noexcept { return errorType == error.errorType; }
	bool operator !=(const mySQLValueError_t &error) const noexcept { return errorType != error.errorType; }
};
#undef MySQL_FORMAT_ARGS
		}
	}
}

#endif /*MYSQL__HXX*/
