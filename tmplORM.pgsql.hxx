#ifndef tmplORM_PGSQL_HXX
#define tmplORM_PGSQL_HXX

#include <substrate/buffer_utils>
#include "tmplORM.hxx"
#include "pgsql.hxx"

namespace tmplORM
{
	namespace pgsql
	{
		using namespace tmplORM::common;
		using namespace tmplORM::types::baseTypes;

		using tmplORM::types::type_t;
		using tmplORM::types::unicode_t;
		using tmplORM::types::unicodeText_t;

		using tmplORM::types::primary_t;
		using tmplORM::types::autoInc_t;
		using tmplORM::types::nullable_t;

		// If we don't know how to translate the type, don't.
		template<typename> struct stringType_t { using value = typestring<>; };
		// Postgres doesn't have a TINYINT/INT1 type, so fake it.
		template<> struct stringType_t<int8_t> { using value = ts("INT2"); };
		template<> struct stringType_t<int16_t> { using value = ts("INT2"); };
		template<> struct stringType_t<int32_t> { using value = ts("INT4"); };
		template<> struct stringType_t<int64_t> { using value = ts("INT8"); };
		template<> struct stringType_t<bool> { using value = ts("BOOLEAN"); };
		template<> struct stringType_t<float> { using value = ts("FLOAT4"); };
		template<> struct stringType_t<double> { using value = ts("FLOAT8"); };
		template<> struct stringType_t<char *> { using vaue = ts("TEXT"); };
		template<> struct stringType_t<void *> { using value = ts("BLOB"); };
		template<> struct stringType_t<ormDate_t> { using value = ts("DATE"); };
		//template<> struct stringType_t<ormTime_t> { using value = ts("TIME"); };
		template<> struct stringType_t<ormDateTime_t> { using value = ts("TIMESTAMP"); };
		template<> struct stringType_t<ormUUID_t> { using value = ts("UUID"); };
		template<typename T> using stringType = typename stringType_t<T>::value;

		namespace driver
		{
			template<typename> struct bind_t { };
			template<> struct bind_t<int8_t> { constexpr static auto value = pgSQLType_t::int2; };
			template<> struct bind_t<int16_t> { constexpr static auto value = pgSQLType_t::int2; };
			template<> struct bind_t<int32_t> { constexpr static auto value = pgSQLType_t::int4; };
			template<> struct bind_t<int64_t> { constexpr static auto value = pgSQLType_t::int8; };
			template<> struct bind_t<bool> { constexpr static auto value = pgSQLType_t::boolean; };
			template<> struct bind_t<float> { constexpr static auto value = pgSQLType_t::float4; };
			template<> struct bind_t<double> { constexpr static auto value = pgSQLType_t::float8; };
			template<> struct bind_t<char *> { constexpr static auto value = pgSQLType_t::unicode; };
			template<> struct bind_t<void *> { constexpr static auto value = pgSQLType_t::binary; };
			template<> struct bind_t<ormDate_t> { constexpr static auto value = pgSQLType_t::date; };
			//template<> struct bind_t<ormTime_t> { constexpr static auto value = pgSQLType_t::time; };
			template<> struct bind_t<ormDateTime_t> { constexpr static auto value = pgSQLType_t::dateTime; };
			template<> struct bind_t<ormUUID_t> { constexpr static auto value = pgSQLType_t::uuid; };

			template<typename T> struct bindLength_t { constexpr static int32_t length = sizeof(T); };
			template<> struct bindLength_t<ormDate_t> { constexpr static int32_t length = sizeof(int32_t); };
			template<> struct bindLength_t<ormDateTime_t> { constexpr static int32_t length = sizeof(int64_t); };

			template<bool> struct bindValue_t
			{
				template<typename T> static const char *bind(const T &value, managedPtr_t<void> &paramStorage) noexcept
				{
					paramStorage = substrate::make_managed_nothrow<T>();
					substrate::buffer_utils::writeBE(value, paramStorage.get());
					return static_cast<const char *>(paramStorage.get());
				}
			};

			template<> struct bindValue_t<true>
			{
				static const char *bind(const void *const value, managedPtr_t<void> &) noexcept
					{ return static_cast<const char *>(value); }
			};

			template<typename T> using bindValue = bindValue_t<std::is_pointer<T>::value>;

			template<typename T> void pgSQLQuery_t::bind(const size_t index, const T &value,
				const fieldLength_t length) noexcept
			{
				paramTypes[index] = typeToOID(bind_t<T>::value);
				params[index] = bindValue<T>::bind(value, paramStorage[index]);
				dataLengths[index] = length.first ? length.first : bindLength_t<T>::length;
			}

			template<typename T> void pgSQLQuery_t::bind(const size_t index, const nullptr_t,
				const fieldLength_t length) noexcept
			{
				paramTypes[index] = typeToOID(pgSQLType_t::null);
				params[index] = nullptr;
				dataLengths[index] = 0;
			}
		} // namespace driver

		template<typename name> using doubleQuote = tycat<ts("\""), name, ts("\"")>;

		template<size_t, typename> struct fieldName_t { };
		template<size_t N, typename fieldName, typename T> struct fieldName_t<N, type_t<fieldName, T>>
			{ using value = tycat<doubleQuote<fieldName>, comma<N>>; };

		template<size_t count, size_t N> struct placeholder_t
		{
			using value = tycat<
				typestring<'$'>,
				toTypestring<N>,
				comma<count>,
				typename placeholder_t<count - 1, N + 1>::value
			>;
		};
		// Termination here is for 0 rather than 1 to protect us when there are no placeholders to generate
		template<size_t N> struct placeholder_t<0, N> { using value = typestring<>; };
		/*! @brief Generates a list of N prepared execution placeholders for a query statement */
		template<size_t count, size_t N> using placeholder = typename placeholder_t<count, N>::value;

		// Intermediary container type for handling conversion of a field into a form suitable for a SELECT query
		template<size_t N> struct selectField_t
		{
			template<typename fieldName, typename T> static auto value(const type_t<fieldName, T> &) ->
				typename fieldName_t<N, type_t<fieldName, T>>::value;
		};
		// Alias for the above container type to make it easier to use.
		template<size_t N, typename T> using selectField = decltype(selectField_t<N>::value(T{}));

		// Constructs a list of fields suitable for use in a SELECT query
		template<size_t, typename...> struct selectList_t;
		// Alias for selectList_t to make it easier to use.
		template<typename... fields> using selectList = typename selectList_t<sizeof...(fields), fields...>::value;
		// Primary specialisation generates the list
		template<size_t N, typename field, typename... fields> struct selectList_t<N, field, fields...>
			{ using value = tycat<selectField<N, field>, selectList<fields...>>; };
		template<> struct selectList_t<0> { using value = typestring<>; };

		// TODO: Implement WHERE clause support

		// Intermediary container type for handling conversion of a field into a form suitable for an INSERT query
		template<size_t N> struct insertField_t
		{
			template<typename fieldName, typename T> static auto value(const type_t<fieldName, T> &) ->
				typename fieldName_t<N, type_t<fieldName, T>>::value;
			template<typename T> static auto value(const autoInc_t<T> &) -> typestring<>;
		};
		// Alias for the above container type to make it easier to use.
		template<size_t N, typename T> using insertField = decltype(insertField_t<N>::value(T{}));

		// Constructs a list of fields suitable for use in an INSERT query
		template<size_t, typename...> struct insertList_t;
		// Alias for the above to make it easier to use.
		template<typename... fields> using insertList = typename insertList_t<sizeof...(fields), fields...>::value;
		// Primary specialisation generates the list
		template<size_t N, typename field, typename... fields> struct insertList_t<N, field, fields...>
			{ using value = tycat<insertField<N, field>, insertList<fields...>>; };
		template<> struct insertList_t<0> { using value = typestring<>; };

		template<bool, size_t N> struct retrieveIDField_t
		{
			template<typename fieldName, typename T> static auto value(const type_t<fieldName, T> &) ->
				tycat<typename fieldName_t<1, type_t<fieldName, T>>::value, comma<N>>;
		};
		template<size_t N> struct retrieveIDField_t<false, N>
			{ template<typename T> static auto value(const T &) -> typestring<>; };
		template<size_t N, typename field> using retrieveIDField =
			decltype(retrieveIDField_t<isPrimaryKey(field{}), N>::value(field{}));

		template<size_t, typename...> struct retrieveIDFields_t;
		template<size_t N, typename... fields> using retrieveIDFields_ =
			typename retrieveIDFields_t<N, fields...>::value;
		template<size_t N, typename field, typename... fields>
			struct retrieveIDFields_t<N, field, fields...>
		{
			using value = tycat<
				retrieveIDField<N, field>,
				retrieveIDFields_<N - (isPrimaryKey(field{}) ? 1 : 0), fields...>
			>;
		};
		template<> struct retrieveIDFields_t<0> { using value = typestring<>; };
		template<typename... fields> using retrieveIDFields = retrieveIDFields_<countPrimary<fields...>::count, fields...>;

		// Intermediary type calculation function handling conversion of a field into a form suitable for an INSERT query
		template<size_t N, typename fieldName, typename T> auto insertAllField_(const type_t<fieldName, T> &) ->
			typename fieldName_t<N, type_t<fieldName, T>>::value;
		// Alias for the above to make it easier to use.
		template<size_t N, typename T> using insertAllField = decltype(insertAllField_<N>(T{}));
		// Constructs a list of fields suitable for use in an INSERT query
		template<size_t, typename...> struct insertAllList_t;
		// Alias for the above to make it easier to use.
		template<typename... fields> using insertAllList = typename insertAllList_t<sizeof...(fields), fields...>::value;
		// Primary specialisation generates the list
		template<size_t N, typename field, typename... fields> struct insertAllList_t<N, field, fields...>
			{ using value = tycat<insertAllField<N, field>, insertAllList<fields...>>; };
		template<> struct insertAllList_t<0> { using value = typestring<>; };

		// Intermediary container type for handling conversion of a field into a form suitable for an UPDATE query
		template<size_t idx, size_t N> struct updateField_t
		{
			template<typename fieldName, typename T> static auto value(const type_t<fieldName, T> &) ->
				tycat<typename fieldName_t<1, type_t<fieldName, T>>::value, ts(" = "), placeholder<1, idx>, comma<N>>;
			template<typename T> static auto value(const primary_t<T> &) -> typestring<>;
		};
		// Alias for the above container type to make it easier to use.
		template<size_t idx, size_t N, typename T> using updateField = decltype(updateField_t<idx, N>::value(T{}));

		// Constructs a list of fields suitable for use in an UPDATE query
		template<size_t, size_t, typename...> struct updateList_t;
		// Alias for updateList_t to make it easier to use.
		template<size_t idx, typename... fields> using updateList =
			typename updateList_t<idx, sizeof...(fields), fields...>::value;
		// Primary specialisation generates the list
		template<size_t idx, size_t N, typename field, typename... fields> struct updateList_t<idx, N, field, fields...>
			{ using value = tycat<updateField<idx, N, field>, updateList<idx + 1, fields...>>; };
		template<size_t idx> struct updateList_t<idx, 0> { using value = typestring<>; };

		template<size_t idx, size_t N, typename field, typename... fields> struct idField_t
			{ using value = typename idField_t<idx, N - 1, fields...>::value; };
		template<size_t idx, typename field, typename... fields> struct idField_t<idx, 0, field, fields...>
			{ using value = updateList<idx, toType<field>>; };
		template<size_t idx, typename... fields> using idField =
			typename idField_t<idx, primaryIndex_t<fields...>::index, fields...>::value;

		template<bool, size_t idx, size_t N, typename field> struct maybeIDField_t
			{ using value = tycat<idField<idx, field>, and_<N>>; };
		template<size_t idx, size_t N, typename field> struct maybeIDField_t<false, idx, N, field>
			{ using value = typestring<>; };
		template<size_t count, size_t N, typename field> using maybeIDField =
			typename maybeIDField_t<isPrimaryKey(field{}), count - N + 1, N, field>::value;

		template<size_t, size_t, typename...> struct matchIDFields_t;
		template<size_t count, size_t N, typename... fields> using matchIDFields_ =
			typename matchIDFields_t<count, N, fields...>::value;
		template<size_t count, size_t N, typename field, typename... fields>
			struct matchIDFields_t<count, N, field, fields...>
		{
			using value = tycat<
				maybeIDField<count, N, field>,
				matchIDFields_<count, N - (isPrimaryKey(field{}) ? 1 : 0), fields...>
			>;
		};
		template<size_t count> struct matchIDFields_t<count, 0> { using value = typestring<>; };
		template<size_t count, typename... fields> using matchIDFieldsN = matchIDFields_<count, count, fields...>;
		template<typename... fields> using matchIDFields = matchIDFieldsN<countPrimary<fields...>::count, fields...>;

		// This intentionally constructs an empty struct to make the using fail if there is no suitable primary field.
		template<bool, typename...> struct updateWhere_t { };
		template<typename... fields> struct updateWhere_t<true, fields...>
			{ using value = tycat<ts(" WHERE "), matchIDFields<fields...>>; };
		template<typename... fields> using updateWhere =
			typename updateWhere_t<hasPrimaryKey<fields...>(), fields...>::value;

		template<bool, typename tableName, typename... fields> struct update_t { using value = typestring<>; };
		template<typename tableName, typename... fields> using update_ = toString<typename update_t<sizeof...(fields) ==
			countPrimary<fields...>::count, tableName, fields...>::value>;

		template<typename field_t> constexpr fieldLength_t fieldLength(const field_t &) noexcept { return {0, 0}; }
		template<typename fieldName, size_t length> fieldLength_t fieldLength(const unicode_t<fieldName, length> &field) noexcept
			{ return {field.length(), length}; }
		template<typename fieldName> fieldLength_t fieldLength(const unicodeText_t<fieldName> &field) noexcept
			{ return {field.length(), 0}; }

		template<typename> struct createName_t { };
		template<typename fieldName, typename T> struct createName_t<type_t<fieldName, T>>
			{ using value = tycat<doubleQuote<fieldName>, ts(" "), stringType<T>>; };

		template<size_t N, typename field> struct createField_t
		{
			template<typename fieldName, typename T> static auto _name(const type_t<fieldName, T> &) ->
				typename createName_t<type_t<fieldName, T>>::value;
			template<typename T> static auto _name(const autoInc_t<T> &) ->
				tycat<decltype(_name(T{})), ts(" GENERATED BY DEFAULT AS IDENTITY")>;
			template<typename T> static auto _name(const primary_t<T> &) ->
				tycat<decltype(_name(T{})), ts(" PRIMARY KEY")>;
			using name = decltype(_name(field{}));

			static auto value() -> tycat<name, nullable<field::nullable>, comma<N>>;
		};
		template<size_t N, typename T> using createField = decltype(createField_t<N, T>::value());

		template<size_t N, typename field, typename... fields> struct createList_t
			{ using value = tycat<createField<N, field>, typename createList_t<N - 1, fields...>::value>; };
		template<typename field> struct createList_t<1, field> { using value = createField<1, field>; };
		template<typename... fields> using createList = typename createList_t<sizeof...(fields), fields...>::value;

		template<typename tableName, typename... fields> using createTable_ = toString<
			tycat<ts("CREATE TABLE IF NOT EXISTS "), doubleQuote<tableName>, ts(" ("), createList<fields...>, ts(");")>
		>;
		template<typename tableName, typename... fields> using select_ = toString<
			tycat<ts("SELECT "), selectList<fields...>, ts(" FROM "), doubleQuote<tableName>, ts(";")>
		>;
		// tycat<> builds up the query string for inserting the data
		template<typename tableName, typename... fields> using add_ = toString<
			tycat<
				ts("INSERT INTO "),
				doubleQuote<tableName>,
				ts(" ("),
				insertList<fields...>,
				ts(") VALUES ("),
				placeholder<countInsert_t<fields...>::count, 1>,
				ts(") RETURNING "),
				retrieveIDFields<fields...>,
				ts(";")
			>
		>;
		// tycat<> builds up the query string for inserting the data
		template<typename tableName, typename... fields> using addAll_ = toString<
			tycat<
				ts("INSERT INTO "),
				doubleQuote<tableName>,
				ts(" ("),
				insertAllList<fields...>,
				ts(") OVERRIDING SYSTEM VALUE VALUES ("),
				placeholder<sizeof...(fields), 1>,
				ts(");")
			>
		>;
		// This constructs invalid if there is no field marked primary_t<>! This is quite intentional.
		template<typename tableName, typename... fields> struct update_t<false, tableName, fields...>
		{
			using value = tycat<
				ts("UPDATE "),
				doubleQuote<tableName>,
				ts(" SET "),
				updateList<1, fields...>,
				updateWhere<fields...>,
				ts(";")
			>;
		};
		template<typename tableName, typename... fields> using del_ = toString<
			tycat<ts("DELETE FROM "), doubleQuote<tableName>, updateWhere<fields...>, ts(";")>
		>;
		template<typename tableName> using deleteTable_ = toString<
			tycat<ts("DROP TABLE IF EXISTS "), doubleQuote<tableName>, ts(";")>
		>;

		struct session_t final
		{
		private:
			driver::pgSQLClient_t database;

		public:
			session_t() noexcept = default;
			~session_t() noexcept = default;
			session_t(session_t &&session) noexcept : database{std::move(session.database)} { }
			void operator =(session_t &&session) noexcept { database = std::move(session.database); }

			template<typename tableName, typename... fields> bool createTable(const model_t<tableName, fields...> &)
			{
				using create = createTable_<tableName, fields...>;
				auto result{database.query(create::value)};
				return result.valid() && result.successful() && result.numRows() == 0;
			}

			template<typename tableName, typename... fields> bool deleteTable(const model_t<tableName, fields...> &)
			{
				using drop = deleteTable_<tableName>;
				// tycat<> builds up the query for dropping (deleting) the table
				auto result{database.query(drop::value)};
				return result.valid() && result.successful() && result.numRows() == 0;
			}

			bool connect(const char *host, const char *port, const char *user, const char *passwd, const char *db) noexcept
				{ return database.connect(host, port, user, passwd, db); }

			session_t(const session_t &) = delete;
			session_t &operator =(const session_t &) = delete;
		};
	} // namespace pgsql
	using pgsql_t = pgsql::session_t;
} // namespace tmplORM

#endif /*tmplORM_PGSQL_HXX*/
