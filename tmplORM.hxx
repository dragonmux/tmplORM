#ifndef tmplORM__HXX
#define tmplORM__HXX

#include <cstdint>
#include <tuple>
#include "typestring/typestring.hh"

#define ts(x) typestring_is(x)

namespace tmplORM
{
	using namespace irqus;

	template<typename... Fields> struct fields_t
	{
	protected:
		constexpr static const size_t N = sizeof...(Fields);
		std::tuple<Fields...> _fields;

		constexpr fields_t() noexcept : _fields{} {}
		constexpr fields_t(Fields &&...fields) noexcept : _fields{fields...} { }
	};

	template<typename _tableName, typename... Fields> struct model_t : public fields_t<Fields...>
	{
	public:
		constexpr model_t() noexcept : fields_t<Fields...>{} {}
		constexpr model_t(Fields... fields) noexcept : fields_t<Fields...>{fields...} { }

		const char *tableName() const noexcept { return _tableName::data(); }
		constexpr static const size_t N = fields_t<Fields...>::N;

		// create(); - Creates the table
		// add(); - CRUD Create
		// update(); - CRUD Update
		// delete(); - CRUD Delete
	};

	namespace types
	{
		template<typename _fieldName, typename T> struct type_t
		{
			T _value;
			bool _modified;

			constexpr type_t() noexcept : _value(), _modified(false) { }
			constexpr type_t(T value) noexcept : _value(value), _modified(false) { }

			constexpr const char *fieldName() const noexcept { return _fieldName::data(); }
			const T value() const noexcept { return _value; }
			// Make the type behave like it's contained type..
			operator T() const noexcept { return _value; }
			bool modified() const noexcept { return _modified; }
			constexpr static const bool nullable = false;

			void value(const T &newValue) noexcept
			{
				_value = newValue;
				_modified = true;
			}

			// create(); - CREATE TABLE definiton of the field
		};

		// Tag type to mark auto increment fields with
		template<typename T> struct autoInc_t : public T { };

		// Tag type to mark the primary key field with
		template<typename T> struct primary_t : public T { };

		// Tag type to mark nullable fields with
		template<typename T> struct nullable_t : public T { constexpr static const bool nullable = true; };

		// Encodes as a VARCHAR type field (NVARCHAR for MSSQL)
		template<typename _fieldName, uint32_t _length> struct unicode_t : public type_t<_fieldName, char *>
		{
			constexpr uint32_t length() const noexcept { return _length; }
		};

		// Encodes as a TEXT type field (NTEXT for MSSQL)
		template<typename _fieldName> struct unicodeText_t : public type_t<_fieldName, char *>
		{
		};

		template<typename _fieldName> struct int64_t : public type_t<_fieldName, std::int64_t>
		{
		};

		template<typename _fieldName> struct int32_t : public type_t<_fieldName, std::int32_t>
		{
		};

		template<typename _fieldName> struct int16_t : public type_t<_fieldName, std::int16_t>
		{
		};

		template<typename _fieldName> struct int8_t : public type_t<_fieldName, std::int8_t>
		{
		};

		template<typename _fieldName> struct bool_t : public type_t<_fieldName, bool>
		{
		};

		template<typename _fieldName> struct float_t : public type_t<_fieldName, float>
		{
		};

		template<typename _fieldName> struct double_t : public type_t<_fieldName, double>
		{
		};

		// Convinience just in case you don't like using the stdint.h like types above.
		template<typename fieldName> using bigInt_t = int64_t<fieldName>;
		template<typename fieldName> using long_t = int64_t<fieldName>;
		template<typename fieldName> using int_t = int32_t<fieldName>;
		template<typename fieldName> using short_t = int16_t<fieldName>;
		template<typename fieldName> using tinyInt_t = int8_t<fieldName>;
	}

	namespace common
	{
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

		template<size_t N> struct comma_t { using value = ts(", "); };
		template<> struct comma_t<1> { using value = typestring<>; };
		template<size_t N> using comma = typename comma_t<N>::value;

		template<bool isNull> struct nullable__t { using value = ts(" NOT NULL"); };
		template<> struct nullable__t<true> { using value = ts(" NULL"); };
		template<bool isNull> using nullable = typename nullable__t<isNull>::value;

		template<size_t N> struct placeholder_t { using value = tycat<typestring<'?'>, comma<N>, typename placeholder_t<N - 1>::value>; };
		template<> struct placeholder_t<0> { using value = typestring<>; };
		template<size_t N> using placeholder = typename placeholder_t<N>::value;
	}
}

#endif /*tmplORM__HXX*/
