#ifndef tmplORM_MYSQL__HXX
#define tmplORM_MYSQL__HXX

#include "tmplORM.hxx"
#include "mysql.hxx"
#include <type_traits>

namespace tmplORM
{
	namespace mysql
	{
		using namespace tmplORM::common;
		using namespace tmplORM::mysql::driver;

		using tmplORM::types::type_t;
		using tmplORM::types::unicode_t;
		using tmplORM::types::_dateTime_t;

		using tmplORM::types::autoInc_t;
		using tmplORM::types::primary_t;
		using tmplORM::types::nullable_t;

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
		template<> struct stringType_t<_dateTime_t> { using value = ts("DATETIME"); };
		template<typename T> using stringType = typename stringType_t<T>::value;

		template<typename> struct bind_t { /*constexpr static const mySQLFieldType_t value = MYSQL_TYPE_NULL;*/ };
		template<> struct bind_t<int8_t> { constexpr static const mySQLFieldType_t value = MYSQL_TYPE_TINY; };
		template<> struct bind_t<int16_t> { constexpr static const mySQLFieldType_t value = MYSQL_TYPE_SHORT; };
		template<> struct bind_t<int32_t> { constexpr static const mySQLFieldType_t value = MYSQL_TYPE_LONG; };
		template<> struct bind_t<int64_t> { constexpr static const mySQLFieldType_t value = MYSQL_TYPE_LONGLONG; };
		//template<> struct bind_t<bool> { constexpr static const mySQLFieldType_t value = MYSQL_TYPE_
		template<> struct bind_t<float> { constexpr static const mySQLFieldType_t value = MYSQL_TYPE_FLOAT; };
		template<> struct bind_t<double> { constexpr static const mySQLFieldType_t value = MYSQL_TYPE_DOUBLE; };
		template<> struct bind_t<char *> { constexpr static const mySQLFieldType_t value = MYSQL_TYPE_STRING; };
		template<> struct bind_t<void *> { constexpr static const mySQLFieldType_t value = MYSQL_TYPE_BLOB; };
		template<> struct bind_t<_dateTime_t> { constexpr static const mySQLFieldType_t value = MYSQL_TYPE_DATETIME; };
		template<> struct bind_t<nullptr_t> { constexpr static const mySQLFieldType_t value = MYSQL_TYPE_NULL; };
		template<typename T, bool = isNumeric<T>::value> struct bindType_t { constexpr static const mySQLFieldType_t value = bind_t<T>::value; };
		template<typename T> struct bindType_t<T, true> { constexpr static const mySQLFieldType_t value = bind_t<typename std::make_signed<T>::type>::value; };

		namespace driver
		{
			static const bool isNotNull = false;
			static const bool isNull = true;

			template<typename T> typename std::enable_if<!isNumeric<T>::value>::type
				bindValue(MYSQL_BIND &param, const T &value) noexcept
			{
				param.is_null = &isNotNull;
				param.is_unsigned = false;
			}

			template<typename T> typename std::enable_if<isNumeric<T>::value>::type
				bindValue(MYSQL_BIND &param, const T &value) noexcept
			{
				param.is_null = &isNotNull;
				param.is_unsigned = std::is_unsigned<T>::value;
			}

			template<typename T> void mySQLPreparedQuery_t::bind(const size_t index, const T &value) noexcept
			{
				if (index >= numParams)
					return;
				MYSQL_BIND &param = params[index];
				param.buffer_type = bindType_t<T>::value;
				param.buffer = &value;
				bindValue(param, value);
			}

			/* special handling for nullable_t<>.. 'cause that's a fun special-case */
			template<typename T> void mySQLPreparedQuery_t::bind(const size_t index, const nullptr_t) noexcept
			{
				if (index >= numParams)
					return;
				MYSQL_BIND &param = params[index];
				// T can be nullptr_t, in which case this becomes MYSQL_TYPE_NULL anyway.. so.. we don't end up caring about what we put in any other field nominally..
				param.buffer_type = bindType_t<T>::value;
				param.is_null = &isNull;
			}
		}

		template<typename name> using backtick = tycat<ts("`"), name, ts("`")>;

		template<size_t, typename> struct fieldName_t { };
		template<size_t N, typename fieldName, typename T> struct fieldName_t<N, type_t<fieldName, T>>
			{ using value = tycat<backtick<fieldName>, comma<N>>; };

		template<typename fieldName, typename T> auto toFieldName(const type_t<fieldName, T> &) -> fieldName;

		template<typename> struct createName_t { };
		template<typename fieldName, typename T> struct createName_t<type_t<fieldName, T>>
			{ using value = tycat<backtick<fieldName>, ts(" "), stringType<T>>; };
		template<typename fieldName, uint32_t length> struct createName_t<unicode_t<fieldName, length>>
			{ using value = tycat<backtick<fieldName>, ts(" VARCHAR("), toTypestring<length>, ts(")")>; };

#include "tmplORM.common.hxx"

		template<size_t N, typename field> struct createList__t
		{
			template<typename fieldName, typename T> static auto _name(const type_t<fieldName, T> &) ->
				typename createName_t<type_t<fieldName, T>>::value;
			template<typename fieldName, uint32_t length> static auto _name(const unicode_t<fieldName, length> &) ->
				typename createName_t<unicode_t<fieldName, length>>::value;
			template<typename T> static auto _name(const primary_t<T> &) -> tycat<decltype(_name(T())), ts(" PRIMARY KEY")>;
			template<typename T> static auto _name(const autoInc_t<T> &) -> tycat<decltype(_name(T())), ts(" AUTO_INCREMENT")>;
			using name = decltype(_name(field()));

			static auto value() -> tycat<name, nullable<field::nullable>, comma<N>>;
		};
		template<size_t N, typename T> using createList__ = decltype(createList__t<N, T>::value());

		template<size_t N, typename field, typename... fields> struct createList_t
			{ using value = tycat<createList__<N, field>, typename createList_t<N - 1, fields...>::value>; };
		template<typename field> struct createList_t<1, field> { using value = createList__<1, field>; };

		template<size_t N> struct updateList__t
		{
			template<typename fieldName, typename T> static auto value(const type_t<fieldName, T> &) ->
				tycat<typename fieldName_t<1, type_t<fieldName, T>>::value, ts(" = "), placeholder<1>, comma<N>>;
			template<typename T> static auto value(const autoInc_t<T> &) -> typestring<>;
		};
		template<size_t N, typename T> using updateList__ = decltype(updateList__t<N>::value(T()));

		template<size_t N, typename field, typename... fields> struct updateList_t
			{ using value = tycat<updateList__<N, field>, typename updateList_t<N - 1, fields...>::value>; };
		template<typename field> struct updateList_t<1, field> { using value = updateList__<1, field>; };

		template<typename... fields> using createList = typename createList_t<sizeof...(fields), fields...>::value;
		template<typename... fields> using selectList = typename selectList_t<sizeof...(fields), fields...>::value;
		template<typename... fields> using insertList = typename insertList_t<sizeof...(fields), fields...>::value;
		template<typename... fields> using updateList = typename updateList_t<sizeof...(fields), fields...>::value;

		template<size_t index, size_t bindIndex, typename... fields_t> struct bindInsert_t
		{
			template<size_t N, typename field, typename... fields> struct field_t { using type = typename field_t<N - 1, fields...>::type; };
			template<typename field, typename... fields> struct field_t<0, field, fields...> { using type = field; };
			// This should make the case where N >= sizeof...(fields) invalid.
			template<size_t N, typename field> struct field_t<N, field> { };
			template<size_t N, typename... fields> using field = typename field_t<N, fields...>::type;

			template<typename std::enable_if<!field<index, fields_t...>::nullable, void *>::type = nullptr>
				static void bind(const std::tuple<fields_t...> &fields, mySQLPreparedQuery_t &query) noexcept
			{
				bindInsert_t<index - 1, bindIndex - 1, fields_t...>::bind(fields, query);
				const auto &field = std::get<index>(fields);
				query.bind(bindIndex, field.value());
			}

			template<typename std::enable_if<field<index, fields_t...>::nullable, void *>::type = nullptr>
				static void bind(const std::tuple<fields_t...> &fields, mySQLPreparedQuery_t &query) noexcept
			{
				bindInsert_t<index - 1, bindIndex - 1, fields_t...>::bind(fields, query);
				const auto &field = std::get<index>(fields);
				if (field.isNull())
					query.bind<typename decltype(field)::type>(bindIndex, nullptr);
				else
					query.bind(bindIndex, field.value());
			}
		};

		template<size_t index, typename... fields_t> struct bindInsert_t<index, 0, fields_t...>
			{ static void bind(const std::tuple<fields_t...> &, mySQLPreparedQuery_t &) { } };
		template<typename... fields_t> using bindInsert = bindInsert_t<sizeof...(fields_t) - 1, countInsert_t<fields_t...>::count - 1, fields_t...>;

		template<size_t N, typename field, typename... fields> struct idField_t
			{ using type = typename idField_t<N - 1, fields...>::type; };
		template<typename field, typename... fields> struct idField_t<0, field, fields...>
			{ using type = updateList<type_t<decltype(toFieldName(field())), bool>>; };

		template<typename... fields> using idField = typename idField_t<autoIncIndex_t<fields...>::index, fields...>::type;

		template<bool, typename... fields> struct updateWhere_t { using value = typestring<>; };
		template<typename... fields> struct updateWhere_t<true, fields...>
			{ using value = tycat<ts(" WHERE "), idField<fields...>>; };
		template<typename... fields> using updateWhere = typename updateWhere_t<hasAutoInc<fields...>(), fields...>::value;

		template<size_t index, size_t bindIndex, typename... fields_t> struct bindUpdate_t
		{
			static void bind(const std::tuple<fields_t...> &fields, mySQLPreparedQuery_t &query) noexcept
			{
				const auto &field = std::get<index>(fields);
				query.bind(bindIndex, field.value());
			}
		};
		template<typename... fields> using bindUpdate = bindUpdate_t<autoIncIndex_t<fields...>::index, countInsert_t<fields...>::count, fields...>;

		template<typename tableName, typename... fields> using createTable__ = toString<
			tycat<ts("CREATE TABLE "), backtick<tableName>, ts(" ("), createList<fields...>, ts(");")>
		>;
		template<typename tableName, typename... fields> bool createTable_(const model_t<tableName, fields...> &) noexcept
		{
			using create = createTable__<tableName, fields...>;
			return database.query(create::value);
		}
		template<typename... models> bool createTable() noexcept { return collect(createTable_(models())...); }

		template<typename tableName, typename... fields> using select__ = toString<
			tycat<ts("SELECT "), selectList<fields...>, ts(" FROM "), backtick<tableName>, ts(";")>
		>;
		template<typename T, typename tableName, typename... fields> T select_(const model_t<tableName, fields...> &) noexcept
		{
			using select = select__<tableName, fields...>;
			database.query(select::value);
			return T();
		}
		template<typename model> model select() noexcept { return select_<model>(model()); }

		template<typename tableName, typename... fields> using add__ = toString<
			tycat<ts("INSERT INTO "), backtick<tableName>, ts(" ("), insertList<fields...>, ts(") VALUES ("), placeholder<countInsert_t<fields...>::count>, ts(");")>
		>;
		template<typename tableName, typename... fields_t> bool add_(const model_t<tableName, fields_t...> &model) noexcept
		{
			using insert = add__<tableName, fields_t...>;
			mySQLPreparedQuery_t query = database.prepare(insert::value, countInsert_t<fields_t...>::count);
			bindInsert<fields_t...>::bind(model.fields(), query);
			if (query.execute())
			{
				if (hasAutoInc<fields_t...>())
					getAutoInc(model) = query.rowID();
				return true;
			}
			return false;
		}
		template<typename... models_t> bool add(const models_t &...models) noexcept { return collect(add_(models)...); }

		template<typename tableName, typename... fields> using update__ = toString<
			tycat<ts("UPDATE "), backtick<tableName>, ts(" SET "), updateList<fields...>, updateWhere<fields...>, ts(";")>
		>;
		template<typename tableName, typename... fields_t> bool update_(const model_t<tableName, fields_t...> &model) noexcept
		{
			using update = update__<tableName, fields_t...>;
			mySQLPreparedQuery_t query = database.prepare(update::value, countInsert_t<fields_t...>::count);
			bindInsert<fields_t...>::bind(model.fields(), query);
			// This pulls back the ID field and binds it last so it tags to the WHERE clause for this query.
			bindUpdate<fields_t...>::bind(model.fields(), query);
			// TODO: perform binds here.
			return query.execute();
		}
		template<typename... models_t> bool update(const models_t &...models) noexcept { return collect(update_(models)...); }

		template<typename tableName, typename... fields> using del__ = toString<
			tycat<ts("DELETE * FROM "), backtick<tableName>, ts(";")>
		>;
		template<typename tableName, typename... fields> bool del_(const model_t<tableName, fields...> &model) noexcept
		{
			using del = del__<tableName, fields...>;
			return database.query(del::value);
		}
		template<typename... models_t> bool del(const models_t &...models) noexcept { return collect(del_(models)...); }

		template<typename tableName> using deleteTable__ = toString<
			tycat<ts("DROP TABLE "), backtick<tableName>, ts(";")>
		>;
		template<typename tableName, typename... fields> bool deleteTable_(const model_t<tableName, fields...> &) noexcept
		{
			using deleteTable = deleteTable__<tableName>;
			return database.query(deleteTable::value);
		}
		template<typename... models> bool deleteTable() noexcept { return collect(deleteTable_(models())...); }
	}
}

#endif /*tmplORM_MYSQL__HXX*/
