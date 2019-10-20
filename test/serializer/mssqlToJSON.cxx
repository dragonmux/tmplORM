#include <tmplORM.mssql.hxx>
#include "toJSON.hxx"
#include "../constString.hxx"

constString_t driver, host, username, password, db;
constexpr static uint32_t port = 1433;

bool haveEnvironment() noexcept
{
	driver = getenv("MSSQL_DRIVER");
	host = getenv("MSSQL_HOST");
	// port?
	username = getenv("MSSQL_USERNAME");
	password = getenv("MSSQL_PASSWORD");
	db = getenv("MSSQL_DATABASE");
	return !(driver.empty() || host.empty() || username.empty() || password.empty() || db.empty());
}

int main(int, char **)
{
	if (!haveEnvironment())
	{
		puts("No suitable environment found, refusing to run");
		return 1;
	}
	tmplORM::session_t<tmplORM::mssql_t> session{};
	session.inner().connect(driver, host, port, username, password);
	session.inner().selectDB(db);
	writeDataTo("data.json", session);
	session.inner().disconnect();

	return 0;
}
