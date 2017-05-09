#ifndef tmplORM_MYSQL__HXX
#define tmplORM_MYSQL__HXX

#include "tmplORM.hxx"
#include <type_traits>
#include <memory>
#include "mysql.hxx"
#include "managedPtr.hxx"

namespace tmplORM
{
	namespace mysql
	{
		using namespace tmplORM::common;
		using namespace tmplORM::mysql::driver;
		using namespace tmplORM::types::baseTypes;

		using tmplORM::types::type_t;
		using tmplORM::types::unicode_t;
		using tmplORM::types::unicodeText_t;

		using tmplORM::types::primary_t;
		using tmplORM::types::autoInc_t;
		using tmplORM::types::nullable_t;

		// If we don't know how to translate the type, don't.
		template<typename> struct stringType_t { using value = typestring<>; };
		template<> struct stringType_t<int8_t> { using value = ts("TINYINT"); };
		template<> struct stringType_t<uint8_t> { using value = ts("TINYINT UNSIGNED"); };
		template<> struct stringType_t<int16_t> { using value = ts("SHORT"); };
		template<> struct stringType_t<uint16_t> { using value = ts("SHORT UNSIGNED"); };
		template<> struct stringType_t<int32_t> { using value = ts("INT"); };
		template<> struct stringType_t<uint32_t> { using value = ts("INT UNSIGNED"); };
		template<> struct stringType_t<int64_t> { using value = ts("BIGINT"); };
		template<> struct stringType_t<uint64_t> { using value = ts("BIGINT UNSIGNED"); };
		template<> struct stringType_t<bool> { using value = ts("BIT(1)"); };
		template<> struct stringType_t<float> { using value = ts("FLOAT"); };
		template<> struct stringType_t<double> { using value = ts("DOUBLE"); };
		template<> struct stringType_t<char *> { using value = ts("TEXT"); };
		template<> struct stringType_t<void *> { using value = ts("BLOB"); };
		template<> struct stringType_t<ormDate_t> { using value = ts("DATE"); };
		template<> struct stringType_t<ormDateTime_t> { using value = ts("DATETIME"); };
		//template<> struct stringType_t<ormUUID_t> { using value = ts("VARCHAR(36)"); };
		template<typename T> using stringType = typename stringType_t<T>::value;

		template<typename> struct bind_t { };
		template<> struct bind_t<int8_t> { constexpr static const mySQLFieldType_t value = MYSQL_TYPE_TINY; };
		template<> struct bind_t<int16_t> { constexpr static const mySQLFieldType_t value = MYSQL_TYPE_SHORT; };
		template<> struct bind_t<int32_t> { constexpr static const mySQLFieldType_t value = MYSQL_TYPE_LONG; };
		template<> struct bind_t<int64_t> { constexpr static const mySQLFieldType_t value = MYSQL_TYPE_LONGLONG; };
		//
		template<> struct bind_t<bool> { constexpr static const mySQLFieldType_t value = MYSQL_TYPE_BIT; };
		template<> struct bind_t<float> { constexpr static const mySQLFieldType_t value = MYSQL_TYPE_FLOAT; };
		template<> struct bind_t<double> { constexpr static const mySQLFieldType_t value = MYSQL_TYPE_DOUBLE; };
		template<> struct bind_t<char *> { constexpr static const mySQLFieldType_t value = MYSQL_TYPE_STRING; };
		template<> struct bind_t<const char *> { constexpr static const mySQLFieldType_t value = MYSQL_TYPE_STRING; };
		template<> struct bind_t<void *> { constexpr static const mySQLFieldType_t value = MYSQL_TYPE_BLOB; };
		template<> struct bind_t<ormDate_t> { constexpr static const mySQLFieldType_t value = MYSQL_TYPE_DATE; };
		template<> struct bind_t<ormDateTime_t> { constexpr static const mySQLFieldType_t value = MYSQL_TYPE_DATETIME; };
		//template<> struct bind_t<ormUUID_t> { constexpr static const mySQLFieldType_t value = MYSQL_TYPE_VARCHAR; };
		template<> struct bind_t<nullptr_t> { constexpr static const mySQLFieldType_t value = MYSQL_TYPE_NULL; };
		template<typename T, bool = isNumeric<T>::value> struct bindType_t : public bind_t<T> { };
		template<typename T> struct bindType_t<T, true> : public bind_t<typename std::make_signed<T>::type> { };

		namespace driver
		{
			using namespace tmplORM::types::baseTypes;

			static const char nullParam = char(true);
			constexpr static char *const notNullParam = nullptr;

			template<typename T> typename std::enable_if<!isNumeric<T>::value>::type
				bindT(MYSQL_BIND &param) noexcept { param.is_unsigned = false; }
			template<typename T> typename std::enable_if<isNumeric<T>::value>::type
				bindT(MYSQL_BIND &param) noexcept { param.is_unsigned = std::is_unsigned<T>::value; }

			template<bool> struct bindValue_t
			{
				using nanoseconds_t = std::chrono::nanoseconds;

				template<typename T> bool operator ()(MYSQL_BIND &param, const T &value, managedPtr_t<void> &) noexcept
					{ param.buffer = const_cast<T *>(&value); return true; }

				bool operator ()(MYSQL_BIND &param, const ormDate_t &value, managedPtr_t<void> &paramStorage) noexcept
				{
					MYSQL_TIME date;
					date.year = value.year();
					date.month = value.month();
					date.day = value.day();
					date.time_type = MYSQL_TIMESTAMP_DATE;

					paramStorage = makeManaged<MYSQL_TIME>(date);
					if (!paramStorage)
						return false;
					param.buffer = paramStorage;
					param.buffer_length = sizeof(date);
					return true;
				}

				bool operator ()(MYSQL_BIND &param, const ormDateTime_t &value, managedPtr_t<void> &paramStorage) noexcept
				{
					MYSQL_TIME dateTime;
					dateTime.year = value.year();
					dateTime.month = value.month();
					dateTime.day = value.day();
					dateTime.hour = value.hour();
					dateTime.minute = value.minute();
					dateTime.second = value.second();
					dateTime.second_part = value.nanoSecond() / std::chrono::duration_cast<nanoseconds_t>(1_us).count();
					dateTime.time_type = MYSQL_TIMESTAMP_DATETIME;

					paramStorage = makeManaged<MYSQL_TIME>(dateTime);
					if (!paramStorage)
						return false;
					param.buffer = paramStorage;
					param.buffer_length = sizeof(dateTime);
					return true;
				}
			};

			template<> struct bindValue_t<true>
			{
				template<typename T> bool operator ()(MYSQL_BIND &param, const T *const value, managedPtr_t<void> &) noexcept
					{ param.buffer = const_cast<T *>(value); return true; }
			};

			template<typename T> using bindValue_ = bindValue_t<std::is_pointer<T>::value>;

			template<typename T> void mySQLPreparedQuery_t::bind(const size_t index, const T &value, const fieldLength_t length) noexcept
			{
				if (index >= numParams)
					return;
				MYSQL_BIND &param = params[index];
				param.buffer_type = bindType_t<T>::value;
				param.buffer_length = length.first;
				if (!bindValue_<T>()(param, value, paramStorage[index]))
					return;
				param.length = &param.buffer_length;
				param.is_null = notNullParam;
				bindT<T>(param);
			}

			template<typename T> void mySQLPreparedQuery_t::bind(const size_t index, const nullptr_t, const fieldLength_t) noexcept
			{
				if (index >= numParams)
					return;
				MYSQL_BIND &param = params[index];
				param.buffer_type = bindType_t<T>::value;
				param.is_null = const_cast<char *>(&nullParam);
			}
		}

		template<typename name> using backtick = tycat<ts("`"), name, ts("`")>;

		// Formatting type for handling field names (to make lists from them)
		template<size_t, typename> struct fieldName_t { };
		template<size_t N, typename fieldName, typename T> struct fieldName_t<N, type_t<fieldName, T>>
			{ using value = tycat<backtick<fieldName>, comma<N>>; };

#include "tmplORM.common.hxx"

		template<typename> struct createName_t { };
		template<typename fieldName, typename T> struct createName_t<type_t<fieldName, T>>
			{ using value = tycat<backtick<fieldName>, ts(" "), stringType<T>>; };
		template<typename fieldName, size_t length> struct createName_t<unicode_t<fieldName, length>>
			{ using value = tycat<backtick<fieldName>, ts(" VARCHAR("), toTypestring<length>, ts(")")>; };

		template<size_t N, typename field> struct createList__t
		{
			template<typename fieldName, typename T> constexpr static auto _name(const type_t<fieldName, T> &) ->
				typename createName_t<type_t<fieldName, T>>::value;
			template<typename fieldName, size_t length> constexpr static auto _name(const unicode_t<fieldName, length> &) ->
				typename createName_t<unicode_t<fieldName, length>>::value;
			template<typename T> constexpr static auto _name(const autoInc_t<T> &) -> tycat<decltype(_name(T())), ts(" AUTO_INCREMENT")>;
			template<typename T> constexpr static auto _name(const primary_t<T> &) -> tycat<decltype(_name(T())), ts(" PRIMARY KEY")>;
			using name = decltype(_name(field()));

			constexpr static auto value() -> tycat<name, nullable<field::nullable>, comma<N>>;
		};
		// Alias for the above container type to make it easier to use
		template<size_t N, typename T> using createList__ = decltype(createList__t<N, T>::value());

		template<size_t N, typename field, typename... fields> struct createList_t
			{ using value = tycat<createList__<N, field>, typename createList_t<N - 1, fields...>::value>; };
		template<typename field> struct createList_t<1, field> { using value = createList__<1, field>; };
		// Alias to make the above easier to use
		template<typename... fields> using createList = typename createList_t<sizeof...(fields), fields...>::value;

		template<typename tableName, typename... fields> using createTable_ = toString<
			tycat<ts("CREATE TABLE IF NOT EXISTS "), backtick<tableName>, ts(" ("), createList<fields...>, ts(");")>
		>;
		template<typename tableName, typename... fields> using select_ = toString<
			tycat<ts("SELECT "), selectList<fields...>, ts(" FROM "), backtick<tableName>, ts(";")>
		>;
		// tycat<> builds up the query string for inserting the data
		template<typename tableName, typename... fields> using add_ = toString<
			tycat<ts("INSERT INTO "), backtick<tableName>, ts(" ("), insertList<fields...>, ts(") VALUES ("), placeholder<countInsert_t<fields...>::count>, ts(");")>
		>;
		// This constructs invalid if there is no field marked primary_t<>! This is quite intentional.
		template<typename tableName, typename... fields> struct update_t<false, tableName, fields...>
			{ using value = tycat<ts("UPDATE "), backtick<tableName>, ts(" SET "), updateList<fields...>, updateWhere<fields...>, ts(";")>; };
		template<typename tableName, typename... fields> using del_ = toString<
			tycat<ts("DELETE FROM "), backtick<tableName>, updateWhere<fields...>, ts(";")>
		>;
		template<typename tableName> using deleteTable_ = toString<
			tycat<ts("DROP TABLE IF NOT EXISTS "), backtick<tableName>, ts(";")>
		>;

		struct session_t final
		{
		private:
			driver::mySQLClient_t database;

		public:
			session_t() noexcept : database() { }
			~session_t() noexcept { }

			template<typename tableName, typename... fields> bool createTable(const model_t<tableName, fields...> &) noexcept
			{
				using create = createTable_<tableName, fields...>;
				return database.query(create::value);
			}

			template<typename T, typename tableName, typename... fields_t> fixedVector_t<T> select(const model_t<tableName, fields_t...> &)
			{
				fixedVector_t<T> data;
				using select = select_<tableName, fields_t...>;
				if (!database.query(select::data))
					throw mySQLValueError_t(mySQLErrorType_t::queryError);
				mySQLResult_t result = database.queryResult();
				if (!result.valid())
					throw mySQLValueError_t(mySQLErrorType_t::queryError);
				mySQLRow_t row = result.resultRows();
				for (size_t i = 0; i < result.numRows(); ++i, row.next())
				{
					T value;
					if (!row.valid())
						return {};
					bindSelect<fields_t...>::bind(value.fields(), row);
					data[i] = std::move(value);
				}
				if (row.valid())
					return {};
				return data;
			}

			template<typename tableName, typename... fields_t> bool add(model_t<tableName, fields_t...> &model)
			{
				using add = add_<tableName, fields_t...>;
				mySQLPreparedQuery_t query(database.prepare(add::value, countInsert_t<fields_t...>::count));
				bindInsert<fields_t...>::bind(model.fields(), query);
				if (query.execute())
				{
					setAutoInc_t<hasAutoInc<fields_t...>()>::set(model, query.rowID());
					return true;
				}
				return false;
			}

			template<typename tableName, typename... fields_t> bool update(model_t<tableName, fields_t...> &model)
			{
				using update = update_<tableName, fields_t...>;
				if (std::is_same<update, toString<typestring<>>>::value)
					return false;
				mySQLPreparedQuery_t query(database.prepare(update::value, sizeof...(fields_t)));
				// This binds the fields, primary key last so it tags to the WHERE clause for the query.
				bindUpdate<fields_t...>::bind(model.fields(), query);
				// This either works or doesn't.. thankfully.. so, we can just execute-and-quit.
				return query.execute();
			}

			template<typename tableName, typename... fields_t> bool del(const model_t<tableName, fields_t...> &model) noexcept
			{
				using del = del_<tableName, fields_t...>;
				mySQLPreparedQuery_t query(database.prepare(del::value, countPrimary<fields_t...>::count));
				// This binds just the primary keys of the model so it tags in-order to the WHERE clause for this query.
				bindDelete<fields_t...>::bind(model.fields(), query);
				return query.execute();
			}

			template<typename tableName, typename... fields> bool deleteTable(const model_t<tableName, fields...> &) noexcept
			{
				using deleteTable = deleteTable_<tableName>;
				return database.query(deleteTable::value);
			}

			bool connect(const char *const host, const uint32_t port, const char *const user, const char *const passwd) const noexcept
				{ return database.connect(host, port, user, passwd); }
			bool connect(const char *const unixSocket, const char *const user, const char *const passwd) const noexcept
				{ return database.connect(unixSocket, user, passwd); }
			void disconnect() noexcept { database.disconnect(); }
			bool selectDB(const char *const db) const noexcept { return database.selectDB(db); }
		};
	}
	using mysql_t = mysql::session_t;
}

#endif /*tmplORM_MYSQL__HXX*/
