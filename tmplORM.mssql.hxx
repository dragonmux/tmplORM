#ifndef tmplORM_MSSQL__HXX
#define tmplORM_MSSQL__HXX

#include "tmplORM.hxx"
#include "mssql.hxx"

namespace tmplORM
{
	namespace mssql
	{
		using namespace tmplORM::common;

		using tmplORM::types::type_t;
		using tmplORM::types::unicode_t;
		using tmplORM::types::_dateTime_t;

		using tmplORM::types::autoInc_t;
		using tmplORM::types::primary_t;
		using tmplORM::types::nullable_t;

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
		// Yes really.. this represents a full double (8-bit float) when given no parameters..
		template<> struct stringType_t<double> { using value = ts("FLOAT"); };
		template<> struct stringType_t<char *> { using value = ts("NTEXT"); };
		template<> struct stringType_t<_dateTime_t> { using value = ts("DATETIME"); };
		template<typename T> using stringType = typename stringType_t<T>::value;

		// bindType_t<>

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

		template<size_t N, typename field, typename... fields> struct createList_t
			{ using value = tycat<createList__<N, field>, typename createList_t<N - 1, fields...>::value>; };
		template<typename field> struct createList_t<1, field> { using value = createList__<1, field>; };

		template<typename... fields> using createList = typename createList_t<sizeof...(fields), fields...>::value;

		template<typename tableName, typename... fields> using createTable__ = toString<
			tycat<ts("CREATE TABLE "), bracket<tableName>, ts(" ("), createList<fields...>, ts(");")>
		>;
		template<typename tableName, typename... fields> using select__ = toString<
			tycat<ts("SELECT "), selectList<fields...>, ts(" FROM "), bracket<tableName>, ts(";")>
		>;
		template<typename tableName, typename... fields> using add__ = toString<
			tycat<ts("INSERT INTO "), bracket<tableName>, ts(" ("), insertList<fields...>, ts(") VALUES ("), placeholder<countInsert_t<fields...>::count>, ts(");")>
		>;
		template<typename tableName, typename... fields> using update__ = toString<
			tycat<ts("UPDATE "), bracket<tableName>, ts(" SET "), updateList<fields...>, updateWhere<fields...>, ts(";")>
		>;

		template<typename tableName, typename...> using deleteTable__ = toString<
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
				using create = createTable__<tableName, fields...>;
				create::value;
				return true;
			}

			template<typename T, typename tableName, typename... fields> T select(const model_t<tableName, fields...> &) noexcept
			{
				using select = select__<tableName, fields...>;
				select::value;
				return T();
			}

			template<typename tableName, typename... fields_t> bool add(const model_t<tableName, fields_t...> &model) noexcept
			{
				using insert = add__<tableName, fields_t...>;
				insert::value;
				return true;
			}

			template<typename tableName, typename... fields_t> bool update(const model_t<tableName, fields_t...> &model) noexcept
			{
				using update = update__<tableName, fields_t...>;
				update::value;
				return true;
			}

			template<typename tableName, typename... fields> bool del(const model_t<tableName, fields...> &model) noexcept
				{ return true; }

			template<typename tableName, typename... fields> bool deleteTable(const model_t<tableName, fields...> &model) noexcept
			{
				using deleteTable = deleteTable__<tableName, fields...>;
				deleteTable::value;
				return true;
			}
		};
	}
	using mssql_t = mssql::session_t;
}

#endif /*tmplORM_MSSQL__HXX*/
