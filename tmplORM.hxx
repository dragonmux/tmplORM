#ifndef tmplORM__HXX
#define tmplORM__HXX

#include <cstdint>
#include <tuple>
#include <chrono>
#include <type_traits>
#include "typestring/typestring.hh"
#include "fixedVector.hxx"

#define ts(x) typestring_is(x)
#define ts_(x) ts(x)()

namespace tmplORM
{
	using namespace irqus;
	using std::nullptr_t;

	namespace common
	{
		template<typename... fields> constexpr bool hasPrimaryKey() noexcept;
		template<typename fieldName, typename field, typename... fields> struct fieldIndex_t;
		template<size_t N, typename field, typename... fields> struct fieldType_t;

		template<typename fieldName, typename... fields> using fieldIndex = fieldIndex_t<fieldName, fields...>;
		template<typename fieldName, typename... fields> using fieldType = typename fieldType_t<fieldIndex<fieldName, fields...>::index, fields...>::type;

		template<typename T> struct isBoolean : std::false_type { };
		template<> struct isBoolean<bool> : std::true_type { };

		template<typename T> struct isNumeric : std::integral_constant<bool, std::is_integral<T>::value && !isBoolean<T>::value> { };
	}
	using tmplORM::common::fieldIndex;
	using tmplORM::common::fieldType;

	template<typename... Fields> struct fields_t
	{
	protected:
		constexpr static size_t N = sizeof...(Fields);
		std::tuple<Fields...> _fields;

		constexpr fields_t() noexcept : _fields{} { }
		constexpr fields_t(Fields &&...fields) noexcept : _fields{fields...} { }

	public:
		const std::tuple<Fields...> &fields() const noexcept { return _fields; }
		template<char... C> auto operator [](const typestring<C...> &) noexcept -> fieldType<typestring<C...>, Fields...> &
			{ return std::get<fieldIndex<typestring<C...>, Fields...>::index>(_fields); }
	};

	template<typename _tableName, typename... Fields> struct model_t : public fields_t<Fields...>
	{
	public:
		constexpr model_t() noexcept : fields_t<Fields...>{} {}
		constexpr model_t(Fields... fields) noexcept : fields_t<Fields...>{fields...} { }

		static_assert(tmplORM::common::hasPrimaryKey<Fields...>(), "Model must have a primary key!");
		const char *tableName() const noexcept { return _tableName::data(); }
		constexpr static size_t N = fields_t<Fields...>::N;

		// create(); - Creates the table
		// add(); - CRUD Create
		// update(); - CRUD Update
		// delete(); - CRUD Delete
	};

	namespace types
	{
		using common::isNumeric;

		template<typename _fieldName, typename T> struct type_t
		{
			T _value;
			bool _modified;
			typedef T type;

			constexpr type_t() noexcept : _value(), _modified(false) { }
			constexpr type_t(const T &value) noexcept : _value(value), _modified(false) { }

			constexpr const char *fieldName() const noexcept { return _fieldName::data(); }
			const T value() const noexcept { return _value; }
			// Make the type behave like it's contained type..
			operator T() const noexcept { return _value; }
			bool modified() const noexcept { return _modified; }
			constexpr static bool nullable = false;

			void value(const T &newValue) noexcept
			{
				_value = newValue;
				_modified = true;
			}

			// create(); - CREATE TABLE definiton of the field
		};

		// Tag type to mark auto increment fields with
		template<typename T> struct autoInc_t : public T { typedef typename T::type type; };

		// Tag type to mark the primary key field with
		template<typename T> struct primary_t : public T { typedef typename T::type type; };

		// Tag type to give a field a program name different to the field name in the database
		template<typename, typename T> struct alias_t : public T { typedef typename T::type type; };

		// Tag type to mark nullable fields with
		template<typename T> struct nullable_t : public T
		{
		private:
			bool _null;

		public:
			typedef typename T::type type;
			constexpr static bool nullable = true;

			constexpr nullable_t() noexcept : T(), _null(true) { }
			constexpr nullable_t(const nullptr_t) noexcept : T(), _null(true) { }
			constexpr nullable_t(const type &value) noexcept : T(value), _null(false) { }

			bool isNull() const noexcept { return _null; }

			void value(const nullptr_t) noexcept
			{
				_null = true;
				this->_modified = true;
			}
		};

		// Encodes as a VARCHAR type field (NVARCHAR for MSSQL)
		template<typename _fieldName, uint32_t _length> struct unicode_t : public type_t<_fieldName, char *>
		{
			constexpr uint32_t length() const noexcept { return _length; }
		};

		// Encodes as a TEXT type field (NTEXT for MSSQL)
		template<typename _fieldName> struct unicodeText_t : public type_t<_fieldName, char *> { };
		template<typename _fieldName> struct int64_t : public type_t<_fieldName, std::int64_t> { };
		template<typename _fieldName> struct int32_t : public type_t<_fieldName, std::int32_t> { };
		template<typename _fieldName> struct int16_t : public type_t<_fieldName, std::int16_t> { };
		template<typename _fieldName> struct int8_t : public type_t<_fieldName, std::int8_t> { };
		template<typename _fieldName> struct bool_t : public type_t<_fieldName, bool> { };
		template<typename _fieldName> struct float_t : public type_t<_fieldName, float> { };
		template<typename _fieldName> struct double_t : public type_t<_fieldName, double> { };
		using _dateTime_t = std::chrono::time_point<std::chrono::system_clock>;
		template<typename _fieldName> struct dateTime_t : public type_t<_fieldName, _dateTime_t> { };

		// Convinience just in case you don't like using the stdint.h like types above.
		template<typename fieldName> using bigInt_t = int64_t<fieldName>;
		template<typename fieldName> using long_t = int64_t<fieldName>;
		template<typename fieldName> using int_t = int32_t<fieldName>;
		template<typename fieldName> using short_t = int16_t<fieldName>;
		template<typename fieldName> using tinyInt_t = int8_t<fieldName>;
	}

	namespace common
	{
		using tmplORM::types::type_t;
		using tmplORM::types::autoInc_t;
		using tmplORM::types::primary_t;
		using tmplORM::types::alias_t;

		template<typename> struct toString { };
		template<char... C> struct toString<typestring<C...>>
			{ static const char value[sizeof...(C) + 1]; };
		template<char... C> const char toString<typestring<C...>>::value[sizeof...(C) + 1] = {C..., '\0'};

		namespace intConversion
		{
			template<size_t N> struct fromInt_t;

			template<size_t N, bool = N < 10> struct value_t { using value = typestring<char(N) + '0'>; };
			template<size_t N> struct value_t<N, false>
			{
				using inner = fromInt_t<N / 10>;
				using value = tycat<typename inner::value, typestring<char(N - (inner::number * 10)) + '0'>>;
			};

			template<size_t N> struct fromInt_t
			{
				constexpr static size_t number = N;
				using value = typename value_t<N>::value;
			};
		}
		using intConversion::fromInt_t;
		template<size_t N> using toTypestring = typename fromInt_t<N>::value;
		template<size_t N> using fromInt = toString<toTypestring<N>>;

		constexpr bool collect(const bool value) noexcept { return value; }
		template<typename... values_t> constexpr bool collect(const bool value, values_t ...values) noexcept
			{ return value && collect(values...); }

		constexpr bool bundle(const bool value) noexcept { return value; }
		template<typename... values_t> constexpr bool bundle(const bool value, values_t ...values) noexcept
			{ return value || bundle(values...); }

		template<size_t N> struct comma_t { using value = ts(", "); };
		template<> struct comma_t<1> { using value = typestring<>; };
		template<size_t N> using comma = typename comma_t<N>::value;

		template<bool isNull> struct nullable__t { using value = ts(" NOT NULL"); };
		template<> struct nullable__t<true> { using value = ts(" NULL"); };
		template<bool isNull> using nullable = typename nullable__t<isNull>::value;

		template<size_t N> struct placeholder_t { using value = tycat<typestring<'?'>, comma<N>, typename placeholder_t<N - 1>::value>; };
		template<> struct placeholder_t<0> { using value = typestring<>; };
		template<size_t N> using placeholder = typename placeholder_t<N>::value;

		template<size_t> struct and_t { using value = ts(" AND "); };
		template<> struct and_t<0> { using value = typestring<>; };
		template<size_t N> using and_ = typename and_t<N - 1>::value;

		template<typename fieldName, typename T> constexpr bool isAutoInc(const type_t<fieldName, T> &) noexcept { return false; }
		template<typename T> constexpr bool isAutoInc(const autoInc_t<T> &) noexcept { return true; }
		template<typename... fields> constexpr bool hasAutoInc() noexcept { return bundle(isAutoInc(fields())...); }

		template<typename field, typename... fields> struct autoIncIndex_t
			{ constexpr static size_t index = isAutoInc(field()) ? 0 : 1 + autoIncIndex_t<fields...>::index; };
		template<typename field> struct autoIncIndex_t<field> { constexpr static size_t index = isAutoInc(field()) ? 0 : 1; };
		template<typename tableName, typename... fields_t> auto getAutoInc(const model_t<tableName, fields_t...> &model) noexcept ->
			decltype(std::get<autoIncIndex_t<fields_t...>::index>()) { return std::get<autoIncIndex_t<fields_t...>::index>(model.fields()); }

		template<typename fieldName, typename T> constexpr bool isPrimaryKey(const type_t<fieldName, T> &) noexcept { return false; }
		template<typename T> constexpr bool isPrimaryKey(const primary_t<T> &) noexcept { return true; }
		template<typename... fields> constexpr bool hasPrimaryKey() noexcept { return bundle(isPrimaryKey(fields())...); }

		template<typename field, typename... fields> struct primaryIndex_t
			{ constexpr static size_t index = isPrimaryKey(field()) ? 0 : 1 + primaryIndex_t<fields...>::index; };
		template<typename field> struct primaryIndex_t<field> { constexpr static size_t index = isPrimaryKey(field()) ? 0 : 1; };
		/*template<typename tableName, typename... fields_t> auto getPrimaryKey(const model_t<tableName, fields_t...> &model) noexcept ->
			decltype(std::get<primaryIndex_t<fields_t...>::index>()) { return std::get<primaryIndex_t<fields_t...>::index>(model.fields()); }*/

		template<size_t N, typename... fields> struct countPrimary_t { };
		template<size_t N, typename field, typename... fields> struct countPrimary_t<N, field, fields...>
			{ constexpr static size_t count = (isPrimaryKey(field()) ? 1 : 0) + countPrimary_t<N - 1, fields...>::count; };
		template<> struct countPrimary_t<0> { constexpr static size_t count = 0; };
		template<typename... fields> using countPrimary = countPrimary_t<sizeof...(fields), fields...>;

		template<typename fieldName, typename T> constexpr size_t countInsert_(const type_t<fieldName, T> &) noexcept { return 1; }
		template<typename T> constexpr size_t countInsert_(const autoInc_t<T> &) noexcept { return 0; }
		template<typename...> struct countInsert_t;
		template<typename field, typename... fields> struct countInsert_t<field, fields...>
			{ constexpr static size_t count = countInsert_(field()) + countInsert_t<fields...>::count; };
		template<> struct countInsert_t<> { constexpr static size_t count = 0; };

		template<typename fieldName, typename T> constexpr size_t countUpdate_(const type_t<fieldName, T> &) noexcept { return 1; }
		template<typename T> constexpr size_t countUpdate_(const primary_t<T> &) noexcept { return 0; }
		template<typename...> struct countUpdate_t;
		template<typename field, typename... fields> struct countUpdate_t<field, fields...>
			{ constexpr static const size_t count = countUpdate_(field()) + countUpdate_t<fields...>::count; };
		template<> struct countUpdate_t<> { constexpr static const size_t count = 0; };

		template<typename fieldName, typename T> auto toType_(const type_t<fieldName, T> &) -> type_t<fieldName, T>;
		template<typename field> using toType = decltype(toType_(field()));

		template<typename A, typename B> constexpr bool typestrcmp() noexcept { return std::is_same<A, B>::value; }

		template<bool, typename fieldName, typename... fields> struct fieldIndex__t;
		template<typename name, typename fieldName, typename T> constexpr bool isFieldsName(const type_t<fieldName, T> &) noexcept
			{ return typestrcmp<name, fieldName>(); }
		template<typename name, typename fieldName, typename T> constexpr bool isFieldsName(const alias_t<fieldName, T> &) noexcept
			{ return typestrcmp<name, fieldName>(); }
		template<typename fieldName, typename field, typename... fields>
			using fieldIndex_ = fieldIndex__t<isFieldsName<fieldName>(field()), fieldName, fields...>;
		template<typename fieldName, typename field, typename... fields> struct fieldIndex__t<false, fieldName, field, fields...>
			{ constexpr static size_t index = fieldIndex_<fieldName, field, fields...>::index + 1; };
		template<typename fieldName, typename... fields> struct fieldIndex__t<true, fieldName, fields...>
			{ constexpr static size_t index = 0; };
		template<typename fieldName, typename field, typename... fields> struct fieldIndex_t
			{ constexpr static size_t index = fieldIndex_<fieldName, field, fields...>::index; };

		template<size_t N, typename field, typename... fields> struct fieldType_t
			{ using type = typename fieldType_t<N - 1, fields...>::type; };
		template<typename field, typename... fields> struct fieldType_t<0, field, fields...> { typedef field type; };
		// .first is the valueLength, and .second is the declLength.
		using fieldLength_t = std::pair<const size_t, const size_t>;
	}

	template<typename api_t> struct session_t final
	{
	private:
		api_t session;

	public:
		template<typename... models> bool createTable() noexcept { return collect(session.template createTable(models())...); }
		template<typename model> fixedVector_t<model> select() noexcept { return session.template select<model>(model()); }
		template<typename... models_t> bool add(const models_t &...models) noexcept { return collect(session.template add(models)...); }
		template<typename... models_t> bool update(const models_t &...models) noexcept { return collect(session.template update(models)...); }
		template<typename... models_t> bool del(const models_t &...models) noexcept { return collect(session.template del(models)...); }
		template<typename... models> bool deleteTable() noexcept { return collect(session.template deleteTable(models())...); }
	};
}

#endif /*tmplORM__HXX*/
