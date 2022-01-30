#ifndef tmplORM_PGSQL_HXX
#define tmplORM_PGSQL_HXX

#include "tmplORM.hxx"

namespace tmplORM
{
	namespace pgsql
	{
		using namespace tmplORM::common;
		using namespace tmplORM::types::baseTypes;

		using tmplORM::types::type_t;

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

		template<typename> struct bind_t { };

		namespace driver
		{
		} // namespace driver

		template<typename name> using doubleQuote = tycat<ts("\""), name, ts("\"")>;

		template<size_t, typename> struct fieldName_t { };
		template<size_t N, typename fieldName, typename T> struct fieldName_t<N, type_t<fieldName, T>>
			{ using value = tycat<doubleQuote<fieldName>, comma<N>>; };

#include "tmplORM.common.hxx"

		template<typename> struct createName_t { };
		template<typename fieldName, typename T> struct createName_t<type_t<fieldName, T>>
			{ using value = tycat<doubleQuote<fieldName>, ts(" "), stringType<T>>; };

		template<size_t N, typename field> struct createField_t
		{
			template<typename fieldName, typename T> static auto _name(const type_t<fieldName, T> &) ->
				typename createName_t<type_t<fieldName, T>>::value;
			template<typename T> static auto _name(const autoInc_t<T> &) ->
				tycat<decltype(_name(T{})), ts(" GENERATE BY DEFAULT AS IDENTITY")>;
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
		template<typename tableName> using deleteTable_ = toString<
			tycat<ts("DROP TABLE IF EXISTS "), doubleQuote<tableName>, ts(";")>
		>;
	} // namespace pgsql
} // namespace tmplORM

#endif /*tmplORM_PGSQL_HXX*/
