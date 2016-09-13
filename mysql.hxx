#ifndef MYSQL__HXX
#define MYSQL__HXX

#include <stdint.h>
#include <mysql.h>
#include <utility>
#include <memory>

struct mySQLValue_t
{
private:
	const char *const data;
	const uint64_t len;
	const uint8_t type;

	//bool checkNumber(bool allowMinus) const noexcept;

public:
	mySQLValue_t(const char *const data, const uint64_t len, const uint8_t type) noexcept;
	bool isNull() const noexcept { return data == nullptr; }

	std::unique_ptr<char []>asSting() const;
	bool asBool() const;
	uint32_t asUint8() const;
	int32_t asInt8() const;
	uint32_t asUint16() const;
	int32_t asInt16() const;
	uint32_t asUint32() const;
	int32_t asInt32() const;
	uint64_t asUint64() const;
	int64_t asInt64() const;

	operator std::unique_ptr<char []>() const { return asString(); }
	operator const char *() const { return asString().get(); }
	explicit operator bool() const { return asBool(); }
	operator uint8_t() const { return asUint8(); }
	operator int8_t() const { return asInt8(); }
	operator uint16_t() const { return asUint16(); }
	operator int16_t() const { return asInt16(); }
	operator uint32_t() const { return asUint32(); }
	operator int32_t() const { return asInt32(); }
	operator uint64_t() const { return asUint64(); }
	operator int64_t() const { return asInt64(); }
};

struct mySQLRow_t
{
private:
	MYSQL_RES *const result;
	MYSQL_ROW row;
	sql_ulong_t *const rowLengths;

	mySQLRow_t(MYSQL_RES *const result) noexcept;
	void fetch() noexcept;

public:
	constexpr mySQLRow_t() noexcept : result(nullptr), row(nullptr), rowLengths(nullptr) {}
	~MySQLRow() noexcept;
	uint32_t numFields() const noexcept;
	bool next() noexcept;
	mySQLValue_t operator [](const uint32_t idx) const noexcept;
	bool valid() const noexcept;

	mySQLRow_t(const mySQLRow_t &) = delete;
};

struct mySQLClient_t
{
private:
	MYSQL *const con;
	bool haveConnection;

	constexpr mySQLClient_t() noexcept : con(nullptr), haveConnection(false) {}

public:
	bool connect(const char *host, uint32_t port, const char *user, const char *passwd) const noexcept;
	bool connect(const char *unixSocket, const char *user, const char *passwd) const noexcept;
	bool selectDB(const char *db) const noexcept;
	//bool query(const char *queryStmt, ...) MySQL_FORMAT_ARGS(2, 3);
	mySQLResult_t queryResult() noexcept;
	uint32_t errorNum() const noexcept;
	const char *error() const noexcept;
};

extern mySQLClient_t &database;

#endif /*MYSQL__HXX*/
