#ifndef tmplORM_MYSQL__HXX
#define tmplORM_MYSQL__HXX

#include "tmplORM.hxx"

namespace tmplORM
{
	namespace mysql
	{
		using tmplORM::types::type_t;
		using tmplORM::types::autoInc_t;
		using tmplORM::types::primary_t;
		using tmplORM::types::nullable_t;
		using tmplORM::model_t;

		template<typename> struct stringType_t { };
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
		template<typename T> using stringType = typename stringType_t<T>::value;

		template<typename name> using backtick = tycat<ts("`"), name, ts("`")>;

		template<size_t N> struct comma_t { using value = ts(", "); };
		template<> struct comma_t<1> { using value = typestring<>; };
		template<size_t N> using comma = typename comma_t<N>::value;

		template<size_t, typename> struct fieldName_t { };
		template<size_t N, typename fieldName, typename T> struct fieldName_t<N, type_t<fieldName, T>>
			{ using value = tycat<backtick<fieldName>, comma<N>>; };

		template<typename> struct createName_t { };
		template<typename fieldName, typename T> struct createName_t<type_t<fieldName, T>>
			{ using value = tycat<backtick<fieldName>, ts(" "), stringType<T>>; };

		template<size_t N> struct selectList__t
		{
			template<typename fieldName, typename T> constexpr static auto value(const type_t<fieldName, T> &) ->
				typename fieldName_t<N, type_t<fieldName, T>>::value;
		};
		template<size_t N, typename T> using selectList__ = decltype(selectList__t<N>::value(T()));

		template<size_t N, typename field, typename... fields> struct selectList_t
			{ using value = tycat<selectList__<N, field>, typename selectList_t<N - 1, fields...>::value>; };
		template<typename field> struct selectList_t<1, field> { using value = selectList__<1, field>; };

		template<size_t N> struct insertList__t
		{
			template<typename fieldName, typename T> constexpr static auto value(const type_t<fieldName, T> &) ->
				typename fieldName_t<N, type_t<fieldName, T>>::value;
			template<typename T> constexpr auto value(const primary_t<T> &) -> typestring<>;
		};
		template<size_t N, typename T> using insertList__ = decltype(selectList__t<N>::value(T()));

		template<size_t N, typename field, typename... fields> struct insertList_t
			{ using value = tycat<insertList__<N, field>, typename insertList_t<N - 1, fields...>::value>; };
		template<typename field> struct insertList_t<1, field> { using value = insertList__<1, field>; };

		template<bool isNull> struct nullable__t { using value = ts(" NOT NULL"); };
		template<> struct nullable__t<true> { using value = ts(" NULL"); };
		template<bool isNull> using nullable = typename nullable__t<isNull>::value;

		template<size_t N, typename field> struct createList__t
		{
			template<typename fieldName, typename T> constexpr static auto _name(const type_t<fieldName, T> &) ->
				typename createName_t<type_t<fieldName, T>>::value;
			template<typename T> constexpr static auto _name(const primary_t<T> &) -> tycat<decltype(_name(T())), ts(" PRIMARY KEY")>;
			using name = decltype(_name(field()));

			constexpr static auto value() -> tycat<name, nullable<field::nullable>, comma<N>>;
		};
		template<size_t N, typename T> using createList__ = decltype(createList__t<N, T>::value());

		template<size_t N, typename field, typename... fields> struct createList_t
			{ using value = tycat<createList__<N, field>, typename createList_t<N - 1, fields...>::value>; };
		template<typename field> struct createList_t<1, field> { using value = createList__<1, field>; };

		template<typename... fields> using createList = typename createList_t<sizeof...(fields), fields...>::value;
		template<typename... fields> using selectList = typename selectList_t<sizeof...(fields), fields...>::value;
		template<typename... fields> using insertList = typename insertList_t<sizeof...(fields), fields...>::value;

		template<typename tableName, typename... fields> using createTable__ = toString<tycat<ts("CREATE TABLE "), backtick<tableName>,
			ts("("), createList<fields...>, ts(");")>>;
		template<typename tableName, typename... fields> bool createTable_(const model_t<tableName, fields...> &) noexcept
		{
			using create = createTable__<tableName, fields...>;
			return true;
		}
		template<typename... models> bool createTable() noexcept { return collect(createTable_(models())...); }

		template<typename tableName, typename... fields> using select__ = toString<tycat<ts("SELECT "), selectList<fields...>,
			ts(" FROM "), backtick<tableName>, ts(";")>>;
		template<typename tableName, typename... fields> bool select_(const model_t<tableName, fields...> &) noexcept
		{
			using select = select__<tableName, fields...>;
			return true;
		}
		template<typename... models> bool select() noexcept { return collect(select_(models())...); }

		template<typename tableName, typename... fields> using add__ = toString<tycat<ts("INSERT INTO "), backtick<tableName>,
			ts(" ("), insertList<tableName>, ts(") VALUES ("), ts("..."), ts(");")>>;
		template<typename tableName, typename... fields> bool add_(const model_t<tableName, fields...> &model) noexcept
		{
			using insert = add__<tableName, fields...>;
			return true;
		}
		template<typename... models_t> bool add(const models_t &...models) noexcept { return collect(add_(models)...); }
	};
};

#endif /*tmplORM_MYSQL__HXX*/
