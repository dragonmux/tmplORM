#ifndef tmplORM_MSSQL__HXX
#define tmplORM_MSSQL__HXX

#include "tmplORM.hxx"
#include "mssql.hxx"

namespace tmplORM
{
	namespace mssql
	{
		using namespace tmplORM::common;
		using namespace tmplORM::mssql::driver;

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

		template<typename tableName, typename... fields> using createTable__ = toString<
			tycat<ts("CREATE TABLE "), bracket<tableName>, ts(" ("), createList<fields...>, ts(");")>
		>;
		template<typename tableName, typename... fields> using select__ = toString<
			tycat<ts("SELECT "), selectList<fields...>, ts(" FROM "), bracket<tableName>, ts(";")>
		>;
		template<typename tableName, typename... fields> using add__ = toString<
			tycat<ts("INSERT INTO "), bracket<tableName>, ts(" ("), insertList<fields...>, ts(")"), outputInsert<fields...>, ts(" VALUES ("), placeholder<countInsert_t<fields...>::count>, ts(");")>
		>;
		template<typename tableName, typename... fields> struct update_t<false, tableName, fields...>
			{ using value = tycat<ts("UPDATE "), bracket<tableName>, ts(" SET "), updateList<fields...>, updateWhere<fields...>, ts(";")>; };
		template<typename tableName, typename... fields> using del__ = toString<
			tycat<ts("DELETE FROM "), bracket<tableName>, updateWhere<fields...>, ts(";")>
		>;
		template<typename tableName> using deleteTable__ = toString<
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
				return database.query(create::value).valid();
			}

			template<typename T, typename tableName, typename... fields_t> T select(const model_t<tableName, fields_t...> &) noexcept
			{
				using select = select__<tableName, fields_t...>;
				select::value;
				return T();
			}

			template<typename tableName, typename... fields_t> bool add(const model_t<tableName, fields_t...> &model) noexcept
			{
				using insert = add__<tableName, fields_t...>;
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
				using del = del__<tableName, fields_t...>;
				tSQLQuery_t query(database.prepare(del::value, countPrimary<fields_t...>::count));
				bindDelete<fields_t...>::bind(model.fields(), query);
				return query.execute().valid();
			}

			template<typename tableName, typename... fields> bool deleteTable(const model_t<tableName, fields...> &) noexcept
			{
				using deleteTable = deleteTable__<tableName>;
				return database.query(deleteTable::value).valid();
			}
		};
	}
	using mssql_t = mssql::session_t;
}

#endif /*tmplORM_MSSQL__HXX*/
