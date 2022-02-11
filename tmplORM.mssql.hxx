#ifndef tmplORM_MSSQL_HXX
#define tmplORM_MSSQL_HXX

#ifndef UNICODE
#define UNICODE
#include <sql.h>
#include <sqlext.h>
#endif
#include <tmplORM.hxx>
#include <mssql.hxx>
//#include <string.h>

/*!
 * @file
 * @author Rachel Mant
 * @date 2016-2020
 * @brief Defines the MSSQL mapper for the ORM
 */

namespace tmplORM
{
	namespace mssql
	{
		using substrate::managedPtr_t;
		using namespace tmplORM::common;
		using namespace tmplORM::mssql::driver;
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
		template<> struct stringType_t<int16_t> { using value = ts("SMALLINT"); };
		template<> struct stringType_t<uint16_t> { using value = ts("SMALLINT UNSIGNED"); };
		template<> struct stringType_t<int32_t> { using value = ts("INT"); };
		template<> struct stringType_t<uint32_t> { using value = ts("INT UNSIGNED"); };
		template<> struct stringType_t<int64_t> { using value = ts("BIGINT"); };
		template<> struct stringType_t<uint64_t> { using value = ts("BIGINT UNSIGNED"); };
		template<> struct stringType_t<bool> { using value = ts("BIT"); };
		template<> struct stringType_t<float> { using value = ts("REAL"); };
		template<> struct stringType_t<double> { using value = ts("DOUBLE"); };
		template<> struct stringType_t<char *> { using value = ts("NVARCHAR(MAX)"); };
		template<> struct stringType_t<ormDate_t> { using value = ts("DATE"); };
		template<> struct stringType_t<ormDateTime_t> { using value = ts("DATETIME2"); };
		template<> struct stringType_t<ormUUID_t> { using value = ts("UNIQUEIDENTIFIER"); };
		template<typename T> using stringType = typename stringType_t<T>::value;

		template<typename> struct bind_t { };
		template<> struct bind_t<int8_t>
			{ constexpr static const int16_t typeC = SQL_C_STINYINT; constexpr static const int16_t typeODBC = SQL_TINYINT; };
		template<> struct bind_t<int16_t>
			{ constexpr static const int16_t typeC = SQL_C_SSHORT; constexpr static const int16_t typeODBC = SQL_SMALLINT; };
		template<> struct bind_t<int32_t>
			{ constexpr static const int16_t typeC = SQL_C_SLONG; constexpr static const int16_t typeODBC = SQL_INTEGER; };
		template<> struct bind_t<int64_t>
			{ constexpr static const int16_t typeC = SQL_C_SBIGINT; constexpr static const int16_t typeODBC = SQL_BIGINT; };
		//
		template<> struct bind_t<bool>
			{ constexpr static const int16_t typeC = SQL_C_BIT; constexpr static const int16_t typeODBC = SQL_BIT; };
		template<> struct bind_t<float>
			{ constexpr static const int16_t typeC = SQL_C_FLOAT; constexpr static const int16_t typeODBC = SQL_REAL; };
		template<> struct bind_t<double>
			{ constexpr static const int16_t typeC = SQL_C_DOUBLE; constexpr static const int16_t typeODBC = SQL_DOUBLE; };
		template<> struct bind_t<char *>
			{ constexpr static const int16_t typeC = SQL_C_CHAR; constexpr static const int16_t typeODBC = SQL_VARCHAR; };
		template<> struct bind_t<const char *>
			{ constexpr static const int16_t typeC = SQL_C_CHAR; constexpr static const int16_t typeODBC = SQL_VARCHAR; };
 		template<> struct bind_t<void *>
 			{ constexpr static int16_t typeC = SQL_C_BINARY; constexpr static int16_t typeODBC = SQL_BINARY; };
		template<> struct bind_t<ormDate_t>
			{ constexpr static const int16_t typeC = SQL_C_TYPE_DATE; constexpr static const int16_t typeODBC = SQL_TYPE_DATE; };
		template<> struct bind_t<ormDateTime_t>
			{ constexpr static const int16_t typeC = SQL_C_TYPE_TIMESTAMP; constexpr static const int16_t typeODBC = SQL_TYPE_TIMESTAMP; };
		template<> struct bind_t<ormUUID_t>
			{ constexpr static const int16_t typeC = SQL_C_GUID; constexpr static const int16_t typeODBC = SQL_GUID; };
		/*template<> struct bind_t<nullptr_t>
			{ constexpr static const int16_t typeC = SQL_C_DEFAULT; constexpr static const int16_t typeODBC = SQL_TYPE_NULL; };*/

		namespace driver
		{
			template<typename T> struct bindLength_t { constexpr static uint32_t length = sizeof(T); };
			template<> struct bindLength_t<ormDate_t> { constexpr static uint32_t length = sizeof(SQL_DATE_STRUCT); };
			template<> struct bindLength_t<ormDateTime_t> { constexpr static uint32_t length = sizeof(SQL_TIMESTAMP_STRUCT); };
			template<> struct bindLength_t<ormUUID_t> { constexpr static uint32_t length = sizeof(SQLGUID); };

			template<bool> struct bindValue_t
			{
				template<typename T> void *operator ()(const T &value, managedPtr_t<void> &paramStorage) const noexcept
				{
					paramStorage = substrate::make_managed_nothrow<T>(value);
					return paramStorage.get();
				}

				void *operator ()(const ormDate_t &value, managedPtr_t<void> &paramStorage) const noexcept
				{
					SQL_DATE_STRUCT date;
					date.year = value.year();
					date.month = value.month();
					date.day = value.day();
					paramStorage = substrate::make_managed_nothrow<SQL_DATE_STRUCT>(date);
					return paramStorage.get();
				}

				void *operator ()(const ormDateTime_t &value, managedPtr_t<void> &paramStorage) const noexcept
				{
					SQL_TIMESTAMP_STRUCT dateTime;
					dateTime.year = value.year();
					dateTime.month = value.month();
					dateTime.day = value.day();
					dateTime.hour = value.hour();
					dateTime.minute = value.minute();
					dateTime.second = value.second();
					// The documentation tells us that this field is in ns, so..
					dateTime.fraction = value.nanoSecond() / 100;
					dateTime.fraction *= 100; // This is here to fix the accuracy for the server

					paramStorage = substrate::make_managed_nothrow<SQL_TIMESTAMP_STRUCT>(dateTime);
					return paramStorage.get();
				}

				void *operator ()(const ormUUID_t &value, managedPtr_t<void> &paramStorage) const noexcept
				{
					SQLGUID guid{};
					memcpy(&guid, value.asPointer(), sizeof(guid_t));
					swapBytes(guid.Data1);
					swapBytes(guid.Data2);
					swapBytes(guid.Data3);
					paramStorage = substrate::make_managed_nothrow<SQLGUID>(guid);
					return paramStorage.get();
				}
			};

			template<> struct bindValue_t<true>
			{
				template<typename T> void *operator ()(const T *const val, managedPtr_t<void> &) const noexcept
					{ return const_cast<T *>(val); } // NOLINT(cppcoreguidelines-pro-type-const-cast)
			};

			template<typename T> using bindValue = bindValue_t<std::is_pointer<T>::value>;
			template<typename T> size_t bindDigits(size_t value) noexcept { return value; }
			// 27 here might seem arbitrary but it's because of MSSQL and ODBC.
			// More specifically, https://docs.microsoft.com/en-us/sql/odbc/reference/appendixes/column-size
			// which states that for SQL_TYPE_TIMESTAMP, the precision field must be populated with a value that is
			// 20 + s, where s is the 'seconds precision' - a ns value requires 9 radix-10 digits, but server only supports 7.
			template<> constexpr size_t bindDigits<ormDateTime_t>(size_t) noexcept { return 27; }
			// This value is a magic number thing - 10 is the number of characters in the date format specification
			template<> constexpr size_t bindDigits<ormDate_t>(size_t) noexcept { return 10; }
			// This value is also a magic number thing - 9 for the core type + 9 for the ns value as above.
			//template<> constexpr size_t bindDigits<ormTime_t>(size_t) noexcept { return 18; }
			// Other documentation: https://docs.microsoft.com/en-us/sql/t-sql/data-types/datetime2-transact-sql

			template<typename T> constexpr int16_t bindScale() noexcept { return 0; }
			template<> constexpr int16_t bindScale<ormDateTime_t>() noexcept { return 7; }
			//template<> constexpr int16_t bindScale<ormTime_t>() noexcept { return 7; }

			template<typename T> void tSQLQuery_t::bind(const size_t index, const T &value,
				const fieldLength_t length) noexcept
			{
				const int16_t dataType = bind_t<T>::typeC;
				const int16_t odbcDataType = bind_t<T>::typeODBC;
				const uint32_t dataLen = length.first ? length.first : bindLength_t<T>::length;

				long *lenPtr = dataType == SQL_C_BINARY ? &dataLengths[index] : nullptr;
				if (dataType == SQL_C_BINARY)
					dataLengths[index] = dataLen;

				error(SQLBindParameter(queryHandle, index + 1, SQL_PARAM_INPUT, dataType, odbcDataType,
					bindDigits<T>(length.second), bindScale<T>(), bindValue<T>{}(value, paramStorage[index]),
					dataLen, lenPtr));
			}

			template<typename T> void tSQLQuery_t::bind(const size_t index, const nullptr_t,
				const fieldLength_t length) noexcept
			{
				const int16_t dataType = bind_t<T>::typeC;
				const int16_t odbcDataType = bind_t<T>::typeODBC;
				dataLengths[index] = SQL_NULL_DATA;
				error(SQLBindParameter(queryHandle, index + 1, SQL_PARAM_INPUT, dataType, odbcDataType, length.second,
					0, nullptr, 0, &dataLengths[index]));
			}
		} // namespace driver

		/*! @brief Adds brackets around a field or table name */
		template<typename name> using bracket = tycat<ts("["), name, ts("]")>;

		// Formatting type for handling field names (to make lists from them)
		template<size_t, typename> struct fieldName_t { };
		template<size_t N, typename fieldName, typename T> struct fieldName_t<N, type_t<fieldName, T>>
			{ using value = tycat<bracket<fieldName>, comma<N>>; };

#include "tmplORM.common.hxx"

		template<typename> struct createName_t { };
		template<typename fieldName, typename T> struct createName_t<type_t<fieldName, T>>
			{ using value = tycat<bracket<fieldName>, ts(" "), stringType<T>>; };
		template<typename fieldName, size_t length> struct createName_t<unicode_t<fieldName, length>>
			{ using value = tycat<bracket<fieldName>, ts(" NVARCHAR("), toTypestring<length>, ts(")")>; };

		template<size_t N, typename field> struct createField_t
		{
			template<typename fieldName, typename T> static auto _name(const type_t<fieldName, T> &) ->
				typename createName_t<type_t<fieldName, T>>::value;
			template<typename fieldName, size_t length> static auto _name(const unicode_t<fieldName, length> &) ->
				typename createName_t<unicode_t<fieldName, length>>::value;
			template<typename T> static auto _name(const autoInc_t<T> &) ->
				tycat<decltype(_name(T{})), ts(" IDENTITY")>;
			template<typename T> static auto _name(const primary_t<T> &) ->
				tycat<decltype(_name(T{})), ts(" PRIMARY KEY")>;
			using name = decltype(_name(field{}));

			static auto value() -> tycat<name, nullable<field::nullable>, comma<N>>;
		};
		// Alias for the above container type to make it easier to use
		template<size_t N, typename T> using createField = decltype(createField_t<N, T>::value());

		// Constructs a list of fields suitable for use in a CREATE query
		template<size_t, typename...> struct createList_t;
		// Primary specialisation generates the list
		template<size_t N, typename field, typename... fields> struct createList_t<N, field, fields...>
			{ using value = tycat<createField<N, field>, typename createList_t<N - 1, fields...>::value>; };
		template<> struct createList_t<0> { using value = typestring<>; };
		// Alias to make createList_t easier to use
		template<typename... fields> using createList = typename createList_t<sizeof...(fields), fields...>::value;

		template<size_t N, typename field, typename... fields> struct outputField_t
			{ using value = typename outputField_t<N - 1, fields...>::value; };
		template<typename field, typename... fields> struct outputField_t<0, field, fields...>
		{
			template<typename fieldName, typename T> static auto _value(const type_t<fieldName, T> &) ->
				typename fieldName_t<1, type_t<fieldName, T>>::value;
			using value = tycat<ts("INSERTED."), decltype(_value(field()))>;
		};
		// Alias to make outputField_t easier to use
		template<typename... fields> using outputField = typename outputField_t<autoIncIndex_t<fields...>::index, fields...>::value;

		template<bool, typename... fields> struct outputInsert_t { using value = typestring<>; };
		template<typename... fields> struct outputInsert_t<true, fields...>
			{ using value = tycat<ts(" OUTPUT "), outputField<fields...>>; };
		// Alias to make outputInsert_t easier to use
		template<typename... fields> using outputInsert = typename outputInsert_t<hasAutoInc<fields...>(), fields...>::value;

		template<typename tableName, typename... fields> using createTable_ = toString<
			tycat<ts("CREATE TABLE "), bracket<tableName>, ts(" ("), createList<fields...>, ts(") COLLATE latin1_general_100_CI_AI_SC;")>
		>;
		template<typename tableName, typename... fields> using select_ = toString<
			tycat<ts("SELECT "), selectList<fields...>, ts(" FROM "), bracket<tableName>, ts(";")>
		>;
		template<typename tableName, typename... fields> using add_ = toString<
			tycat<ts("INSERT INTO "), bracket<tableName>, ts(" ("), insertList<fields...>, ts(")"), outputInsert<fields...>, ts(" VALUES ("), placeholder<countInsert_t<fields...>::count>, ts(");")>
		>;
		template<typename tableName, typename... fields> using addAll_ = toString<
			tycat<ts("INSERT INTO "), bracket<tableName>, ts(" ("), insertAllList<fields...>, ts(") VALUES ("), placeholder<sizeof...(fields)>, ts(");")>
		>;
		// This constructs invalid if there is no field marked primary_t<>! This is quite intentional.
		template<typename tableName, typename... fields> struct update_t<false, tableName, fields...>
			{ using value = tycat<ts("UPDATE "), bracket<tableName>, ts(" SET "), updateList<fields...>, updateWhere<fields...>, ts(";")>; };
		template<typename tableName, typename... fields> using del_ = toString<
			tycat<ts("DELETE FROM "), bracket<tableName>, updateWhere<fields...>, ts(";")>
		>;
		template<typename tableName> using deleteTable_ = toString<
			tycat<ts("DROP TABLE "), bracket<tableName>, ts(";")>
		>;

		struct session_t final
		{
		private:
			driver::tSQLClient_t database;

		public:
			session_t() noexcept = default;
			~session_t() noexcept = default;
			session_t(session_t &&) noexcept = default;
			session_t &operator =(session_t &&) noexcept = default;

			template<typename tableName, typename... fields> bool createTable(const model_t<tableName, fields...> &)
			{
				using create = createTable_<tableName, fields...>;
				return database.query(create::value).valid();
			}

			template<typename T, typename tableName, typename... fields_t> fixedVector_t<T> select(const model_t<tableName, fields_t...> &) noexcept
			{
				using select = select_<tableName, fields_t...>;
				tSQLResult_t result(database.query(select::value));
				fixedVector_t<T> data{result.numRows()};
				if (!data.valid())
					return {};
				for (size_t i = 0; i < result.numRows(); ++i, result.next())
				{
					T value;
					if (!result.valid())
						return {};
					bindSelect<fields_t...>::bind(value.fields(), result);
					data[i] = std::move(value);
				}
				if (result.valid())
					return {};
				return data;
			}

			/*template<typename T, typename where, typename tableName, typename... fields_t> fixedVector_t<T> select(const model_t<tableName, fields_t...> &, const where &cond) noexcept
			{
				fixedVector_t<T> data;
				using select = select_<tableName, where, fields_t...>;
				tSQLResult_t result(database.query(select::value));
				for (size_t i = 0; i < result.numRows(); ++i, result.next())
				{
					T value;
					if (!result.valid())
						return {};
					bindSelect<fields_t...>::bind(value.fields(), result);
					data[i] = std::move(value);
				}
				if (result.valid())
					return {};
				return data;
			}*/

			template<typename tableName, typename... fields_t> bool add(model_t<tableName, fields_t...> &model) noexcept
			{
				using insert = add_<tableName, fields_t...>;
				tSQLQuery_t query(database.prepare(insert::value, countInsert_t<fields_t...>::count));
				bindInsert<fields_t...>::bind(model.fields(), query);
				tSQLResult_t result(query.execute());
				if (result.valid())
				{
					setAutoInc_t<hasAutoInc<fields_t...>()>::set(model, result[0]);
					return true;
				}
				return false;
			}

			template<typename tableName, typename... fields_t> bool add(const model_t<tableName, fields_t...> &model) noexcept
			{
				using insert = addAll_<tableName, fields_t...>;
				tSQLQuery_t query(database.prepare(insert::value, sizeof...(fields_t)));
				// This binds the fields in order so we insert a value for every column.
				bindInsertAll<fields_t...>::bind(model.fields(), query);
				// This either works or doesn't.. thankfully.. so, we can just execute-and-quit.
				return query.execute().valid();
			}

			template<typename tableName, typename... fields_t> bool update(const model_t<tableName, fields_t...> &model) noexcept
			{
				using update = update_<tableName, fields_t...>;
				if (std::is_same<update, toString<typestring<>>>::value)
					return false;
				tSQLQuery_t query(database.prepare(update::value, sizeof...(fields_t)));
				// This binds the fields, primary key last so it tags to the WHERE clause for this query.
				bindUpdate<fields_t...>::bind(model.fields(), query);
				// This either works or doesn't.. thankfully.. so, we can just execute-and-quit.
				return query.execute().valid();
			}

			template<typename tableName, typename... fields_t> bool del(const model_t<tableName, fields_t...> &model) noexcept
			{
				using del = del_<tableName, fields_t...>;
				tSQLQuery_t query(database.prepare(del::value, countPrimary<fields_t...>::count));
				// This binds the primary key fields only, in the order they're given in the WHERE clause for this query.
				bindDelete<fields_t...>::bind(model.fields(), query);
				// This either works or doesn't.. thankfully.. so, we can just execute-and-quit.
				return query.execute().valid();
			}

			template<typename tableName, typename... fields> bool deleteTable(const model_t<tableName, fields...> &)
			{
				// tycat<> builds up the query for dropping (deleting) the table
				using drop = deleteTable_<tableName>;
				return database.query(drop::value).valid();
			}

			bool connect(const char *const driver, const char *const host, const uint32_t port, const char *const user, const char *const passwd) const noexcept
				{ return database.connect(driver, host, port, user, passwd); }
			void disconnect() noexcept { database.disconnect(); }
			bool selectDB(const char *const db) const noexcept { return database.selectDB(db); }
			const tSQLExecError_t &error() const noexcept { return database.error(); }

			session_t(const session_t &) = delete;
			session_t &operator =(const session_t &) = delete;
		};
	} // namespace mssql
	using mssql_t = mssql::session_t;
} // namespace tmplORM

#endif /*tmplORM_MSSQL_HXX*/
