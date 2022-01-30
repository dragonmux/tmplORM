#ifndef PGSQL_HXX
#define PGSQL_HXX

#include <cstdint>
#include <libpq-fe.h>
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
struct tmplORM_API pgSQLResult_t final
{
private:
	PGresult *result{nullptr};

public:
	/*! @brief Default constructor for result objects, constructing invalid result objects by default */
	constexpr pgSQLResult_t() noexcept = default;
	pgSQLResult_t(pgSQLResult_t &&res) noexcept;
	~pgSQLResult_t() noexcept;
	pgSQLResult_t &operator =(pgSQLResult_t &&res) noexcept;
	/*!
	 * @brief Call to determine if this result object is valid
	 * @returns true if the object is valid, false otherwise
	 */
	bool valid() const noexcept { return result; }
	uint32_t numRows() const noexcept;

	/*! @brief Deleted copy constructor for pgSQLResult_t as results are not copyable */
	pgSQLResult_t(const pgSQLResult_t &) = delete;
	/*! @brief Deleted copy assignment operator for pgSQLResult_t as results are not copyable */
	pgSQLResult_t &operator =(const pgSQLResult_t &) = delete;
};

struct tmplORM_API pgSQLClient_t final
{
private:
	PGconn *connection{nullptr};

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
};
		} // namespace driver
	} // namespace pgsql
} // namespace tmplORM

#endif /*PGSQL_HXX*/
