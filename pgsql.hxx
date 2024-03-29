#ifndef PGSQL_HXX
#define PGSQL_HXX

#include <cstdint>
#include <libpq-fe.h>
#include <substrate/managed_ptr>
#include "tmplORM.hxx"

/*!
 * @file
 * @author Rachel Mant
 * @date 2022
 * @brief Defines the interface to the PostgreSQL abstraction layer
 */

namespace tmplORM
{
	namespace pgsql
	{
		namespace driver
		{
struct pgSQLClient_t;
using namespace tmplORM::types::baseTypes;
using tmplORM::common::fieldLength_t;

enum class pgSQLErrorType_t : uint8_t
{
	noError, binError,
	stringError, boolError,
	uint8Error, int8Error,
	uint16Error, int16Error,
	uint32Error, int32Error,
	uint64Error, int64Error,
	floatError, doubleError,
	dateError, dateTimeError,
	uuidError
};

enum class pgSQLType_t
{
	boolean,
	int2,
	int4,
	int8,
	float4,
	float8,
	unicode,
	unicodeText,
	binary,
	date,
	time,
	dateTime,
	uuid
};

struct tmplORM_API pgSQLValue_t final
{
private:
	const void *data{nullptr};
	Oid type{InvalidOid};

	template<typename T> inline T reinterpret() const noexcept;
	template<typename T, Oid, pgSQLErrorType_t> inline T asInt() const;
	static ormDate_t julianDateToDate(const int64_t date) noexcept;

public:
	/*! @brief Default constructor for value objects, constructing the invalid value by default */
	pgSQLValue_t() noexcept = default;
	pgSQLValue_t(std::nullptr_t) noexcept;
	pgSQLValue_t(const char *const value, Oid _type) noexcept : data{static_cast<const void *>(value)}, type{_type} { }
	pgSQLValue_t(pgSQLValue_t &&value) noexcept : pgSQLValue_t{} { swap(value); }
	~pgSQLValue_t() noexcept = default;
	void operator =(pgSQLValue_t &&value) noexcept { swap(value); }
	void swap(pgSQLValue_t &value) noexcept;

	bool valid() const noexcept { return data || type != InvalidOid; }
	bool isNull() const noexcept { return !data; }
	const char *asString() const;
	bool asBool() const;
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
	explicit operator const char *() const { return asString(); }
	/*! @brief Auto-converter for booleans */
	explicit operator bool() const { return asBool(); }
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
	/*! @brief Auto-converter for floats */
	operator float() const { return asFloat(); }
	/*! @brief Auto-converter for doubles */
	operator double() const { return asDouble(); }
	/*! @brief Auto-converter for dates */
	operator ormDate_t() const { return asDate(); }
	/*! @brief Auto-converter for date-times */
	operator ormDateTime_t() const { return asDateTime(); }
	/*! @brief Auto-converter for UUIDs */
	operator ormUUID_t() const { return asUUID(); }

	/*! @brief Deleted copy constructor for pgSQLValue_t as wrapped values are not copyable */
	pgSQLValue_t(const pgSQLValue_t &) = delete;
	/*! @brief Deleted copy assignment operator for pgSQLValue_t as wrapped values are not copyable */
	pgSQLValue_t &operator =(const pgSQLValue_t &) = delete;
};

struct tmplORM_API pgSQLResult_t final
{
private:
	PGresult *result{nullptr};
	uint32_t rows{0};
	uint32_t fields{0};
	uint32_t row{0};
	// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
	fixedVector_t<Oid> fieldInfo{};

public:
	/*! @brief Default constructor for result objects, constructing invalid result objects by default */
	constexpr pgSQLResult_t() noexcept = default;
	pgSQLResult_t(PGresult *res) noexcept;
	pgSQLResult_t(pgSQLResult_t &&res) noexcept : pgSQLResult_t{} { swap(res); }
	~pgSQLResult_t() noexcept;
	pgSQLResult_t &operator =(pgSQLResult_t &&res) noexcept;
	/*!
	 * @brief Call to determine if this result object is valid
	 * @returns true if the object is valid, false otherwise
	 */
	bool valid() const noexcept { return result && (!fields || fieldInfo.valid()); }
	uint32_t errorNum() const noexcept;
	const char *error() const noexcept;
	bool successful() const noexcept;
	bool hasData() const noexcept;
	uint32_t numRows() const noexcept { return rows; }
	uint32_t numFields() const noexcept { return fields; }
	bool next() noexcept;
	pgSQLValue_t operator [](const uint32_t idx) const noexcept;

	void swap(pgSQLResult_t &res) noexcept;

	/*! @brief Deleted copy constructor for pgSQLResult_t as results are not copyable */
	pgSQLResult_t(const pgSQLResult_t &) = delete;
	/*! @brief Deleted copy assignment operator for pgSQLResult_t as results are not copyable */
	pgSQLResult_t &operator =(const pgSQLResult_t &) = delete;
};

struct tmplORM_API pgSQLQuery_t final
{
private:
	PGconn *connection{nullptr};
	const char *query{nullptr};
	size_t numParams{0};
	fixedVector_t<Oid> paramTypes{};
	fixedVector_t<const char *> params{};
	fixedVector_t<substrate::managedPtr_t<void>> paramStorage{};
	// Postgres did a stupid and assumed 'int' was 32-bit at least and that that was big enough. The API
	// also has no sense to it re signed vs unsigned so everything is signed, even when it shouldn't be
	fixedVector_t<int> dataLengths{};

	static Oid typeToOID(pgSQLType_t type) noexcept;

protected:
	pgSQLQuery_t(PGconn *conn, const char *queryStmt, size_t paramsCount) noexcept;
	friend struct pgSQLClient_t;

public:
	/*! @brief Default constructor for prepared query objects, constructing an invalid query by default */
	constexpr pgSQLQuery_t() noexcept = default;
	pgSQLQuery_t(pgSQLQuery_t &&qry) noexcept : pgSQLQuery_t{} { swap(qry); }
	~pgSQLQuery_t() noexcept = default;
	void operator =(pgSQLQuery_t &&qry) noexcept { swap(qry); }
	/*!
	 * @brief Call to determine if this prepared query object is valid
	 * @returns true if the object is valid, false otherwise
	 */
	bool valid() const noexcept { return connection && query; }
	pgSQLResult_t execute() const noexcept;
	template<typename T> void bind(const size_t index, const T &value, const fieldLength_t length) noexcept;
	template<typename T> void bind(const size_t index, const std::nullptr_t, const fieldLength_t length) noexcept;
	void swap(pgSQLQuery_t &qry) noexcept;

	/*! @brief Deleted copy constructor for pgSQLQuery_t as prepared queries are not copyable */
	pgSQLQuery_t(const pgSQLQuery_t &) = delete;
	/*! @brief Deleted copy assignment operator for pgSQLQuery_t as prepared queries are not copyable */
	pgSQLQuery_t &operator =(const pgSQLQuery_t &) = delete;
};

struct tmplORM_API pgSQLClient_t final
{
private:
	PGconn *connection{nullptr};
	bool needsCommit{false};

public:
	pgSQLClient_t() noexcept = default;
	pgSQLClient_t(pgSQLClient_t &&con) noexcept;
	~pgSQLClient_t() noexcept;
	void operator =(pgSQLClient_t &&conn) noexcept;
	/*!
	 * @brief Call to determine if this client connection container is valid
	 * @returns true if the object is valid, false otherwise
	 */
	bool valid() const noexcept { return connection; }
	bool connect(const char *host, const char *port, const char *user, const char *passwd, const char *db) noexcept;
	void disconnect() noexcept;
	bool switchDB(const char *db) noexcept;
	bool beginTransact() noexcept;
	bool endTransact(bool commitSuccess) noexcept;
	bool commit() noexcept { return endTransact(true); }
	bool rollback() noexcept { return endTransact(false); }
	pgSQLResult_t query(const char *queryStmt) const noexcept;
	pgSQLQuery_t prepare(const char *queryStmt, const size_t paramsCount) const noexcept;
	const char *error() const noexcept;

	/*! @brief Deleted move constructor for pgSQLClient_t as client connections are not movable */
	pgSQLClient_t(const pgSQLClient_t &) = delete;
	/*! @brief Deleted move assignment operator for pgSQLClient_t as client connections are not movable */
	pgSQLClient_t &operator =(const pgSQLClient_t &) = delete;
};

struct tmplORM_API pgSQLValueError_t final : std::exception
{
private:
	pgSQLErrorType_t errorType{pgSQLErrorType_t::noError};

public:
	pgSQLValueError_t() noexcept = default;
	pgSQLValueError_t(const pgSQLErrorType_t type) noexcept : errorType{type} { }
	pgSQLValueError_t(const pgSQLValueError_t &) noexcept = default;
	pgSQLValueError_t(pgSQLValueError_t &&) noexcept = default;
	~pgSQLValueError_t() noexcept final = default;
	pgSQLValueError_t &operator =(const pgSQLValueError_t &) noexcept = default;
	pgSQLValueError_t &operator =(pgSQLValueError_t &&) noexcept = default;
	const char *error() const noexcept;
	const char *what() const noexcept final { return error(); }

	bool operator ==(const pgSQLValueError_t &error) const noexcept { return errorType == error.errorType; }
	bool operator !=(const pgSQLValueError_t &error) const noexcept { return errorType != error.errorType; }
};
		} // namespace driver
	} // namespace pgsql
} // namespace tmplORM

#endif /*PGSQL_HXX*/
