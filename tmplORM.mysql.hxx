#ifndef tmplORM_MYSQL_HXX
#define tmplORM_MYSQL_HXX

#include "tmplORM.hxx"
#include <type_traits>
#include <memory>
#include <mysql.hxx>

namespace tmplORM
{
	namespace mysql
	{
		using substrate::managedPtr_t;
		using namespace tmplORM::common;
		using namespace tmplORM::mysql::driver;
		using namespace tmplORM::types::baseTypes;
		using tmplORM::utils::isNumeric;

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
		template<> struct stringType_t<int16_t> { using value = ts("SMALLINT"); };
		template<> struct stringType_t<uint16_t> { using value = ts("SMALLINT UNSIGNED"); };
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
		template<> struct stringType_t<ormUUID_t> { using value = ts("CHAR(32)"); };
		template<typename T> using stringType = typename stringType_t<T>::value;

		template<typename> struct bind_t { };
		template<> struct bind_t<int8_t> { constexpr static const mySQLFieldType_t value = MYSQL_TYPE_TINY; };
		template<> struct bind_t<int16_t> { constexpr static const mySQLFieldType_t value = MYSQL_TYPE_SHORT; };
		template<> struct bind_t<int32_t> { constexpr static const mySQLFieldType_t value = MYSQL_TYPE_LONG; };
		template<> struct bind_t<int64_t> { constexpr static const mySQLFieldType_t value = MYSQL_TYPE_LONGLONG; };
		//
		template<> struct bind_t<bool> { constexpr static const mySQLFieldType_t value = MYSQL_TYPE_TINY; };
		template<> struct bind_t<float> { constexpr static const mySQLFieldType_t value = MYSQL_TYPE_FLOAT; };
		template<> struct bind_t<double> { constexpr static const mySQLFieldType_t value = MYSQL_TYPE_DOUBLE; };
		template<> struct bind_t<char *> { constexpr static const mySQLFieldType_t value = MYSQL_TYPE_VAR_STRING; };
		template<> struct bind_t<const char *> { constexpr static const mySQLFieldType_t value = MYSQL_TYPE_VAR_STRING; };
		template<> struct bind_t<void *> { constexpr static const mySQLFieldType_t value = MYSQL_TYPE_BLOB; };
		template<> struct bind_t<ormDate_t> { constexpr static const mySQLFieldType_t value = MYSQL_TYPE_DATE; };
		template<> struct bind_t<ormDateTime_t> { constexpr static const mySQLFieldType_t value = MYSQL_TYPE_DATETIME; };
		template<> struct bind_t<ormUUID_t> { constexpr static const mySQLFieldType_t value = MYSQL_TYPE_STRING; };
		template<> struct bind_t<nullptr_t> { constexpr static const mySQLFieldType_t value = MYSQL_TYPE_NULL; };
		template<typename T, bool = isNumeric<T>::value> struct bindType_t : public bind_t<T> { };
		template<typename T> struct bindType_t<T, true> : public bind_t<typename std::make_signed<T>::type> { };

		namespace driver
		{
			using namespace tmplORM::types::baseTypes;
			constexpr auto notNullParam = nullptr;

			template<typename T> typename std::enable_if<!isNumeric<T>::value>::type
				bindT(MYSQL_BIND &param) noexcept { param.is_unsigned = false; }
			template<typename T> typename std::enable_if<isNumeric<T>::value>::type
				bindT(MYSQL_BIND &param) noexcept { param.is_unsigned = std::is_unsigned<T>::value; }

			template<bool> struct bindValueIn_t
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

					paramStorage = substrate::make_managed_nothrow<MYSQL_TIME>(date);
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

					paramStorage = substrate::make_managed_nothrow<MYSQL_TIME>(dateTime);
					if (!paramStorage)
						return false;
					param.buffer = paramStorage;
					param.buffer_length = sizeof(dateTime);
					return true;
				}

				bool operator ()(MYSQL_BIND &param, const ormUUID_t &_value, managedPtr_t<void> &paramStorage) noexcept
				{
					std::array<uint8_t, sizeof(guid_t)> value{};
					memcpy(value.data(), _value.asPointer(), value.size());
					std::array<char, 32> uuid{};
					for (uint8_t i = 0; i < uuid.size(); ++i)
					{
						// Computes a shift of 4 for the first nibble, and 0 for the second
						const uint8_t shift = 4U >> ((i & 1U) << 2U);
						// This then extracts the correct nibble of the current byte to convert. It acomplishes
						// this by performing a shift to get the correct nibble into the bottom nibble of the byte
						// and then masking off that nibble
						char hex = uint8_t(value[i >> 1U] >> shift) & 0x0FU;
						if (hex > 9)
							hex += 0x07;
						uuid[i] = hex + 0x30;
					}

					auto storage = substrate::make_managed_nothrow<decltype(uuid)>(uuid);
					if (!storage)
						return false;
					param.buffer = storage->data();
					param.buffer_length = uuid.size();
					paramStorage = std::move(storage);
					return true;
				}
			};

			template<> struct bindValueIn_t<true>
			{
				template<typename T> bool operator ()(MYSQL_BIND &param, const T *const value, managedPtr_t<void> &) noexcept
					{ param.buffer = const_cast<T *>(value); return true; }
			};

			template<typename T> using bindValueIn_ = bindValueIn_t<std::is_pointer<T>::value>;

			template<typename T> void mySQLBind_t::bindIn(const size_t index, const T &value, const fieldLength_t length) noexcept
			{
				if (index >= numParams)
					return;
				MYSQL_BIND &param = params[index];
				param.buffer_type = bindType_t<T>::value;
				param.buffer_length = length.first;
				if (!bindValueIn_<T>()(param, value, paramStorage[index]))
					return;
				param.length = &param.buffer_length;
				param.is_null = notNullParam;
				bindT<T>(param);
			}

			template<typename T> void mySQLBind_t::bindIn(const size_t index, const nullptr_t, const fieldLength_t) noexcept
			{
				if (index >= numParams)
					return;
				MYSQL_BIND &param = params[index];
				param.buffer_type = bindType_t<T>::value;
				param.is_null = const_cast<char *>(&nullParam);
			}

			template<typename value_t> struct bindOutStorage_t
			{
				constexpr uint32_t length() const noexcept { return sizeof(value_t); }
				managedPtr_t<void> operator ()() const noexcept { return substrate::make_managed_nothrow<value_t>(); }
			};

			template<> struct bindOutStorage_t<ormDate_t>
			{
				constexpr uint32_t length() const noexcept { return sizeof(MYSQL_TIME); }
				managedPtr_t<void> operator ()() const noexcept { return substrate::make_managed_nothrow<MYSQL_TIME>(); }
			};

			template<> struct bindOutStorage_t<ormDateTime_t>
			{
				constexpr uint32_t length() const noexcept { return sizeof(MYSQL_TIME); }
				managedPtr_t<void> operator ()() const noexcept { return substrate::make_managed_nothrow<MYSQL_TIME>(); }
			};

			/*template<> struct bindOutStorage_t<ormUUID_t>
			{
				constexpr uint32_t length() const noexcept { return 32; }
				managedPtr_t<void> operator ()() const noexcept { return substrate::make_managed_nothrow<char []>(32); }
			};*/

			template<typename T> struct bindValueOut_t
			{
				bool operator ()(MYSQL_BIND &param, managedPtr_t<void> &paramStorage) noexcept
				{
					const bindOutStorage_t<T> makeStorage;
					paramStorage = makeStorage();
					if (!paramStorage)
						return false;
					param.buffer_length = makeStorage.length();
					return true;
				}
			};

			template<typename T> void mySQLBind_t::bindOut(const size_t index, const fieldLength_t)
				noexcept
			{
				if (index >= numParams)
					return;
				MYSQL_BIND &param = params[index];
				param.buffer_type = bindType_t<T>::value;
				if (!bindValueOut_t<T>()(param, paramStorage[index]))
					return;
				param.length = &param.buffer_length;
				param.is_null = notNullParam;
			}
		}

		/*! @brief Adds backticks around a field or table name */
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
			template<typename fieldName, typename T> static auto _name(const type_t<fieldName, T> &) ->
				typename createName_t<type_t<fieldName, T>>::value;
			template<typename fieldName, size_t length> static auto _name(const unicode_t<fieldName, length> &) ->
				typename createName_t<unicode_t<fieldName, length>>::value;
			template<typename T> static auto _name(const autoInc_t<T> &) -> tycat<decltype(_name(T())), ts(" AUTO_INCREMENT")>;
			template<typename T> static auto _name(const primary_t<T> &) -> tycat<decltype(_name(T())), ts(" PRIMARY KEY")>;
			using name = decltype(_name(field()));

			static auto value() -> tycat<name, nullable<field::nullable>, comma<N>>;
		};
		// Alias for the above container type to make it easier to use
		template<size_t N, typename T> using createList__ = decltype(createList__t<N, T>::value());

		template<size_t N, typename field, typename... fields> struct createList_t
			{ using value = tycat<createList__<N, field>, typename createList_t<N - 1, fields...>::value>; };
		template<typename field> struct createList_t<1, field> { using value = createList__<1, field>; };
		// Alias to make the above easier to use
		template<typename... fields> using createList = typename createList_t<sizeof...(fields), fields...>::value;

		template<typename tableName, typename... fields> using createTable_ = toString<
			tycat<ts("CREATE TABLE IF NOT EXISTS "), backtick<tableName>, ts(" ("), createList<fields...>, ts(") CHARACTER SET utf8;")>
		>;
		template<typename tableName, typename... fields> using select_ = toString<
			tycat<ts("SELECT "), selectList<fields...>, ts(" FROM "), backtick<tableName>, ts(";")>
		>;
		template<typename tableName, typename where, typename... fields> using selectWhere_ = toString<
			tycat<ts("SELECT "), selectList<fields...>, ts(" FROM "), backtick<tableName>, selectWhere<where>, ts(";")>
		>;
		// tycat<> builds up the query string for inserting the data
		template<typename tableName, typename... fields> using add_ = toString<
			tycat<ts("INSERT INTO "), backtick<tableName>, ts(" ("), insertList<fields...>, ts(") VALUES ("), placeholder<countInsert_t<fields...>::count>, ts(");")>
		>;
		// tycat<> builds up the query string for inserting the data
		template<typename tableName, typename... fields> using addAll_ = toString<
			tycat<ts("INSERT INTO "), backtick<tableName>, ts(" ("), insertAllList<fields...>, ts(") VALUES ("), placeholder<sizeof...(fields)>, ts(");")>
		>;
		// This constructs invalid if there is no field marked primary_t<>! This is quite intentional.
		template<typename tableName, typename... fields> struct update_t<false, tableName, fields...>
			{ using value = tycat<ts("UPDATE "), backtick<tableName>, ts(" SET "), updateList<fields...>, updateWhere<fields...>, ts(";")>; };
		template<typename tableName, typename... fields> using del_ = toString<
			tycat<ts("DELETE FROM "), backtick<tableName>, updateWhere<fields...>, ts(";")>
		>;
		template<typename tableName> using deleteTable_ = toString<
			tycat<ts("DROP TABLE IF EXISTS "), backtick<tableName>, ts(";")>
		>;

		struct session_t final
		{
		private:
			driver::mySQLClient_t database;

		public:
			session_t() noexcept = default;
			~session_t() noexcept = default;
			session_t(session_t &&session) noexcept : database{session.database} { }
			void operator =(session_t &&session) noexcept { database = session.database; }

			template<typename tableName, typename... fields> bool createTable(const model_t<tableName, fields...> &)
			{
				using create = createTable_<tableName, fields...>;
				return database.query(create::value);
			}

			template<typename T, typename tableName, typename... fields_t> fixedVector_t<T> select(const model_t<tableName, fields_t...> &)
			{
				using select = select_<tableName, fields_t...>;
				if (!database.query(select::value))
					throw mySQLValueError_t(mySQLErrorType_t::queryError);
				mySQLResult_t result = database.queryResult();
				if (!result.valid())
					throw mySQLValueError_t(mySQLErrorType_t::queryError);
				mySQLRow_t row = result.resultRows();
				fixedVector_t<T> data{result.numRows()};
				if (!data.valid())
					return {};
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

			template<typename T, typename where, typename tableName, typename... fields_t> fixedVector_t<T> select(const model_t<tableName, fields_t...> &, const where &cond)
			{
				// Generate the SELECT query with WHERE clause
				using select = selectWhere_<tableName, where, fields_t...>;
				// Now prepare that query abd bind data to the WHERE clause
				mySQLPreparedQuery_t query{database.prepare(select::value, countCond_t<where>::count)};
				bindCond<where, fields_t...>::bind(cond, query);
				// Next, run the query
				if (!query.execute())
					throw mySQLValueError_t(mySQLErrorType_t::queryError);
				// Pull the result set back
				mySQLPreparedResult_t result = query.queryResult(sizeof...(fields_t));
				if (!result.valid())
					throw mySQLValueError_t(mySQLErrorType_t::queryError);
				fixedVector_t<T> data{result.numRows()};
				if (!data.valid())
					return {};
				// For each result, bind a new T's fields to the columns in the query and pull the resulting data set back
				// This is split into a pre-bind phase that does an initial call to ask for field lengths of variable length fields
				// Then does an allocations and bind pass for the real call to ask for the data of all the fields
				for (size_t i = 0; i < result.numRows(); ++i)
				{
					T value;
					bindSelectCore<fields_t...>::bind(value.fields(), result);
					// If there is an issue fetching the data for this record, return the empty fixedVector_t<>
					if (!result.next())
						return {};
					//bindSelectAlloc<fields_t...>::bind(value.fields(), result);
					data[i] = std::move(value);
				}
				return data;
			}

			// Unpacks a model_t into its name and fields
			template<typename tableName, typename... fields_t> bool add(model_t<tableName, fields_t...> &model)
			{
				using add = add_<tableName, fields_t...>;
				mySQLPreparedQuery_t query{database.prepare(add::value, countInsert_t<fields_t...>::count)};
				bindInsert<fields_t...>::bind(model.fields(), query);
				if (query.execute())
				{
					setAutoInc_t<hasAutoInc<fields_t...>()>::set(model, query.rowID());
					return true;
				}
				return false;
			}

			template<typename tableName, typename... fields_t> bool add(const model_t<tableName, fields_t...> &model)
			{
				using add = addAll_<tableName, fields_t...>;
				mySQLPreparedQuery_t query(database.prepare(add::value, sizeof...(fields_t)));
				// This binds the fields in order so we insert a value for every column.
				bindInsertAll<fields_t...>::bind(model.fields(), query);
				// This either works or doesn't.. thankfully.. so, we can just execute-and-quit.
				return query.execute();
			}

			template<typename tableName, typename... fields_t> bool update(const model_t<tableName, fields_t...> &model)
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

			template<typename tableName, typename... fields_t> bool del(const model_t<tableName, fields_t...> &model)
			{
				using del = del_<tableName, fields_t...>;
				mySQLPreparedQuery_t query(database.prepare(del::value, countPrimary<fields_t...>::count));
				// This binds just the primary keys of the model so it tags in-order to the WHERE clause for this query.
				bindDelete<fields_t...>::bind(model.fields(), query);
				// This either works or doesn't.. thankfully.. so, we can just execute-and-quit.
				return query.execute();
			}

			template<typename tableName, typename... fields> bool deleteTable(const model_t<tableName, fields...> &)
			{
				using drop = deleteTable_<tableName>;
				// tycat<> builds up the query for dropping (deleting) the table
				return database.query(drop::value);
			}

			bool connect(const char *const host, const uint32_t port, const char *const user, const char *const passwd) const noexcept
				{ return database.connect(host, port, user, passwd); }
			bool connect(const char *const unixSocket, const char *const user, const char *const passwd) const noexcept
				{ return database.connect(unixSocket, user, passwd); }
			void disconnect() noexcept { database.disconnect(); }
			bool selectDB(const char *const db) const noexcept { return database.selectDB(db); }
			const char *error() const noexcept { return database.error(); }
			uint32_t errorNum() const noexcept { return database.errorNum(); }

			session_t(const session_t &) = delete;
			session_t &operator =(const session_t &) = delete;
		};
	}
	using mysql_t = mysql::session_t;
}

#endif /*tmplORM_MYSQL_HXX*/
