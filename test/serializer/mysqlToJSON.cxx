#include <tmplORM.mysql.hxx>
#include "toJSON.hxx"
#include "../constString.hxx"

constString_t driver, host, username, password, db;
constexpr static uint32_t port = 1433;

bool haveEnvironment() noexcept
{
	host = getenv("MYSQL_HOST");
	// port?
	username = getenv("MYSQL_USERNAME");
	password = getenv("MYSQL_PASSWORD");
	db = getenv("MYSQL_DATABASE");
	return !(host.empty() || username.empty() || password.empty() || db.empty());
}

int main(int, char **)
{
	if (!haveEnvironment())
	{
		puts("No suitable environment found, refusing to run");
		return 1;
	}
	tmplORM::session_t<tmplORM::mysql_t> session{};
	session.inner().connect(host, port, username, password);
	session.inner().selectDB(db);
	writeDataTo("data.json", session);
	session.inner().disconnect();

	return 0;
}
