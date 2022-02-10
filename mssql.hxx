#ifndef MSSQL_HXX
#define MSSQL_HXX

#include <cstdint>
#include <array>
#include <memory>
#include <utility>
#include <substrate/managed_ptr>
#include "tmplORM.hxx"

/*!
 * @file
 * @author Rachel Mant
 * @date 2016-2020
 * @brief Defines the interface to the MSSQL (Transact-SQL) abstraction layer
 */

namespace tmplORM
{
	namespace mssql
	{
		namespace driver
		{
// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
using stringPtr_t = std::unique_ptr<const char []>;
struct tSQLClient_t;
using namespace tmplORM::types::baseTypes;
using tmplORM::common::fieldLength_t;

enum class tSQLExecErrorType_t : uint8_t
{
	ok, connect,
	query, handleInv,
	generalError, needData,
	noData, dataAvail,
	unknown
};

struct tmplORM_API tSQLExecError_t final
{
private:
	using sqlState_t = std::array<char, 6>;
	tSQLExecErrorType_t _error{tSQLExecErrorType_t::ok};
	sqlState_t _state{{}};
	// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
	std::unique_ptr<char []> _message{};

public:
	tSQLExecError_t() noexcept = default;
	tSQLExecError_t(const tSQLExecErrorType_t error, const int16_t handleType, void *const handle) noexcept;
	tSQLExecError_t(tSQLExecError_t &&err) noexcept : tSQLExecError_t() { *this = std::move(err); }
	~tSQLExecError_t() noexcept = default;
	void operator =(tSQLExecError_t &&err) noexcept;

	const char *error() const noexcept;
	const char *state() const noexcept { return _state.data(); }
	const char *message() const noexcept { return _message.get(); }
	tSQLExecErrorType_t errorNum() const noexcept { return _error; }

	bool operator ==(const tSQLExecError_t &err) const noexcept { return _error == err._error && _state == err._state; }
	bool operator !=(const tSQLExecError_t &err) const noexcept { return _error != err._error || _state != err._state; }
	bool operator ==(const tSQLExecErrorType_t &error) const noexcept { return _error == error; }
	bool operator !=(const tSQLExecErrorType_t &error) const noexcept { return _error != error; }

	tSQLExecError_t(const tSQLExecError_t &) = delete;
	tSQLExecError_t &operator =(const tSQLExecError_t &) = delete;
};

enum class tSQLErrorType_t : uint8_t
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

struct tmplORM_API tSQLValue_t final
{
private:
	mutable stringPtr_t data{};
	uint64_t length{0};
	int16_t type{0};

	template<typename T> T reinterpret() const noexcept;
	template<int16_t rawType, int16_t, tSQLErrorType_t error, typename T> T asInt(int16_t type) const;

public:
	/*! @brief Default constructor for value objects, constructing the null value by default */
	tSQLValue_t() noexcept = default;
	tSQLValue_t(const void *const _data, const uint64_t _length, const int16_t _type) noexcept;
	tSQLValue_t(tSQLValue_t &&value) noexcept : tSQLValue_t() { *this = std::move(value); }
	~tSQLValue_t() noexcept = default;
	void operator =(tSQLValue_t &&value) noexcept;

	bool isNull() const noexcept { return !data; }
	// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
	std::unique_ptr<const char []> asString(const bool release = true) const;
	/*! @brief Converter for arbitrary binary buffers */
	const void *asBuffer(size_t &bufferLength, const bool release = false) const;
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
	// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
	operator std::unique_ptr<const char []>() const { return asString(); }
	/*! @brief Auto-converter to raw buffer */
	//operator const char *() const noexcept { return data.get(); }
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

	/*! @brief Deleted copy constructor for tSQLValue_t as wrapped values are not copyable */
	tSQLValue_t(const tSQLValue_t &) = delete;
	/*! @brief Deleted copy assignment operator for tSQLValue_t as wrapped values are not copyable */
	tSQLValue_t &operator =(const tSQLValue_t &) = delete;
};

struct tmplORM_API tSQLResult_t final
{
private:
	using fieldType_t = std::pair<int16_t, uint32_t>;
	const tSQLClient_t *client{nullptr};
	void *queryHandle{nullptr};
	bool _hasData{false};
	bool _freeHandle{false};
	uint16_t fields{0};
	// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
	std::unique_ptr<fieldType_t []> fieldInfo{};
	mutable fixedVector_t<tSQLValue_t> valueCache{};
	static tSQLValue_t nullValue;

protected:
	tSQLResult_t(const tSQLClient_t *const _client, void *handle, const bool hasData, const bool freeHandle = true) noexcept;
	bool error(const int16_t err) const noexcept;
	friend struct tSQLQuery_t;

public:
	/*! @brief Default constructor for result objects, constructing invalid result objects by default */
	tSQLResult_t() noexcept = default;
	tSQLResult_t(tSQLResult_t &&res) noexcept : tSQLResult_t() { *this = std::move(res); }
	~tSQLResult_t() noexcept;
	void operator =(tSQLResult_t &&res) noexcept;
	/*!
	 * @brief Call to determine if this result object is valid
	 * @returns true if the object is valid, false otherwise
	 */
	bool valid() const noexcept { return client && queryHandle && (fields == 0 || (fieldInfo && _hasData)); }
	bool hasData() const noexcept { return _hasData; }
	uint64_t numRows() const noexcept;
	uint16_t numFields() const noexcept { return fields; }
	bool next() const noexcept;
	tSQLValue_t &operator [](const uint16_t idx) const noexcept;

	/*! @brief Deleted copy constructor for tSQLResult_t as results are not copyable */
	tSQLResult_t(const tSQLResult_t &) = delete;
	/*! @brief Deleted copy assignment operator for tSQLResult_t as results are not copyable */
	tSQLResult_t &operator =(const tSQLResult_t &) = delete;
};

// TODO: Introduce multiQuery support in here.. we know which we are based on whether this has been created via query() or prepare() down below.
struct tmplORM_API tSQLQuery_t final
{
private:
	const tSQLClient_t *client{nullptr};
	void *queryHandle{nullptr};
	size_t numParams{0};
	fixedVector_t<substrate::managedPtr_t<void>> paramStorage{};
	fixedVector_t<long> dataLengths{};
	mutable bool executed{false};

protected:
	tSQLQuery_t(const tSQLClient_t *const parent, void *handle, const char *const queryStmt, const size_t paramsCount) noexcept;
	bool error(const int16_t err) const noexcept;
	friend struct tSQLClient_t;

public:
	/*! @brief Default constructor for prepared query objects, constructing an invalid query by default */
	tSQLQuery_t() noexcept = default;
	tSQLQuery_t(tSQLQuery_t &&qry) noexcept : tSQLQuery_t{} { swap(qry); }
	~tSQLQuery_t() noexcept;
	void operator =(tSQLQuery_t &&qry) noexcept { swap(qry); }
	/*!
	 * @brief Call to determine if this prepared query object is valid
	 * @returns true if the object is valid, false otherwise
	 */
	bool valid() const noexcept { return client && queryHandle; }
	tSQLResult_t execute() const noexcept;
	template<typename T> void bind(const size_t index, const T &value, const fieldLength_t length) noexcept;
	template<typename T> void bind(const size_t index, const std::nullptr_t, const fieldLength_t length) noexcept;
	void swap(tSQLQuery_t &qry) noexcept;

	/*! @brief Deleted copy constructor for tSQLQuery_t as prepared queries are not copyable */
	tSQLQuery_t(const tSQLQuery_t &) = delete;
	/*! @brief Deleted copy assignment operator for tSQLQuery_t as prepared queries are not copyable */
	tSQLQuery_t &operator =(const tSQLQuery_t &) = delete;
};

struct tmplORM_API tSQLClient_t final
{
private:
	void *dbHandle;
	void *connection;
	mutable bool haveConnection, needsCommit;
	mutable tSQLExecError_t _error;

protected:
	bool error(const int16_t err, const int16_t handleType, void *const handle) const noexcept;
	bool error(const tSQLExecErrorType_t err, const int16_t handleType, void *const handle) const noexcept;
	bool error(const tSQLExecErrorType_t err) const noexcept;
	friend struct tSQLResult_t;
	friend struct tSQLQuery_t;

public:
	tSQLClient_t() noexcept;
	tSQLClient_t(tSQLClient_t &&con) noexcept;
	~tSQLClient_t() noexcept;
	void operator =(tSQLClient_t &&con) noexcept;
	/*!
	 * @brief Call to determine if this client connection container is valid
	 * @returns true if the object is valid, false otherwise
	 */
	bool valid() const noexcept { return dbHandle && connection && haveConnection; }
	bool connect(const char *driver, const char *host, uint32_t port, const char *user, const char *passwd) const noexcept;
	void disconnect() const noexcept;
	bool selectDB(const char *db) const noexcept;
	bool beginTransact() const noexcept;
	bool endTransact(bool commitSuccess) const noexcept;
	bool commit() const noexcept { return endTransact(true); }
	bool rollback() const noexcept { return endTransact(false); }
	tSQLResult_t query(const char *queryStmt) const noexcept;
	tSQLQuery_t prepare(const char *queryStmt, const size_t paramsCount) const noexcept;
	const tSQLExecError_t &error() const noexcept { return _error; }

	/*! @brief Deleted copy constructor for tSQLClient_t as client connections are not copyable */
	tSQLClient_t(const tSQLClient_t &) = delete;
	/*! @brief Deleted copy assignment operator for tSQLClient_t as client connections are not copyable */
	tSQLClient_t &operator =(const tSQLClient_t &) = delete;
};

struct tmplORM_API tSQLValueError_t final : std::exception
{
private:
	tSQLErrorType_t errorType{tSQLErrorType_t::noError};

public:
	tSQLValueError_t() noexcept = default;
	tSQLValueError_t(const tSQLErrorType_t type) noexcept : errorType{type} { }
	tSQLValueError_t(const tSQLValueError_t &) noexcept = default;
	tSQLValueError_t(tSQLValueError_t &&) noexcept = default;
	~tSQLValueError_t() noexcept final = default;
	tSQLValueError_t &operator =(const tSQLValueError_t &) noexcept = default;
	tSQLValueError_t &operator =(tSQLValueError_t &&) noexcept = default;
	const char *error() const noexcept;
	const char *what() const noexcept final { return error(); }

	bool operator ==(const tSQLValueError_t &error) const noexcept { return errorType == error.errorType; }
	bool operator !=(const tSQLValueError_t &error) const noexcept { return errorType != error.errorType; }
};
		} // namespace driver
	} // namespace mssql
} // namespace tmplORM

#endif /*MSSQL_HXX*/
