#ifndef tmplORM__HXX
#define tmplORM__HXX

#include <cstdint>
#include <string>
#include <tuple>
#include <memory>
#include <new>
#include <chrono>
#include <type_traits>
#include "typestring/typestring.hh"
#include "tmplORM.extern.hxx"
#include "tmplORM.types.hxx"
#include "fixedVector.hxx"

#define ts(x) typestring_is(x)
#define ts_(x) ts(x)()

constexpr std::chrono::microseconds operator ""_us(const unsigned long long usecs) { return std::chrono::microseconds{usecs}; }

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

		template<typename> struct isBoolean : std::false_type { };
		template<> struct isBoolean<bool> : std::true_type { };

		template<typename T> struct isNumeric : std::integral_constant<bool, std::is_integral<T>::value && !isBoolean<T>::value> { };
	}
	using common::fieldIndex;
	using common::fieldType;

	template<typename... Fields> struct fields_t
	{
	protected:
		constexpr static const size_t N = sizeof...(Fields);
		std::tuple<Fields...> _fields;

		constexpr fields_t() noexcept : _fields{} { }
		constexpr fields_t(Fields &&...fields) noexcept : _fields{fields...} { }

	public:
		const std::tuple<Fields...> &fields() const noexcept { return _fields; }
		std::tuple<Fields...> &fields() noexcept { return _fields; }
		template<char... C> auto operator [](const typestring<C...> &) noexcept -> fieldType<typestring<C...>, Fields...> &
			{ return std::get<fieldIndex<typestring<C...>, Fields...>::index>(_fields); }
	};

	template<typename _tableName, typename... Fields> struct model_t : public fields_t<Fields...>
	{
	public:
		constexpr model_t() noexcept : fields_t<Fields...>{} {}
		constexpr model_t(Fields... fields) noexcept : fields_t<Fields...>{&fields...} { }

		static_assert(common::hasPrimaryKey<Fields...>(), "Model must have a primary key!");
		constexpr static const char *tableName() noexcept { return _tableName::data(); }
		constexpr static const size_t N = fields_t<Fields...>::N;
	};

	namespace types
	{
		using common::isNumeric;

		template<typename _fieldName, typename T> struct type_t
		{
		protected:
			T _value;

		private:
			bool _modified;

		public:
			constexpr type_t() noexcept : _value(), _modified(false) { }
			constexpr type_t(const T value) noexcept : _value(value), _modified(false) { }

			constexpr const char *fieldName() const noexcept { return _fieldName::data(); }
			const T value() const noexcept { return _value; }
 			T value() noexcept { return _value; }
			// Make the type behave like its' contained type..
			operator const T() const noexcept { return _value; }
			void operator =(const T &_value) noexcept { value(_value); }
			bool modified() const noexcept { return _modified; }
			constexpr static bool nullable = false;

			void value(const T &newValue) noexcept
			{
				_value = newValue;
				_modified = true;
			}

			bool operator ==(const type_t<_fieldName, T> &value) const noexcept { return _value == value._value; }
			bool operator !=(const type_t<_fieldName, T> &value) const noexcept { return _value != value._value; }

			using type = T;
		};

		// Tag type to mark auto increment fields with
		template<typename T> struct autoInc_t : public T
		{
			using type = typename T::type;
			static_assert(isNumeric<type>::value, "Cannot create automatically incrementing field from non-numeric base type");
			using T::operator =;
			using T::value;
			using T::operator const type;
			using T::operator ==;
			using T::operator !=;
		};

		// Tag type to mark the primary key field with
		template<typename T> struct primary_t : public T
		{
			using type = typename T::type;
			using T::operator =;
			using T::value;
			using T::operator const type;
			using T::operator ==;
			using T::operator !=;
		};

		// Tag type to give a field a program name different to the field name in the database
		template<typename, typename T> struct alias_t : public T
		{
			using type = typename T::type;
			using T::operator =;
			using T::value;
			operator const type() const noexcept { return value(); }
			using T::operator ==;
			using T::operator !=;
		};

		// Tag type to mark nullable fields with
		template<typename T> struct nullable_t : public T
		{
		private:
			bool _null;

			template<typename value_t = typename T::type> typename std::enable_if<std::is_same<value_t, typename T::type>::value && !std::is_pointer<value_t>::value, const value_t>::type
				_value() const noexcept { return T::value(); }
			template<typename value_t = typename T::type> typename std::enable_if<std::is_same<value_t, typename T::type>::value && std::is_pointer<value_t>::value, const value_t>::type
				_value() const noexcept { return const_cast<value_t>(T::value()); }

		public:
			using type = typename T::type;
			using T::operator ==;
			using T::operator !=;
			constexpr static bool nullable = true;

			constexpr nullable_t() noexcept : T(), _null(true) { }
			constexpr nullable_t(const nullptr_t) noexcept : nullable_t() { }
			constexpr nullable_t(const type &value) noexcept : T(value), _null(false) { }
			bool isNull() const noexcept { return _null; }

			void value(const nullptr_t) noexcept
			{
				_null = true;
				T::value(type());
			}

			void operator =(const nullptr_t) noexcept { value(nullptr); }
			void operator =(const type &_value) noexcept { value(_value); }
			const type value() const noexcept { return _value(); }
			type value() noexcept { return T::value(); }
			void value(const type &_value) noexcept { _null = false; T::value(_value); }
		};

		// Encodes as a VARCHAR type field (NVARCHAR for MSSQL)
		template<typename _fieldName, size_t _length> struct unicode_t : public type_t<_fieldName, const char *>
		{
		/*private:
			using parentType_t = type_t<_fieldName, const char *>;

		public:
			using type = char *;
			using parentType_t::operator =;
			using parentType_t::value;
			using parentType_t::operator const char *const;
			using parentType_t::operator ==;
			using parentType_t::operator !=;

			operator type() const noexcept { return const_cast<const type>(parentType_t::_value); }
			void operator =(type const value) noexcept { *this = const_cast<const char *const>(value); }
			type value() noexcept { return *this; }
			size_t length() const noexcept { return value() ? std::char_traits<char>::length(value()) : 0; }*/
		};

		// Encodes as a TEXT type field (NTEXT for MSSQL)
		template<typename _fieldName> struct unicodeText_t : public type_t<_fieldName, const char *>
		{
		private:
			using parentType_t = type_t<_fieldName, const char *>;

		public:
			using type = char *;
			using parentType_t::operator =;
			using parentType_t::value;
			using parentType_t::operator const char *const;
			using parentType_t::operator ==;
			using parentType_t::operator !=;

			operator type() const noexcept { return const_cast<const type>(parentType_t::_value); }
			void operator =(type const value) noexcept { *this = const_cast<const char *const>(value); }
			type value() noexcept { return *this; }
			size_t length() const noexcept { return value() ? std::char_traits<char>::length(value()) : 0; }
		};

		template<typename _fieldName> using int64_t = type_t<_fieldName, std::int64_t>;
		template<typename _fieldName> using int32_t = type_t<_fieldName, std::int32_t>;
		template<typename _fieldName> using int16_t = type_t<_fieldName, std::int16_t>;

		template<typename _fieldName> struct int8_t : public type_t<_fieldName, std::int8_t>
		{
		private:
			using parentType_t = type_t<_fieldName, std::int8_t>;

		public:
			using type = typename parentType_t::type;
			using parentType_t::operator const type;
			using parentType_t::operator ==;
			using parentType_t::operator !=;

			void operator =(const std::int64_t &value) noexcept { parentType_t::value(std::int8_t(value)); }
			void value(const std::int64_t &_value) noexcept { parentType_t::value(std::int8_t(_value)); }
			type value() noexcept { return *this; }
			const type value() const noexcept { return *this; }
		};

		template<typename _fieldName> using bool_t = type_t<_fieldName, bool>;
		template<typename _fieldName> using float_t = type_t<_fieldName, float>;
		template<typename _fieldName> using double_t = type_t<_fieldName, double>;

		namespace dateTimeTypes
		{
			using std::ratio;
			using namespace std::chrono;

			struct _dateTime_t
			{
			public:
				using timePoint_t = time_point<system_clock>;
				using duration_t = typename timePoint_t::duration;

			private:
				uint16_t _year;
				uint16_t _month;
				uint16_t _day;
				timePoint_t _time;

			public:
				constexpr _dateTime_t() noexcept : _year(0), _month(0), _day(0), _time() { }
				_dateTime_t(const uint16_t year, const uint16_t month, const uint16_t day, const duration_t &time) noexcept :
					_year(year), _month(month), _day(day), _time(timePoint_t(time)) { }
				_dateTime_t(const uint16_t year, const uint16_t month, const uint16_t day) noexcept :
					_year(year), _month(month), _day(day), _time() { }
				constexpr uint16_t year() const noexcept { return _year; }
				constexpr uint16_t month() const noexcept { return _month; }
				constexpr uint16_t day() const noexcept { return _day; }
				constexpr const timePoint_t &time() const noexcept { return _time; }
			};

			template<typename _fieldName> struct dateTime_t : public type_t<_fieldName, _dateTime_t>
			{
			private:
				using parentType_t = type_t<_fieldName, _dateTime_t>;

			public:
				using type = ormDateTime_t;
				operator ormDateTime_t() const noexcept { return dateTime(); }
				void operator =(const char *const _value) noexcept { value(ormDateTime_t(_value)); }
				void operator =(const ormDateTime_t &_value) noexcept { value(_value); }
				void operator =(ormDateTime_t &&_value) noexcept { value(_value); }
				void dateTime(const ormDateTime_t &_value) noexcept { value(_value); }
				ormDateTime_t dateTime() const noexcept { return value(); }
				void value(const char *const _value) noexcept { value(ormDateTime_t(_value)); }

				ormDateTime_t value() const noexcept
				{
					const _dateTime_t _value = parentType_t::value();
					nanoseconds time(_value.time().time_since_epoch());
					const auto hour = duration_cast<hours>(time);
					time -= hour;
					const auto minute = duration_cast<minutes>(time);
					time -= minute;
					const auto second = duration_cast<seconds>(time);
					time -= second;
					return ormDateTime_t(_value.year(), _value.month(), _value.day(),
						hour.count(), minute.count(), second.count(), time.count());
				}

				void value(const ormDateTime_t &_value) noexcept
				{
					nanoseconds time(_value.nanoSecond());
					time += seconds(_value.second());
					time += minutes(_value.minute());
					time += hours(_value.hour());
					parentType_t::value(_dateTime_t(_value.year(), _value.month(), _value.day(),
						duration_cast<typename _dateTime_t::duration_t>(time)));
				}
			};

			template<typename _fieldName> struct date_t : public type_t<_fieldName, _dateTime_t>
			{
			private:
				using parentType_t = type_t<_fieldName, _dateTime_t>;

			public:
				using type = ormDate_t;
				operator ormDate_t() const noexcept { return date(); }
				void operator =(const char *const _value) noexcept { value(ormDate_t(_value)); }
				void operator =(const ormDate_t &_value) noexcept { value(_value); }
				void operator =(ormDate_t &&_value) noexcept { value(_value); }
				void date(const ormDate_t &_value) noexcept { value(_value); }
				ormDate_t date() const noexcept { return value(); }
				void value(const char *const _value) noexcept { value(ormDate_t(_value)); }

				ormDate_t value() const noexcept
				{
					const _dateTime_t _value = parentType_t::value();
					return ormDate_t(_value.year(), _value.month(), _value.day());
				}

				void value(const ormDate_t &_value) noexcept
					{ parentType_t::value(_dateTime_t(_value.year(), _value.month(), _value.day())); }
			};
		}
		using dateTimeTypes::dateTime_t;
		using dateTimeTypes::date_t;

		template<typename _fieldName> struct uuid_t : public type_t<_fieldName, ormUUID_t>
		{
		private:
			using parentType_t = type_t<_fieldName, ormUUID_t>;

		public:
			using type = typename parentType_t::type;
			using parentType_t::operator =;
			using parentType_t::value;
			using parentType_t::operator const type;
			using parentType_t::operator ==;
			using parentType_t::operator !=;

			void operator =(const char *const _value) noexcept { value(ormUUID_t(_value)); }
			void guid(const ormUUID_t &_value) noexcept { value(_value); }
			ormUUID_t guid() const noexcept { return value(); }
			void uuid(const ormUUID_t &_value) noexcept { value(_value); }
			ormUUID_t uuid() const noexcept { return value(); }
			void value(const char *const _value) noexcept { value(ormUUID_t(_value)); }
		};

		// Convinience just in case you don't like using the stdint.h like types above.
		template<typename fieldName> using bigInt_t = int64_t<fieldName>;
		template<typename fieldName> using long_t = int64_t<fieldName>;
		template<typename fieldName> using int_t = int32_t<fieldName>;
		template<typename fieldName> using short_t = int16_t<fieldName>;
		template<typename fieldName> using tinyInt_t = int8_t<fieldName>;
		template<typename fieldName> using bit_t = bool_t<fieldName>;
	}

	namespace common
	{
		using tmplORM::types::type_t;
		using tmplORM::types::autoInc_t;
		using tmplORM::types::primary_t;
		using tmplORM::types::alias_t;

		// This is strictly only required to work around a bug in MSVC++, however it turns out to be useful when writing the generator aliases per-engine.
		template<typename> struct toString { };
		template<char... C> struct toString<typestring<C...>> { static const char value[sizeof...(C) + 1]; };
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
		/*! @brief Generates a comma (as necessary) to put after a field in a statement */
		template<size_t N> using comma = typename comma_t<N>::value;

		template<bool isNull> struct nullable__t { using value = ts(" NOT NULL"); };
		template<> struct nullable__t<true> { using value = ts(" NULL"); };
		template<bool isNull> using nullable = typename nullable__t<isNull>::value;

		template<size_t N> struct placeholder_t { using value = tycat<typestring<'?'>, comma<N>, typename placeholder_t<N - 1>::value>; };
		// Termination here is for 0 rather than 1 to protect us when there are no placeholders to generate
		template<> struct placeholder_t<0> { using value = typestring<>; };
		/*! @brief Generates a list of N prepared execution placeholders for a query statement */
		template<size_t N> using placeholder = typename placeholder_t<N>::value;

		template<size_t> struct and_t { using value = ts(" AND "); };
		template<> struct and_t<0> { using value = typestring<>; };
		template<size_t N> using and_ = typename and_t<N - 1>::value;

		template<typename fieldName, typename T> constexpr bool isAutoInc(const type_t<fieldName, T> &) noexcept { return false; }
		template<typename T> constexpr bool isAutoInc(const autoInc_t<T> &) noexcept { return true; }
		template<typename... fields> constexpr bool hasAutoInc() noexcept { return bundle(isAutoInc(fields())...); }

		template<typename...> struct autoIncIndex_t;
		template<typename field, typename... fields> struct autoIncIndex_t<field, fields...>
			{ constexpr static const size_t index = isAutoInc(field()) ? 0 : 1 + autoIncIndex_t<fields...>::index; };
		template<> struct autoIncIndex_t<> { constexpr static  const size_t index = 0; };
		template<typename... fields> using autoIncType = typename fieldType_t<autoIncIndex_t<fields...>::index, fields...>::type &;
		template<typename tableName, typename... fields_t> auto autoInc(model_t<tableName, fields_t...> &model) noexcept ->
			autoIncType<fields_t...> { return std::get<autoIncIndex_t<fields_t...>::index>(model.fields()); }

		template<bool> struct setAutoInc_t
		{
			template<typename model_t> static void set(model_t &model, const uint64_t rowID) noexcept
				{ autoInc(model) = rowID; }
		};
		template<> struct setAutoInc_t<false> { template<typename model_t> static void set(model_t &, const uint64_t) noexcept { } };

		template<size_t N, typename... fields> struct countAutoInc_t;
		template<size_t N, typename field, typename... fields> struct countAutoInc_t<N, field, fields...>
			{ constexpr static size_t count = (isAutoInc(field()) ? 1 : 0) + countAutoInc_t<N - 1, fields...>::count; };
		template<> struct countAutoInc_t<0> { constexpr static size_t count = 0; };
		template<typename... fields> using countAutoInc = countAutoInc_t<sizeof...(fields), fields...>;

		template<typename fieldName, typename T> constexpr bool isPrimaryKey(const type_t<fieldName, T> &) noexcept { return false; }
		template<typename T> constexpr bool isPrimaryKey(const primary_t<T> &) noexcept { return true; }
		template<typename... fields> constexpr bool hasPrimaryKey() noexcept { return bundle(isPrimaryKey(fields())...); }

		template<typename...> struct primaryIndex_t;
		template<typename field, typename... fields> struct primaryIndex_t<field, fields...>
			{ constexpr static const size_t index = isPrimaryKey(field()) ? 0 : 1 + primaryIndex_t<fields...>::index; };
		template<> struct primaryIndex_t<> { constexpr static const size_t index = 0; };

		template<size_t N, typename... fields> struct countPrimary_t;
		template<size_t N, typename field, typename... fields> struct countPrimary_t<N, field, fields...>
			{ constexpr static size_t count = (isPrimaryKey(field()) ? 1 : 0) + countPrimary_t<N - 1, fields...>::count; };
		template<> struct countPrimary_t<0> { constexpr static size_t count = 0; };
		template<typename... fields> using countPrimary = countPrimary_t<sizeof...(fields), fields...>;

		template<typename fieldName, typename T> constexpr size_t countInsert_(const type_t<fieldName, T> &) noexcept { return 1; }
		template<typename T> constexpr size_t countInsert_(const autoInc_t<T> &) noexcept { return 0; }
		/*! @brief Counts how many fields are insertable for an INSERT query */
		template<typename...> struct countInsert_t;
		template<typename field, typename... fields> struct countInsert_t<field, fields...>
			{ constexpr static const size_t count = countInsert_(field()) + countInsert_t<fields...>::count; };
		template<> struct countInsert_t<> { constexpr static const size_t count = 0; };

		template<typename fieldName, typename T> constexpr size_t countUpdate_(const type_t<fieldName, T> &) noexcept { return 1; }
		template<typename T> constexpr size_t countUpdate_(const primary_t<T> &) noexcept { return 0; }
		/*! @brief Counts how many fields are updateable for an UPDATE query */
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
			{ constexpr static const size_t index = fieldIndex_<fieldName, field, fields...>::index + 1; };
		template<typename fieldName, typename... fields> struct fieldIndex__t<true, fieldName, fields...>
			{ constexpr static const size_t index = 0; };
		template<typename fieldName, typename field, typename... fields> struct fieldIndex_t
			{ constexpr static const size_t index = fieldIndex_<fieldName, field, fields...>::index; };

		template<size_t N, typename field, typename... fields> struct fieldType_t
			{ using type = typename fieldType_t<N - 1, fields...>::type; };
		template<typename field, typename... fields> struct fieldType_t<0, field, fields...> { typedef field type; };
		// .first is the valueLength, and .second is the declLength.
		using fieldLength_t = std::pair<const size_t, const size_t>;

		template<typename api_t> struct session_t final
		{
		private:
			api_t session;

		public:
			session_t() noexcept : session() { }
			~session_t() noexcept { }

			template<typename... models> bool createTable() { return collect(session.template createTable(models())...); }
			template<typename model> fixedVector_t<model> select() { return session.template select<model>(model()); }
			template<typename... models_t> bool add(const models_t &...models) { return collect(session.template add(models)...); }
			/*!
			 * @brief Add model instances to the database
			 * @param models The model instances to add
			 */
			template<typename... models_t> bool add(models_t &...models) { return collect(session.template add(models)...); }
			template<typename... models_t> bool update(const models_t &...models) { return collect(session.template update(models)...); }
			template<typename... models_t> bool del(const models_t &...models) { return collect(session.template del(models)...); }
			template<typename... models> bool deleteTable() { return collect(session.template deleteTable(models())...); }

			api_t &inner() noexcept { return session; }
		};
	}
	using common::session_t;
}

#endif /*tmplORM__HXX*/
