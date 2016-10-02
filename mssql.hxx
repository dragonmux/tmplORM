#ifndef MSSQL__HXX
#define MSSQL__HXX

#include <stdint.h>
#include <array>
#include <memory>
#include <utility>

namespace tmplORM
{
	namespace mssql
	{
		namespace driver
		{
using std::nullptr_t;
using stringPtr_t = std::unique_ptr<const char []>;
struct tSQLClient_t;

enum class tSQLExecErrorType_t : uint8_t
{
	ok, connect,
	query, handleInv,
	generalError, needData,
	noData, dataAvail,
	unknown
};

struct tSQLExecError_t final
{
private:
	const tSQLExecErrorType_t _error;
	std::array<char, 6> _state;
	std::unique_ptr<char []> _message;

public:
	constexpr tSQLExecError_t() noexcept : _error(tSQLExecErrorType_t::ok), _state{{}}, _message() { }
	tSQLExecError_t(const tSQLExecErrorType_t error, const int16_t handleType, void *const handle) noexcept;
	tSQLExecError_t(tSQLExecError_t &&err) noexcept : tSQLExecError_t() { *this = std::move(err); }
	~tSQLExecError_t() { }
	tSQLExecError_t &operator =(tSQLExecError_t &&err) noexcept;

	const char *error() const noexcept;
	const char *state() const noexcept { return _state.data(); }
	const char *message() const noexcept { return _message.get(); }
	tSQLExecErrorType_t errorNum() const noexcept { return _error; }

	tSQLExecError_t(const tSQLExecError_t &) = delete;
	tSQLExecError_t &operator =(const tSQLExecError_t &) = delete;
};

struct tSQLResult_t final
{
private:
	using fieldType_t = std::pair<int16_t, uint32_t>;
	const tSQLClient_t *const client;
	void *const handle;

protected:
	tSQLResult_t(const tSQLClient_t *const client, void *const &&handle, const bool hasData, const bool freeHandle = true) noexcept;
	bool error(const int16_t err) const noexcept;
	friend struct tSQLQuery_t;

public:
	constexpr tSQLResult_t() noexcept : client(nullptr), handle(nullptr) { }
	tSQLResult_t(tSQLResult_t &&row) noexcept : tSQLResult_t() { *this = std::move(row); }
	~tSQLResult_t();
	tSQLResult_t &operator =(tSQLResult_t &&row) noexcept;

	tSQLResult_t(const tSQLResult_t &) = delete;
	tSQLResult_t &operator =(const tSQLResult_t &) = delete;
};

struct tSQLQuery_t final
{
private:
	const tSQLClient_t *const client;
	void *const handle;

protected:
	tSQLQuery_t(const tSQLClient_t *const client, void *const handle, const char *const queryStmt, const size_t paramsCount) noexcept;
	bool error(const int16_t err) const noexcept;
	friend struct tSQLClient_t;

public:
	constexpr tSQLQuery_t() noexcept : client(nullptr), handle(nullptr) { }
	tSQLQuery_t(tSQLQuery_t &&qry) noexcept : tSQLQuery_t() { *this = std::move(qry); }
	~tSQLQuery_t();
	tSQLQuery_t &operator =(tSQLQuery_t &&qry) noexcept;

	tSQLQuery_t(const tSQLQuery_t &) = delete;
	tSQLQuery_t &operator =(const tSQLQuery_t &) = delete;
};

struct tSQLClient_t final
{
private:
	void *dbHandle;
	void *connection;
	mutable bool haveConnection, needsCommit;
	mutable tSQLExecError_t _error;

	bool connect(const stringPtr_t &connString) const noexcept;

protected:
	bool error(const int16_t err, const int16_t handleType, void *const handle) const noexcept;
	bool error(const tSQLExecErrorType_t err, const int16_t handleType, void *const handle) const noexcept;
	bool error(const tSQLExecErrorType_t err) const noexcept;
	friend struct tSQLResult_t;
	friend struct tSQLQuery_t;

public:
	tSQLClient_t() noexcept;
	tSQLClient_t(tSQLClient_t &&con) noexcept : dbHandle(nullptr), connection(nullptr), haveConnection(false),
		needsCommit(false), _error() { *this = std::move(con); }
	~tSQLClient_t() noexcept;
	tSQLClient_t &operator =(tSQLClient_t &&) noexcept;

	bool valid() const noexcept { return dbHandle && connection && haveConnection; }
	bool connect(const char *const driver, const char *const host, const uint32_t port, const char *const user, const char *const passwd) const noexcept;
	void disconnect() const noexcept;
	bool selectDB(const char *const db) const noexcept;
	bool beginTransact() const noexcept;
	bool endTransact(const bool commitSuccess) const noexcept;
	bool commit() const noexcept { return endTransact(true); }
	bool rollback() const noexcept { return endTransact(false); }
	tSQLResult_t query(const char *const queryStmt) const noexcept;
	tSQLQuery_t prepare(const char *const queryStmt, const size_t paramsCount) const noexcept;
	const tSQLExecError_t &error() const noexcept { return _error; }

	tSQLClient_t(const tSQLClient_t &) = delete;
	tSQLClient_t &operator =(const tSQLClient_t &) = delete;
};
		}
	}
}

#endif /*MSSQL__HXX*/
