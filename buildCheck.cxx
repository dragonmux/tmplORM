#include <iostream>
#ifndef _MSC_VER
#include <unistd.h>
#include <sys/ioctl.h>
#else
#include <io.h>
#include <sys/locking.h>
#endif

#include "tmplORM.mysql.hxx"
#include "tmplORM.mssql.hxx"
#include "tmplORM.pgsql.hxx"

using std::cout;
using std::endl;

#ifndef _MSC_VER
#define COLOUR(Code) "\x1B[" Code "m"
#define NORMAL COLOUR("0;39")
#define SUCCESS COLOUR("1;32")
#define FAILURE COLOUR("1;31")
#define BRACKET COLOUR("1;34")
#define INFO COLOUR("1;36")

#define SET_COL(x) "\x1B[" << (x) << "G"
#define NEWLINE NORMAL "\n"

uint16_t getColumns()
{
	struct winsize win{};
	ioctl(STDIN_FILENO, TIOCGWINSZ, &win);
	return win.ws_col == 0 ? 80 : win.ws_col;
}

#define COL ((getColumns() >> 1U) - 4)

void echoPass() noexcept { cout << SET_COL(COL) BRACKET "[" SUCCESS "  OK  " BRACKET "]" NEWLINE; }
void echoFail() noexcept { cout << SET_COL(COL) BRACKET "[" FAILURE " FAIL " BRACKET "]" NEWLINE; }
#else
uint16_t getColumns()
{
	CONSOLE_SCREEN_BUFFER_INFO window{};
	GetConsoleScreenBufferInfo(console, &window);
	return window.dwSize.X;
}

#define COL (getColumns() >> 1) - 5

void echoPass()
{
	CONSOLE_SCREEN_BUFFER_INFO cursor{};
	GetConsoleScreenBufferInfo(console, &cursor);
	cursor.dwCursorPosition.Y--;
	cursor.dwCursorPosition.X = COL;
	SetConsoleCursorPosition(console, cursor.dwCursorPosition);
	SetConsoleTextAttribute(console, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	cout << "[";
	SetConsoleTextAttribute(console, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	cout << "  OK  ";
	SetConsoleTextAttribute(console, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	cout << "]";
	SetConsoleTextAttribute(console, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	cout << endl;
}

void echoFail()
{
	CONSOLE_SCREEN_BUFFER_INFO cursor{};
	GetConsoleScreenBufferInfo(console, &cursor);
	cursor.dwCursorPosition.Y--;
	cursor.dwCursorPosition.X = COL;
	SetConsoleCursorPosition(console, cursor.dwCursorPosition);
	SetConsoleTextAttribute(console, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	cout << "[";
	SetConsoleTextAttribute(console, FOREGROUND_RED | FOREGROUND_INTENSITY);
	cout << " FAIL ";
	SetConsoleTextAttribute(console, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	cout << "]";
	SetConsoleTextAttribute(console, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	cout << endl;
}
#endif

using tmplORM::model_t;
using namespace tmplORM::common;

namespace models
{
	using namespace tmplORM;

	struct user_t : public model_t<ts("Users"),
		types::autoInc_t<types::primary_t<types::int32_t<ts("UserID")>>>, types::int32_t<ts("UserForumID")>,
		types::unicode_t<ts("UserLogin"), 50>,
		// Other fields and stuff..
		types::bool_t<ts("UserActive")>, types::bool_t<ts("UserIsMinor")>,
		types::int32_t<ts("UserBadgeNo")>,
		types::bool_t<ts("UserMagstripeExpired")>
	> { };

	struct userTimeLog_t : public model_t<ts("UserTimeLogs"),
		types::autoInc_t<types::primary_t<types::int32_t<ts("UserTimeLogID")>>>, types::int32_t<ts("UserID")>,
		// More fields and stuff..
		types::int32_t<ts("UserTimeLogNotes")>
	> { };

	struct multiPrimary_t : public model_t<ts("MultiPrimary"),
		types::primary_t<types::int64_t<ts("Key1")>>, types::primary_t<types::int64_t<ts("Key2")>>
	> { };
} // namespace models

using models::user_t;
using models::userTimeLog_t;
using models::multiPrimary_t;

user_t user;
userTimeLog_t timeLog;
multiPrimary_t multiPrimary;

namespace mysql
{
	template<typename tableName, typename... fields> using createTable__ = tmplORM::mysql::createTable_<tableName, fields...>;
	template<typename tableName, typename... fields> using select__ = tmplORM::mysql::select_<tableName, fields...>;
	template<typename tableName, typename... fields> using add__ = tmplORM::mysql::add_<tableName, fields...>;
	template<typename tableName, typename... fields> using addAll__ = tmplORM::mysql::addAll_<tableName, fields...>;
	template<typename tableName, typename... fields> using update__ = tmplORM::mysql::update_<tableName, fields...>;
	template<typename tableName, typename... fields>  using del__ = tmplORM::mysql::del_<tableName, fields...>;
	template<typename tableName> using deleteTable__ = tmplORM::mysql::deleteTable_<tableName>;

	template<typename tableName, typename... fields> bool createTable_(const model_t<tableName, fields...> &) noexcept
	{
		using create = createTable__<tableName, fields...>;
		cout << create::value << "\n";
		return true;
	}
	template<typename... models> bool createTable() noexcept { return collect(createTable_(models())...); }

	template<typename tableName, typename... fields> bool select_(const model_t<tableName, fields...> &) noexcept
	{
		using select = select__<tableName, fields...>;
		cout << select::value << "\n";
		return true;
	}
	template<typename... models> bool select() noexcept { return collect(select_(models())...); }

	template<typename tableName, typename... fields> bool add_(const model_t<tableName, fields...> &) noexcept
	{
		using insert = add__<tableName, fields...>;
		cout << insert::value << "\n";
		return true;
	}
	template<typename... models_t> bool add(const models_t &...models) noexcept { return collect(add_(models)...); }

	template<typename tableName, typename... fields> bool addAll_(const model_t<tableName, fields...> &) noexcept
	{
		using insert = addAll__<tableName, fields...>;
		cout << insert::value << "\n";
		return true;
	}
	template<typename... models_t> bool addAll(const models_t &...models) noexcept { return collect(addAll_(models)...); }

	template<typename tableName, typename... fields> bool update_(const model_t<tableName, fields...> &) noexcept
	{
		using update = update__<tableName, fields...>;
		cout << update::value << "\n";
		return true;
	}
	template<typename... models_t> bool update(const models_t &...models) noexcept { return collect(update_(models)...); }

	template<typename tableName, typename... fields> bool del_(const model_t<tableName, fields...> &) noexcept
	{
		using del = del__<tableName, fields...>;
		cout << del::value << "\n";
		return true;
	}
	template<typename... models_t> bool del(const models_t &...models) noexcept { return collect(del_(models)...); }

	template<typename tableName, typename... fields> bool deleteTable_(const model_t<tableName, fields...> &) noexcept
	{
		using deleteTable = deleteTable__<tableName>;
		cout << deleteTable::value << "\n";
		return true;
	}
	template<typename... models> bool deleteTable() noexcept { return collect(deleteTable_(models())...); }

	void test() noexcept
	{
		createTable<user_t, userTimeLog_t>() ? echoPass() : echoFail();
		select<user_t, userTimeLog_t>() ? echoPass() : echoFail();
		add(user, timeLog) ? echoPass() : echoFail();
		addAll(user, timeLog) ? echoPass() : echoFail();
		update(user, timeLog) ? echoPass() : echoFail();
		del(user, timeLog, multiPrimary) ? echoPass() : echoFail();
		deleteTable<user_t, userTimeLog_t>() ? echoPass() : echoFail();
	}
} // namespace mysql

namespace mssql
{
	template<typename tableName, typename... fields> using createTable__ = tmplORM::mssql::createTable_<tableName, fields...>;
	template<typename tableName, typename... fields> using select__ = tmplORM::mssql::select_<tableName, fields...>;
	template<typename tableName, typename... fields> using add__ = tmplORM::mssql::add_<tableName, fields...>;
	template<typename tableName, typename... fields> using addAll__ = tmplORM::mssql::addAll_<tableName, fields...>;
	template<typename tableName, typename... fields> using update__ = tmplORM::mssql::update_<tableName, fields...>;
	template<typename tableName, typename... fields>  using del__ = tmplORM::mssql::del_<tableName, fields...>;
	template<typename tableName> using deleteTable__ = tmplORM::mssql::deleteTable_<tableName>;

	template<typename tableName, typename... fields> bool createTable_(const model_t<tableName, fields...> &) noexcept
	{
		using create = createTable__<tableName, fields...>;
		cout << create::value << "\n";
		return true;
	}
	template<typename... models> bool createTable() noexcept { return collect(createTable_(models())...); }

	template<typename tableName, typename... fields> bool select_(const model_t<tableName, fields...> &) noexcept
	{
		using select = select__<tableName, fields...>;
		cout << select::value << "\n";
		return true;
	}
	template<typename... models> bool select() noexcept { return collect(select_(models())...); }

	template<typename tableName, typename... fields> bool add_(const model_t<tableName, fields...> &) noexcept
	{
		using insert = add__<tableName, fields...>;
		cout << insert::value << "\n";
		return true;
	}
	template<typename... models_t> bool add(const models_t &...models) noexcept { return collect(add_(models)...); }

	template<typename tableName, typename... fields> bool addAll_(const model_t<tableName, fields...> &) noexcept
	{
		using insert = addAll__<tableName, fields...>;
		cout << insert::value << "\n";
		return true;
	}
	template<typename... models_t> bool addAll(const models_t &...models) noexcept { return collect(addAll_(models)...); }

	template<typename tableName, typename... fields> bool update_(const model_t<tableName, fields...> &) noexcept
	{
		using update = update__<tableName, fields...>;
		cout << update::value << "\n";
		return true;
	}
	template<typename... models_t> bool update(const models_t &...models) noexcept { return collect(update_(models)...); }

	template<typename tableName, typename... fields> bool del_(const model_t<tableName, fields...> &) noexcept
	{
		using del = del__<tableName, fields...>;
		cout << del::value << "\n";
		return true;
	}
	template<typename... models_t> bool del(const models_t &...models) noexcept { return collect(del_(models)...); }

	template<typename tableName, typename... fields> bool deleteTable_(const model_t<tableName, fields...> &) noexcept
	{
		using deleteTable = deleteTable__<tableName>;
		cout << deleteTable::value << "\n";
		return true;
	}
	template<typename... models> bool deleteTable() noexcept { return collect(deleteTable_(models())...); }

	void test() noexcept
	{
		createTable<user_t, userTimeLog_t>() ? echoPass() : echoFail();
		select<user_t, userTimeLog_t>() ? echoPass() : echoFail();
		add(user, timeLog) ? echoPass() : echoFail();
		addAll(user, timeLog) ? echoPass() : echoFail();
		update(user, timeLog) ? echoPass() : echoFail();
		del(user, timeLog, multiPrimary) ? echoPass() : echoFail();
		deleteTable<user_t, userTimeLog_t>() ? echoPass() : echoFail();
	}
} // namespace mssql

namespace pgsql
{
	template<typename tableName, typename... fields> using createTable__ = tmplORM::pgsql::createTable_<tableName, fields...>;
	template<typename tableName, typename... fields> using select__ = tmplORM::pgsql::select_<tableName, fields...>;
	template<typename tableName, typename... fields> using add__ = tmplORM::pgsql::add_<tableName, fields...>;
	template<typename tableName, typename... fields> using addAll__ = tmplORM::pgsql::addAll_<tableName, fields...>;
	template<typename tableName, typename... fields> using update__ = tmplORM::pgsql::update_<tableName, fields...>;
	template<typename tableName, typename... fields>  using del__ = tmplORM::pgsql::del_<tableName, fields...>;
	template<typename tableName> using deleteTable__ = tmplORM::pgsql::deleteTable_<tableName>;

	template<typename tableName, typename... fields> bool createTable_(const model_t<tableName, fields...> &) noexcept
	{
		using create = createTable__<tableName, fields...>;
		cout << create::value << "\n";
		return true;
	}
	template<typename... models> bool createTable() noexcept { return collect(createTable_(models())...); }

	template<typename tableName, typename... fields> bool select_(const model_t<tableName, fields...> &) noexcept
	{
		using select = select__<tableName, fields...>;
		cout << select::value << "\n";
		return true;
	}
	template<typename... models> bool select() noexcept { return collect(select_(models())...); }

	template<typename tableName, typename... fields> bool add_(const model_t<tableName, fields...> &) noexcept
	{
		using insert = add__<tableName, fields...>;
		cout << insert::value << "\n";
		return true;
	}
	template<typename... models_t> bool add(const models_t &...models) noexcept { return collect(add_(models)...); }

	template<typename tableName, typename... fields> bool addAll_(const model_t<tableName, fields...> &) noexcept
	{
		using insert = addAll__<tableName, fields...>;
		cout << insert::value << "\n";
		return true;
	}
	template<typename... models_t> bool addAll(const models_t &...models) noexcept { return collect(addAll_(models)...); }

	template<typename tableName, typename... fields> bool update_(const model_t<tableName, fields...> &) noexcept
	{
		using update = update__<tableName, fields...>;
		cout << update::value << "\n";
		return true;
	}
	template<typename... models_t> bool update(const models_t &...models) noexcept { return collect(update_(models)...); }

	template<typename tableName, typename... fields> bool del_(const model_t<tableName, fields...> &) noexcept
	{
		using del = del__<tableName, fields...>;
		cout << del::value << "\n";
		return true;
	}
	template<typename... models_t> bool del(const models_t &...models) noexcept { return collect(del_(models)...); }

	template<typename tableName, typename... fields> bool deleteTable_(const model_t<tableName, fields...> &) noexcept
	{
		using deleteTable = deleteTable__<tableName>;
		cout << deleteTable::value << "\n";
		return true;
	}
	template<typename... models> bool deleteTable() noexcept { return collect(deleteTable_(models())...); }

	void test() noexcept
	{
		createTable<user_t, userTimeLog_t>() ? echoPass() : echoFail();
		select<user_t, userTimeLog_t>() ? echoPass() : echoFail();
		add(user, timeLog) ? echoPass() : echoFail();
		addAll(user, timeLog) ? echoPass() : echoFail();
		update(user, timeLog) ? echoPass() : echoFail();
		del(user, timeLog, multiPrimary) ? echoPass() : echoFail();
		deleteTable<user_t, userTimeLog_t>() ? echoPass() : echoFail();
	}
} // namespace pgsql

template<typename fieldName, typename T> const char *fieldName_(const type_t<fieldName, T> &) noexcept { return fieldName::data(); }

int main(int, char **) noexcept
{
	cout << "Model user_t represents " << user_t::tableName() << " with " << user_t::N << " fields\n";
	cout << "Model userTimeLog_t represents " << userTimeLog_t::tableName() << " with " << userTimeLog_t::N << " fields\n";
	echoPass();

	cout << INFO "MySQL tests:" NEWLINE;
	mysql::test();

	cout << INFO "MSSQL (Transact-SQL) tests:" NEWLINE;
	mssql::test();

	cout << INFO "PGSQL tests:" NEWLINE;
	pgsql::test();

	cout << INFO "General test:" NEWLINE;
	cout << std::boolalpha;
	cout << "UserID field: " << fieldName_(user[ts_("UserID")]) << " (" << typeid(user[ts_("UserID")]).name() << ")\n";
	cout << "UserBadgeNo field: " << fieldName_(user[ts_("UserBadgeNo")]) << " (" << typeid(user[ts_("UserBadgeNo")]).name() << ")\n";
	echoPass();

	return 0;
}
