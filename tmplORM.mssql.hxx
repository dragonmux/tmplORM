#ifndef tmplORM_MSSQL__HXX
#define tmplORM_MSSQL__HXX

#ifndef UNICODE
#define UNICODE
#include <sql.h>
#include <sqlext.h>
#endif
#include "tmplORM.hxx"
#include "mssql.hxx"

/*!
 * @file
 * @author Rachel Mant
 * @date 2016-2017
 * @brief Defines the MSSQL mapper for the ORM
 */

namespace tmplORM
{
	namespace mssql
	{
		using namespace tmplORM::common;
		using namespace tmplORM::mssql::driver;
		using namespace tmplORM::types::baseTypes;

		using tmplORM::types::type_t;
		using tmplORM::types::unicode_t;
		using tmplORM::types::unicodeText_t;

		using tmplORM::types::primary_t;
		using tmplORM::types::nullable_t;
		using tmplORM::types::autoInc_t;

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
		// Yes really.. this represents a full double (8-byte float) when given no parameters..
		template<> struct stringType_t<double> { using value = ts("FLOAT"); };
		template<> struct stringType_t<char *> { using value = ts("NTEXT"); };
		template<> struct stringType_t<ormDate_t> { using value = ts("DATE"); };
		template<> struct stringType_t<ormDateTime_t> { using value = ts("DATETIME"); };
		template<> struct stringType_t<ormUUID_t> { using value = ts("UNIQUEIDENTIFIER"); };
		template<typename T> using stringType = typename stringType_t<T>::value;

		template<typename> struct bind_t { };
		template<> struct bind_t<int8_t>
			{ constexpr static const uint16_t typeC = SQL_C_STINYINT; constexpr static const int64_t typeODBC = SQL_TINYINT; };
		template<> struct bind_t<int16_t>
			{ constexpr static const uint16_t typeC = SQL_C_SSHORT; constexpr static const int64_t typeODBC = SQL_SMALLINT; };
		template<> struct bind_t<int32_t>
			{ constexpr static const uint16_t typeC = SQL_C_SLONG; constexpr static const int64_t typeODBC = SQL_INTEGER; };
		template<> struct bind_t<int64_t>
			{ constexpr static const uint16_t typeC = SQL_C_SBIGINT; constexpr static const int64_t typeODBC = SQL_BIGINT; };
		//
		template<> struct bind_t<bool>
			{ constexpr static const uint16_t typeC = SQL_C_BIT; constexpr static const int64_t typeODBC = SQL_BIT; };
		template<> struct bind_t<float>
			{ constexpr static const uint16_t typeC = SQL_C_FLOAT; constexpr static const int64_t typeODBC = SQL_REAL; };
		template<> struct bind_t<double>
			{ constexpr static const uint16_t typeC = SQL_C_DOUBLE; constexpr static const int64_t typeODBC = SQL_FLOAT; };
		template<> struct bind_t<char *>
			{ constexpr static const uint16_t typeC = SQL_C_CHAR; constexpr static const int64_t typeODBC = SQL_VARCHAR; };
		template<> struct bind_t<const char *>
			{ constexpr static const uint16_t typeC = SQL_C_CHAR; constexpr static const int64_t typeODBC = SQL_VARCHAR; };
		template<> struct bind_t<ormDate_t>
			{ constexpr static const uint16_t typeC = SQL_C_DATE; constexpr static const int64_t typeODBC = SQL_DATE; };
		template<> struct bind_t<ormDateTime_t>
			{ constexpr static const uint16_t typeC = SQL_C_TIMESTAMP; constexpr static const int64_t typeODBC = SQL_DATETIME; };
		template<> struct bind_t<ormUUID_t>
			{ constexpr static const uint16_t typeC = SQL_C_GUID; constexpr static const int64_t typeODBC = SQL_GUID; };
		/*template<> struct bind_t<nullptr_t>
			{ constexpr static const uint16_t typeC = SQL_C_DEFAULT; constexpr static const int64_t typeODBC = SQL_TYPE_NULL; };*/

		namespace driver
		{
			// bindValue()
			// tSQLQuery_t::bind() * 2
		}

		template<typename name> using bracket = tycat<ts("["), name, ts("]")>;

		template<size_t, typename> struct fieldName_t { };
		template<size_t N, typename fieldName, typename T> struct fieldName_t<N, type_t<fieldName, T>>
			{ using value = tycat<bracket<fieldName>, comma<N>>; };

		template<typename> struct createName_t { };
		template<typename fieldName, typename T> struct createName_t<type_t<fieldName, T>>
			{ using value = tycat<bracket<fieldName>, ts(" "), stringType<T>>; };
		template<typename fieldName, uint32_t length> struct createName_t<unicode_t<fieldName, length>>
			{ using value = tycat<bracket<fieldName>, ts(" NVARCHAR("), toTypestring<length>, ts(")")>; };

#include "tmplORM.common.hxx"

		template<size_t N, typename field> struct createList__t
		{
			template<typename fieldName, typename T> static auto _name(const type_t<fieldName, T> &) ->
				typename createName_t<type_t<fieldName, T>>::value;
			template<typename fieldName, uint32_t length> static auto _name(const unicode_t<fieldName, length> &) ->
				typename createName_t<unicode_t<fieldName, length>>::value;
			template<typename T> static auto _name(const primary_t<T> &) -> tycat<decltype(_name(T())), ts(" PRIMARY KEY")>;
			template<typename T> static auto _name(const autoInc_t<T> &) -> tycat<decltype(_name(T())), ts(" IDENTITY")>;
			using name = decltype(_name(field()));

			static auto value() -> tycat<name, nullable<field::nullable>, comma<N>>;
		};
		template<size_t N, typename T> using createList__ = decltype(createList__t<N, T>::value());

		// Constructs a list of fields suitable for use in a CREATE query
		template<size_t, typename...> struct createList_t;
		// Alias to make createList_t easier to use
		template<typename... fields> using createList = typename createList_t<sizeof...(fields), fields...>::value;
		// Primary specialisation generates the list
		template<size_t N, typename field, typename... fields> struct createList_t<N, field, fields...>
			{ using value = tycat<createList__<N, field>, typename createList_t<N - 1, fields...>::value>; };
		template<> struct createList_t<0> { using value = typestring<>; };

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
		// Alias to make otuputInsert_t easier to use
		template<typename... fields> using outputInsert = typename outputInsert_t<hasAutoInc<fields...>(), fields...>::value;

		template<typename tableName, typename... fields> using createTable_ = toString<
			tycat<ts("CREATE TABLE "), bracket<tableName>, ts(" ("), createList<fields...>, ts(");")>
		>;
		template<typename tableName, typename... fields> using select_ = toString<
			tycat<ts("SELECT "), selectList<fields...>, ts(" FROM "), bracket<tableName>, ts(";")>
		>;
		template<typename tableName, typename... fields> using add_ = toString<
			tycat<ts("INSERT INTO "), bracket<tableName>, ts(" ("), insertList<fields...>, ts(")"), outputInsert<fields...>, ts(" VALUES ("), placeholder<countInsert_t<fields...>::count>, ts(");")>
		>;
		template<typename tableName, typename... fields> struct update_t<false, tableName, fields...>
			{ using value = tycat<ts("UPDATE "), bracket<tableName>, ts(" SET "), updateList<fields...>, updateWhere<fields...>, ts(";")>; };
		template<typename tableName, typename... fields> using del_ = toString<
			tycat<ts("DELETE FROM "), bracket<tableName>, updateWhere<fields...>, ts(";")>
		>;
		template<typename tableName> using deleteTable_ = toString<
			tycat<ts("DROP TABLE "), bracket<tableName>, ts(";")>
		>;

		struct session_t
		{
		private:
			driver::tSQLClient_t database;

		public:
			session_t() noexcept : database() { }
			~session_t() noexcept { }

			template<typename tableName, typename... fields> bool createTable(const model_t<tableName, fields...> &) noexcept
			{
				using create = createTable_<tableName, fields...>;
				return database.query(create::value).valid();
			}

			template<typename T, typename tableName, typename... fields_t> T select(const model_t<tableName, fields_t...> &) noexcept
			{
				using select = select_<tableName, fields_t...>;
				select::value;
				return T();
			}

			template<typename tableName, typename... fields_t> bool add(const model_t<tableName, fields_t...> &model) noexcept
			{
				using insert = add_<tableName, fields_t...>;
				tSQLQuery_t query(database.prepare(insert::value, countInsert_t<fields_t...>::count));
				bindInsert<fields_t...>::bind(model.fields(), query);
				tSQLResult_t result(query.execute());
				if (result.valid())
				{
					if (hasAutoInc<fields_t...>())
						getAutoInc(model) = result[0];
					return true;
				}
				return false;
			}

			template<typename tableName, typename... fields_t> bool update(const model_t<tableName, fields_t...> &model) noexcept
			{
				using update = update_<tableName, fields_t...>;
				if (std::is_same<update, toString<typestring<>>>::value)
					return false;
				tSQLQuery_t query(database.prepare(update::value, sizeof...(fields_t)));
				bindUpdate<fields_t...>::bind(model.fields(), query);
				return query.execute().valid();
			}

			template<typename tableName, typename... fields_t> bool del(const model_t<tableName, fields_t...> &model) noexcept
			{
				using del = del_<tableName, fields_t...>;
				tSQLQuery_t query(database.prepare(del::value, countPrimary<fields_t...>::count));
				bindDelete<fields_t...>::bind(model.fields(), query);
				return query.execute().valid();
			}

			template<typename tableName, typename... fields> bool deleteTable(const model_t<tableName, fields...> &) noexcept
			{
				using deleteTable = deleteTable_<tableName>;
				return database.query(deleteTable::value).valid();
			}
		};
	}
	using mssql_t = mssql::session_t;
}

#endif /*tmplORM_MSSQL__HXX*/
