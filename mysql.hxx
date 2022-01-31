#ifndef MYSQL_HXX
#define MYSQL_HXX

#include <cstdint>
#include <mysql.h>
#include <utility>
#include <substrate/managed_ptr>
#include "tmplORM.hxx"

/*!
 * @file
 * @author Rachel Mant
 * @date 2016-2020
 * @brief Defines the interface to the MySQL abstraction layer
 */

namespace tmplORM
{
	namespace mysql
	{
		namespace driver
		{
using std::nullptr_t;
using sql_ulong_t = unsigned long;
#define MySQL_FORMAT_ARGS(n, m) __attribute__((format(printf, n, m)))
using mySQLFieldType_t = enum enum_field_types;
using namespace tmplORM::types::baseTypes;
using tmplORM::common::fieldLength_t;
tmplORM_FNAPI const char nullParam;

struct tmplORM_API mySQLValue_t final
{
private:
	const char *data{nullptr};
	uint64_t len{0};
	mySQLFieldType_t type{MYSQL_TYPE_NULL};

public:
	/*! @brief Default constructor for value objects, constructing the null value by default */
	constexpr mySQLValue_t() noexcept = default;
	mySQLValue_t(const char *const _data, const uint64_t _len, const mySQLFieldType_t _type) noexcept;
	mySQLValue_t(mySQLValue_t &&value) noexcept : mySQLValue_t() { swap(value); }
	~mySQLValue_t() noexcept = default;
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
	float asFloat() const;
	double asDouble() const;
	ormDate_t asDate() const;
	ormDateTime_t asDateTime() const;
	ormUUID_t asUUID() const;

	/*! @brief Auto-converter for strings */
	operator std::unique_ptr<char []>() const { return asString(); }
	/*! @brief Auto-converter for raw strings */
	//operator const char *() const { return data; }
	/*! @brief Auto-converter for booleans */
	explicit operator bool() const { return asBool(0); }
	/*! @brief Auto-converter for uint8_t's */
	operator uint8_t() const { return asUint8(); }
	/*! @brief Auto-converter for int8_t's */
	operator int8_t() const { return asInt8(); }
	/*! @brief Auto-converter for uint16_t's */
	operator uint16_t() const { return asUint16(); }
	/*! @brief Auto-converter for int16_t's */
	operator int16_t() const { return asInt16(); }
	/*! @brief Auto-converter for uint32_t's */
	operator uint32_t() const { return asUint32(); }
	/*! @brief Auto-converter for int32_t's */
	operator int32_t() const { return asInt32(); }
	/*! @brief Auto-converter for uint64_t's */
	operator uint64_t() const { return asUint64(); }
	/*! @brief Auto-converter for int64_t's */
	operator int64_t() const { return asInt64(); }
	/*! @brief Auto-converter for float's */
	operator float() const { return asFloat(); }
	/*! @brief Auto-converter for double's */
	operator double() const { return asDouble(); }
	/*! @brief Auto-converter for ormDate_t's */
	operator ormDate_t() const { return asDate(); }
	/*! @brief Auto-converter for ormDateTime_t's */
	operator ormDateTime_t() const { return asDateTime(); }
	/*! @brief Auto-converter for ormUUID_t's */
	operator ormUUID_t() const { return asUUID(); }

	/*! @brief Deleted copy constructor for mySQLValue_t as wrapped values are not copyable */
	mySQLValue_t(const mySQLValue_t &) = delete;
	/*! @brief Deleted copy assignment operator for mySQLValue_t as wrapped values are not copyable */
	mySQLValue_t &operator =(const mySQLValue_t &) = delete;
};

inline void swap(mySQLValue_t &x, mySQLValue_t &y) noexcept { x.swap(y); }

struct tmplORM_API mySQLRow_t final
{
private:
	MYSQL_RES *result{nullptr};
	MYSQL_ROW row{nullptr};
	uint32_t fields{0};
	sql_ulong_t *rowLengths{nullptr};
	// fixedVector_t?
	std::unique_ptr<mySQLFieldType_t []> fieldTypes{};

	mySQLRow_t(MYSQL_RES *const result) noexcept;
	void fetch() noexcept;
	friend struct mySQLResult_t;

public:
	/*! @brief Default constructor for row objects, constructing invalid row objects by default */
	mySQLRow_t() noexcept = default;
	mySQLRow_t(mySQLRow_t &&row) noexcept : mySQLRow_t{} { swap(row); }
	~mySQLRow_t() noexcept;
	void operator =(mySQLRow_t &&row) noexcept { swap(row); }
	/*!
	 * @brief Call to determine if this row object is valid
	 * @returns true if the object is valid, false otherwise
	 */
	bool valid() const noexcept { return row && rowLengths && fieldTypes; }
	uint32_t numFields() const noexcept;
	bool next() noexcept;
	mySQLValue_t operator [](const uint32_t idx) const noexcept;
	void swap(mySQLRow_t &row) noexcept;

	/*! @brief Deleted copy constructor for mySQLRow_t as rows are not copyable */
	mySQLRow_t(const mySQLRow_t &) = delete;
	/*! @brief Deleted copy assignment operator for mySQLRow_t as rows are not copyable */
	mySQLRow_t &operator =(const mySQLRow_t &) = delete;
};

inline void swap(mySQLRow_t &a, mySQLRow_t &b) noexcept { a.swap(b); }

struct tmplORM_API mySQLResult_t final
{
private:
	MYSQL_RES *result{nullptr};

protected:
	mySQLResult_t(MYSQL *const con) noexcept;
	friend struct mySQLClient_t;

public:
	/*! @brief Default constructor for result objects, constructing invalid result objects by default */
	constexpr mySQLResult_t() noexcept = default;
	mySQLResult_t(mySQLResult_t &&res) noexcept;
	~mySQLResult_t() noexcept;
	mySQLResult_t &operator =(mySQLResult_t &&res) noexcept;
	/*!
	 * @brief Call to determine if this result object is valid
	 * @returns true if the object is valid, false otherwise
	 */
	bool valid() const noexcept { return result; }
	uint64_t numRows() const noexcept;
	mySQLRow_t resultRows() const noexcept;

	/*! @brief Deleted copy constructor for mySQLResult_t as results are not copyable */
	mySQLResult_t(const mySQLResult_t &) = delete;
	/*! @brief Deleted copy assignment operator for mySQLResult_t as results are not copyable */
	mySQLResult_t &operator =(const mySQLResult_t &) = delete;
};

struct mySQLBind_t final
{
private:
	fixedVector_t<MYSQL_BIND> params{};
	fixedVector_t<substrate::managedPtr_t<void>> paramStorage{};
	size_t numParams{0};

	void resetParams() noexcept;

protected:
	mySQLBind_t(const size_t paramsCount) noexcept;
	friend struct mySQLPreparedResult_t;
	friend struct mySQLPreparedQuery_t;

public:
	mySQLBind_t() noexcept = default;
	mySQLBind_t(mySQLBind_t &&binds) noexcept;
	~mySQLBind_t() noexcept = default;
	void operator =(mySQLBind_t && binds) noexcept;

	bool valid() const noexcept { return !numParams || params; }
	bool haveData() const noexcept { return params.valid(); }
	MYSQL_BIND *data() const noexcept { return params.data(); }
	template<typename T> void bindIn(const size_t index, const T &value, const fieldLength_t length) noexcept;
	template<typename T> void bindIn(const size_t index, const nullptr_t, const fieldLength_t length) noexcept;
	template<typename T> void bindOut(const size_t index, const fieldLength_t length) noexcept;
	size_t count() const noexcept { return numParams; }

	mySQLBind_t(const mySQLBind_t &) = delete;
	mySQLBind_t &operator =(const mySQLBind_t &) = delete;
};

struct tmplORM_API mySQLPreparedResult_t final
{
private:
	MYSQL_STMT *const query{nullptr};
	mySQLBind_t columns{};

protected:
	mySQLPreparedResult_t(MYSQL_STMT *const query, const size_t columnCount) noexcept;
	friend struct mySQLPreparedQuery_t;

public:
	mySQLPreparedResult_t() noexcept = default;
	mySQLPreparedResult_t(mySQLPreparedResult_t &&res) noexcept;
	~mySQLPreparedResult_t() noexcept = default;
	/*!
	 * @brief Call to determine if this prepared query object is valid
	 * @returns true if the object is valid, false otherwise
	 */
	bool valid() const noexcept { return columns.valid(); }
	template<typename T> void bind(const size_t index, const fieldLength_t length) noexcept { columns.bindOut<T>(index, length); }
	void bindForBuffer(const size_t index) noexcept;
	uint64_t numRows() const noexcept;
	bool next() const noexcept;
	void fetchColumn(const size_t index) const noexcept;

	mySQLPreparedResult_t(const mySQLPreparedResult_t &) = delete;
	mySQLPreparedResult_t &operator =(const mySQLPreparedResult_t &) = delete;
	mySQLPreparedResult_t &operator =(mySQLPreparedResult_t &&) = delete;
};

struct tmplORM_API mySQLPreparedQuery_t final
{
private:
	MYSQL_STMT *query{nullptr};
	mySQLBind_t params{};
	bool executed{false};

	void dtor() noexcept;

protected:
	mySQLPreparedQuery_t(MYSQL *con, const char *queryStmt, size_t paramsCount) noexcept;
	friend struct mySQLClient_t;

public:
	/*! @brief Default constructor for prepared query objects, constructing an invalid query by default */
	mySQLPreparedQuery_t() noexcept = default;
	mySQLPreparedQuery_t(mySQLPreparedQuery_t &&qry) noexcept;
	~mySQLPreparedQuery_t() noexcept;
	void operator =(mySQLPreparedQuery_t &&qry) noexcept;
	/*!
	 * @brief Call to determine if this prepared query object is valid
	 * @returns true if the object is valid, false otherwise
	 */
	bool valid() const noexcept { return query; }
	bool execute() noexcept;
	uint64_t rowID() const noexcept;
	template<typename T> void bind(size_t index, const T &value, fieldLength_t length) noexcept
		{ params.bindIn(index, value, length); }
	template<typename T> void bind(size_t index, const nullptr_t, fieldLength_t length) noexcept
		{ params.bindIn<T>(index, nullptr, length); }
	mySQLPreparedResult_t queryResult(size_t columnCount) const noexcept;
	uint32_t errorNum() const noexcept;
	const char *error() const noexcept;

	/*! @brief Deleted copy constructor for mySQLPreparedQuery_t as prepared queries are not copyable */
	mySQLPreparedQuery_t(const mySQLPreparedQuery_t &) = delete;
	/*! @brief Deleted copy assignment operator for mySQLPreparedQuery_t as prepared queries are not copyable */
	mySQLPreparedQuery_t &operator =(const mySQLPreparedQuery_t &) = delete;
};

struct tmplORM_API mySQLClient_t final
{
private:
	static MYSQL *con;
	static uint32_t handles;
	static bool haveConnection;

public:
	mySQLClient_t() noexcept;
	mySQLClient_t(const mySQLClient_t &) noexcept;
	~mySQLClient_t() noexcept;
	/*! @brief "Copies" a MySQL client connection container reference over another */
	mySQLClient_t &operator =(const mySQLClient_t &) noexcept = default;
	/*!
	 * @brief Call to determine if this client connection container is valid
	 * @returns true if the object is valid, false otherwise
	 */
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

	/*! @brief Deleted move constructor for mySQLClient_t as client connections are not movable */
	mySQLClient_t(mySQLClient_t &&) = delete;
	/*! @brief Deleted move assignment operator for mySQLClient_t as client connections are not movable */
	mySQLClient_t &operator =(mySQLClient_t &&) = delete;
};

enum class mySQLErrorType_t : uint8_t
{
	noError, queryError,
	stringError, boolError,
	uint8Error, int8Error,
	uint16Error, int16Error,
	uint32Error, int32Error,
	uint64Error, int64Error,
	floatError, doubleError,
	dateError, dateTimeError,
	uuidError
};

struct tmplORM_API mySQLValueError_t final : std::exception
{
private:
	mySQLErrorType_t errorType{mySQLErrorType_t::noError};

public:
	mySQLValueError_t() noexcept = default;
	mySQLValueError_t(const mySQLErrorType_t type) noexcept : errorType{type} { }
	mySQLValueError_t(const mySQLValueError_t &) noexcept = default;
	mySQLValueError_t(mySQLValueError_t &&) noexcept = default;
	~mySQLValueError_t() noexcept final = default;
	mySQLValueError_t &operator =(const mySQLValueError_t &) noexcept = default;
	mySQLValueError_t &operator =(mySQLValueError_t &&) noexcept = default;
	const char *error() const noexcept;
	const char *what() const noexcept final { return error(); }

	bool operator ==(const mySQLValueError_t &error) const noexcept { return errorType == error.errorType; }
	bool operator !=(const mySQLValueError_t &error) const noexcept { return errorType != error.errorType; }
};
#undef MySQL_FORMAT_ARGS
		} // namespace driver
	} // namespace mysql
} // namespace tmplORM

#endif /*MYSQL_HXX*/
