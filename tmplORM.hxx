// SPDX-License-Identifier: BSD-3-Clause
#ifndef tmplORM_HXX
#define tmplORM_HXX

#include <cstdint>
#include <string>
#include <tuple>
#include <bitset>
#include <memory>
#include <new>
#include <chrono>
#include <type_traits>
#include <substrate/fixed_vector>
#include <typestring/typestring.hh>
#include "tmplORM.extern.hxx"
#include "tmplORM.types.hxx"

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ts(x) typestring_is(x)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ts_(x) ts(x)()

using substrate::fixedVector_t;
constexpr std::chrono::microseconds operator ""_us(const unsigned long long usecs)
	{ return std::chrono::microseconds{usecs}; }

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

		// This is strictly only required to work around a bug in MSVC++, however it turns out to be useful when writing the generator aliases per-engine.
		template<typename> struct toString { };
		// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
		template<char... C> struct toString<typestring<C...>> { static const char value[sizeof...(C) + 1]; };
		// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
		template<char... C> const char toString<typestring<C...>>::value[sizeof...(C) + 1] = {C..., '\0'};

	}
	using common::fieldIndex;
	using common::fieldType;

	template<typename... Fields> struct fields_t
	{
	protected:
		constexpr static const size_t N = sizeof...(Fields);
		// NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes,modernize-use-default-member-init)
		std::tuple<Fields...> _fields;

		// NOLINTNEXTLINE(modernize-use-default-member-init)
		constexpr fields_t() noexcept : _fields{} { }
		constexpr fields_t(Fields &&...fields) noexcept : _fields{fields...} { }

	public:
		const std::tuple<Fields...> &fields() const noexcept { return _fields; }
		std::tuple<Fields...> &fields() noexcept { return _fields; }

		template<char... C> auto operator [](const typestring<C...> &) noexcept ->
			fieldType<typestring<C...>, Fields...> &
			{ return std::get<fieldIndex<typestring<C...>, Fields...>::index>(_fields); }

		template<char... C> auto operator [](const typestring<C...> &) const noexcept ->
			const fieldType<typestring<C...>, Fields...> &
			{ return std::get<fieldIndex<typestring<C...>, Fields...>::index>(_fields); }
	};

	template<typename _tableName, typename... Fields> struct model_t : fields_t<Fields...>
	{
	public:
		constexpr model_t() noexcept = default;
		constexpr model_t(Fields... fields) noexcept : fields_t<Fields...>{&fields...} { }

		static_assert(common::hasPrimaryKey<Fields...>(), "Model must have a primary key!");
		constexpr static const char *tableName() noexcept { return _tableName::data(); }
		constexpr static const size_t N = fields_t<Fields...>::N;
	};

	namespace utils
	{
		using tmplORM::common::toString;

		constexpr bool isLowerCase(const char x) noexcept { return x >= 'a' && x <= 'z'; }
		constexpr bool isUpperCase(const char x) noexcept { return x >= 'A' && x <= 'Z'; }
		constexpr char toLower(const char x) noexcept { return isUpperCase(x) ? char(x + 0x20) : x; }
		constexpr char toUpper(const char x) noexcept { return isLowerCase(x) ? char(x - 0x20) : x; }
		constexpr bool isUnderscore(const char x) noexcept { return x == '_'; }

		template<char...> struct isUpperCaseC_t;
		template<char x, char... C> struct isUpperCaseC_t<x, C...>
			{ constexpr static bool value = isUpperCase(x) && isUpperCaseC_t<C...>::value; };
		template<> struct isUpperCaseC_t<> { constexpr static bool value = true; };

		template<typename> struct isUpperCase_t;
		template<char... C> struct isUpperCase_t<typestring<C...>>
			{ constexpr static bool value = isUpperCaseC_t<C...>::value; };

		template<char...> struct hasUnderscore_t;
		template<char x, char... C> struct hasUnderscore_t<x, C...>
			{ constexpr static bool value = isUnderscore(x) || hasUnderscore_t<C...>::value; };
		template<> struct hasUnderscore_t<> { constexpr static bool value = false; };

		template<char...> struct removeUnderscore_t;
		template<char x, char... C> struct removeUnderscore_t<'_', x, C...>
			{ using value = tycat<typestring<toUpper(x)>, typename removeUnderscore_t<C...>::value>; };
		template<char x, char... C> struct removeUnderscore_t<x, C...>
			{ using value = tycat<typestring<toLower(x)>, typename removeUnderscore_t<C...>::value>; };
		template<> struct removeUnderscore_t<> { using value = typestring<>; };

		template<bool, char...> struct toLowerCamelCase_t;
		template<char... C> struct toLowerCamelCase_t<true, C...> :
			toString<typename removeUnderscore_t<C...>::value> { };
		template<char x, char... C> struct toLowerCamelCase_t<false, x, C...> :
			toString<typestring<toLower(x), C...>> { };

		template<typename value, bool = !isUpperCase_t<value>::value> struct lowerCamelCase_t;
		template<char... C> struct lowerCamelCase_t<typestring<C...>, false> : toString<typestring<C...>> { };
		template<char... C> struct lowerCamelCase_t<typestring<C...>, true> :
			toLowerCamelCase_t<hasUnderscore_t<C...>::value, C...> { };

		template<typename> struct isBoolean : public std::false_type { };
		template<> struct isBoolean<bool> : public std::true_type { };
		template<typename T> using isIntegral = std::is_integral<T>;
		template<typename T> using isInteger = std::integral_constant<bool,
			!isBoolean<T>::value && isIntegral<T>::value>;
		template<typename T> using isFloatingPoint = std::is_floating_point<T>;
		template<typename T> using isNumeric = isInteger<T>;

		template<bool B, typename T = void> using enableIf = typename std::enable_if<B, T>::type;
		template<typename T, typename U> using isSame = std::is_same<T, U>;
	} // namespace utils

	namespace types
	{
		using tmplORM::utils::isNumeric;
		using tmplORM::utils::isBoolean;
		using tmplORM::utils::enableIf;
		using tmplORM::utils::isSame;

		template<typename _fieldName, typename T> struct type_t
		{
		protected:
			// NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
			T _value{};

		/*private:
			bool _modified{false};*/

		public:
			constexpr type_t() noexcept = default;
			constexpr type_t(const T value) noexcept : _value{value} { }

			constexpr const char *fieldName() const noexcept { return _fieldName::data(); }
			const T &value() const noexcept { return _value; }
			T &value() noexcept { return _value; }
			// Make the type behave like its' contained type..
			operator T() const noexcept { return _value; }
			void operator =(const T &_value) noexcept { value(_value); }
			//bool modified() const noexcept { return _modified; }
			constexpr static bool nullable = false;

			void value(const T &newValue) noexcept
			{
				_value = newValue;
				//_modified = true;
			}

			template<typename value_t, typename U = T> enableIf<isBoolean<U>::value>
				value(const value_t &newValue) noexcept { _value = bool(newValue); }
			template<typename value_t, typename U = T> enableIf<isBoolean<U>::value>
				operator =(const value_t &newValue) noexcept { value(newValue); }

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
			using T::operator type;
			using T::operator ==;
			using T::operator !=;
		};

		// Tag type to mark the primary key field with
		template<typename T> struct primary_t : public T
		{
			using type = typename T::type;
			using T::operator =;
			using T::value;
			using T::operator type;
			using T::operator ==;
			using T::operator !=;
		};

		// Tag type to give a field a program name different to the field name in the database
		template<typename, typename T> struct alias_t : public T
		{
			using type = typename T::type;
			using T::operator =;
			using T::value;
			using T::operator type;
			using T::operator ==;
			using T::operator !=;
		};

		// Tag type to mark nullable fields with
		template<typename T> struct nullable_t : public T
		{
		private:
			bool _null{true};

			template<typename value_t = typename T::type> typename std::enable_if<std::is_same<value_t, typename T::type>::value && !std::is_pointer<value_t>::value, value_t>::type
				_value() const noexcept { return T::value(); }
			template<typename value_t = typename T::type> typename std::enable_if<std::is_same<value_t, typename T::type>::value && std::is_pointer<value_t>::value, const value_t>::type
				_value() const noexcept { return T::value(); }

		public:
			using type = typename T::type;
			using T::operator ==;
			using T::operator !=;
			constexpr static bool nullable = true;

			constexpr nullable_t() noexcept = default;
			constexpr nullable_t(const nullptr_t) noexcept : nullable_t{} { }
			constexpr nullable_t(const type &value) noexcept : T{value}, _null{false} { }
			bool isNull() const noexcept { return _null; }

			void value(const nullptr_t) noexcept
			{
				_null = true;
				T::value(type());
			}

			void operator =(const nullptr_t) noexcept { value(nullptr); }
			void operator =(const type &_value) noexcept { value(_value); }
			type value() const noexcept { return _value(); }
			type value() noexcept { return T::value(); }
			void value(const type &_value) noexcept { _null = false; T::value(_value); }
			operator type() const noexcept { return _value(); }

			template<typename U = type, typename = enableIf<isSame<U, const char *>::value>>
				// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
				void operator =(const std::unique_ptr<char []> &_value) noexcept { value(_value.get()); }
			template<typename U = type, typename = enableIf<isSame<U, const char *>::value>>
				// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
				void operator =(std::unique_ptr<char []> &&_value) noexcept { value(_value.release()); }
			template<typename U = type, typename = enableIf<isSame<U, const char *>::value>>
				// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
				void value(const std::unique_ptr<char []> &_value) noexcept { value(_value.get()); }
			// TODO: This is bad.. it works, but it leaks.
			template<typename U = type, typename = enableIf<isSame<U, const char *>::value>>
				// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
				void value(std::unique_ptr<char []> &&_value) noexcept { value(_value.release()); }
		};

		// Encodes as a VARCHAR type field (NVARCHAR for MSSQL)
		template<typename _fieldName, size_t _length> struct unicode_t : public type_t<_fieldName, const char *>
		{
		private:
			using parentType_t = type_t<_fieldName, const char *>;

		public:
			using type = typename parentType_t::type;
			using parentType_t::operator =;
			using parentType_t::value;
			using parentType_t::operator type;
			using parentType_t::operator ==;
			using parentType_t::operator !=;

			constexpr unicode_t() noexcept = default;
			constexpr unicode_t(const type value) noexcept : parentType_t{value} { }
			size_t length() const noexcept { return value() ? std::char_traits<char>::length(value()) : 0; }

			// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
			void operator =(const std::unique_ptr<char []> &_value) noexcept { value(_value); }
			// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
			void operator =(std::unique_ptr<char []> &&_value) noexcept { value(std::move(_value)); }
			// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
			void value(const std::unique_ptr<char []> &_value) noexcept { value(_value.get()); }
			// TODO: This is bad.. it works, but it leaks.
			// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
			void value(std::unique_ptr<char []> &&_value) noexcept { value(_value.release()); }
		};

		// Encodes as a TEXT type field (NTEXT for MSSQL)
		template<typename _fieldName> struct unicodeText_t : public type_t<_fieldName, const char *>
		{
		private:
			using parentType_t = type_t<_fieldName, const char *>;

		public:
			using type = typename parentType_t::type;
			using parentType_t::operator =;
			using parentType_t::value;
			using parentType_t::operator type;
			using parentType_t::operator ==;
			using parentType_t::operator !=;

			constexpr unicodeText_t() noexcept = default;
			constexpr unicodeText_t(const type value) noexcept : parentType_t{value} { }
			size_t length() const noexcept { return value() ? std::char_traits<char>::length(value()) : 0; }

			// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
			void operator =(const std::unique_ptr<char []> &_value) noexcept { value(_value); }
			// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
			void operator =(std::unique_ptr<char []> &&_value) noexcept { value(std::move(_value)); }
			// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
			void value(const std::unique_ptr<char []> &_value) noexcept { value(_value.get()); }
			// TODO: This is bad.. it works, but it leaks.
			// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
			void value(std::unique_ptr<char []> &&_value) noexcept { value(_value.release()); }
		};

		template<typename _fieldName> using int64_t = type_t<_fieldName, std::int64_t>;
		template<typename _fieldName> using int32_t = type_t<_fieldName, std::int32_t>;
		template<typename _fieldName> using int16_t = type_t<_fieldName, std::int16_t>;
		template<typename _fieldName> using int8_t = type_t<_fieldName, std::int8_t>;

		template<typename _fieldName> using bool_t = type_t<_fieldName, bool>;
		template<typename _fieldName> using float_t = type_t<_fieldName, float>;
		template<typename _fieldName> using double_t = type_t<_fieldName, double>;

		namespace dateTimeTypes
		{
			using std::ratio;
			using namespace std::chrono;

			struct _time_t
			{
			public:
				using duration_t = typename system_clock::duration;

			private:
				duration_t _time{};

			public:
				constexpr _time_t() noexcept = default;
				constexpr _time_t(const duration_t &time) noexcept : _time{time} { }
				constexpr const duration_t &time() const noexcept { return _time; }
			};

			struct _date_t
			{
			private:
				::int16_t _year{0};
				uint8_t _month{0};
				uint8_t _day{0};

			public:
				constexpr _date_t() noexcept = default;
				// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
				constexpr _date_t(const ::int16_t year, const uint8_t month, const uint8_t day) noexcept :
					_year{year}, _month{month}, _day{day} { }
				constexpr ::int16_t year() const noexcept { return _year; }
				constexpr uint8_t month() const noexcept { return _month; }
				constexpr uint8_t day() const noexcept { return _day; }
			};

			struct _dateTime_t : public _date_t, public _time_t
			{
			public:
				constexpr _dateTime_t() noexcept = default;
				constexpr _dateTime_t(const ::int16_t year, const uint8_t month, const uint8_t day,
					const duration_t &time) noexcept : _date_t{year, month, day}, _time_t{time} { }
				constexpr _dateTime_t(const ::int16_t year, const uint8_t month, const uint8_t day) noexcept :
					_date_t{year, month, day}, _time_t{} { }
				constexpr _dateTime_t(const duration_t &time) noexcept : _date_t{}, _time_t{time} { }
			};

			template<typename _fieldName> struct date_t : public type_t<_fieldName, _date_t>
			{
			private:
				using parentType_t = type_t<_fieldName, _date_t>;

			public:
				using type = ormDate_t;
				constexpr date_t() noexcept = default;
				date_t(const date_t &value) noexcept : parentType_t{value._value} { }
				date_t(date_t &&value) noexcept : parentType_t{value._value} { }
				date_t(const ormDate_t _value) noexcept : parentType_t{} { value(_value); }
				~date_t() noexcept = default;
				operator ormDate_t() const noexcept { return date(); }
				void operator =(const date_t &value) noexcept
				{
					if (this == &value)
						return;
					parentType_t::_value = value._value;
				}
				void operator =(date_t &&value) noexcept { parentType_t::_value = value._value; }
				void operator =(const char *const _value) noexcept { value(_value); }
				void operator =(const ormDate_t &_value) noexcept { value(_value); }
				void operator =(ormDate_t &&_value) noexcept { value(_value); }
				void date(const ormDate_t &_value) noexcept { value(_value); }
				ormDate_t date() const noexcept { return value(); }
				void value(const char *const _value) noexcept
					{ _value ? value(ormDate_t{_value}) : value(ormDate_t{}); }

				ormDate_t value() const noexcept
				{
					const _date_t _value = parentType_t::value();
					return {_value.year(), _value.month(), _value.day()};
				}

				void value(const ormDate_t &_value) noexcept
					{ parentType_t::value({_value.year(), _value.month(), _value.day()}); }
				void value(const ormDateTime_t &_value) noexcept { value(static_cast<const ormDate_t &>(_value)); }
			};

			template<typename _fieldName> struct time_t : public type_t<_fieldName, _time_t>
			{
			private:
				using parentType_t = type_t<_fieldName, _time_t>;

			public:
				using type = ormTime_t;
				constexpr time_t() noexcept = default;
				time_t(const time_t &value) noexcept : parentType_t{value._value} { }
				time_t(time_t &&value) noexcept : parentType_t{value._value} { }
				time_t(const ormTime_t _value) noexcept : parentType_t{} { value(_value); }
				~time_t() noexcept = default;
				operator ormTime_t() const noexcept { return time(); }
				void operator =(const time_t &value) noexcept
				{
					if (this == &value)
						return;
					parentType_t::_value = value._value;
				}
				void operator =(time_t &&value) noexcept { parentType_t::_value = value._value; }
				void operator =(const char *const _value) noexcept { value(_value); }
				void operator =(const ormTime_t &_value) noexcept { value(_value); }
				void operator =(ormTime_t &&_value) noexcept { value(_value); }
				void time(const ormTime_t &_value) noexcept { value(_value); }
				ormTime_t time() const noexcept { return value(); }
				void value(const char *const _value) noexcept
					{ _value ? value(ormTime_t{_value}) : value(ormTime_t{}); }

				ormTime_t value() const noexcept
				{
					const _time_t _value = parentType_t::value();
					_time_t::duration_t time{_value.time()};
					const auto hour = duration_cast<hours>(time);
					time -= hour;
					const auto minute = duration_cast<minutes>(time);
					time -= minute;
					const auto second = duration_cast<seconds>(time);
					time -= second;
					return {uint16_t(hour.count()), uint16_t(minute.count()),
						uint16_t(second.count()), uint32_t(time.count())};
				}

				void value(const ormTime_t &_value) noexcept
				{
					nanoseconds time(_value.nanoSecond());
					time += seconds(_value.second());
					time += minutes(_value.minute());
					time += hours(_value.hour());
					parentType_t::value({duration_cast<typename _time_t::duration_t>(time)});
				}

				void value(const ormDateTime_t &_value) noexcept { value(static_cast<const ormTime_t &>(_value)); }
			};

			template<typename _fieldName> struct dateTime_t : type_t<_fieldName, _dateTime_t>
			{
			private:
				using parentType_t = type_t<_fieldName, _dateTime_t>;

			public:
				using type = ormDateTime_t;
				constexpr dateTime_t() noexcept = default;
				dateTime_t(const dateTime_t &value) noexcept : parentType_t{value._value} { }
				dateTime_t(dateTime_t &&value) noexcept : parentType_t{value._value} { }
				dateTime_t(const ormDateTime_t _value) noexcept : parentType_t{} { value(_value); }
				~dateTime_t() noexcept = default;
				operator ormDateTime_t() const noexcept { return dateTime(); }
				void operator =(const dateTime_t &value) noexcept
				{
					if (this == &value)
						return;
					parentType_t::_value = value._value;
				}
				void operator =(dateTime_t &&value) noexcept { parentType_t::_value = value._value; }
				void operator =(const char *const _value) noexcept { value(_value); }
				void operator =(const ormDateTime_t &_value) noexcept { value(_value); }
				void operator =(ormDateTime_t &&_value) noexcept { value(_value); }
				void dateTime(const ormDateTime_t &_value) noexcept { value(_value); }
				ormDateTime_t dateTime() const noexcept { return value(); }
				void value(const char *const _value) noexcept
					{ _value ? value(ormDateTime_t{_value}) : value(ormDateTime_t{}); }

				ormDateTime_t value() const noexcept
				{
					const _dateTime_t _value = parentType_t::value();
					_dateTime_t::duration_t time{_value.time()};
					const auto hour = duration_cast<hours>(time);
					time -= hour;
					const auto minute = duration_cast<minutes>(time);
					time -= minute;
					const auto second = duration_cast<seconds>(time);
					time -= second;
					return {_value.year(), _value.month(), _value.day(),
						uint16_t(hour.count()), uint16_t(minute.count()),
						uint16_t(second.count()), uint32_t(time.count())};
				}

				void value(const ormDateTime_t &_value) noexcept
				{
					nanoseconds time(_value.nanoSecond());
					time += seconds(_value.second());
					time += minutes(_value.minute());
					time += hours(_value.hour());
					parentType_t::value({_value.year(), _value.month(), _value.day(),
						duration_cast<typename _dateTime_t::duration_t>(time)});
				}
			};
		} // namespace dateTimeTypes

		using dateTimeTypes::date_t;
		using dateTimeTypes::time_t;
		using dateTimeTypes::dateTime_t;

		template<typename _fieldName> struct uuid_t : public type_t<_fieldName, ormUUID_t>
		{
		private:
			using parentType_t = type_t<_fieldName, ormUUID_t>;

		public:
			using type = typename parentType_t::type;
			using parentType_t::operator =;
			using parentType_t::value;
			using parentType_t::operator type;
			using parentType_t::operator ==;
			using parentType_t::operator !=;

			constexpr uuid_t() noexcept : parentType_t{} { }
			uuid_t(const ormUUID_t _value) noexcept : parentType_t{} { value(_value); }
			void operator =(const char *const _value) noexcept { value(_value); }
			void guid(const ormUUID_t &_value) noexcept { value(_value); }
			ormUUID_t guid() const noexcept { return *this; }
			void uuid(const ormUUID_t &_value) noexcept { value(_value); }
			ormUUID_t uuid() const noexcept { return *this; }
			void value(const char *const _value) noexcept
				{ _value ? value(ormUUID_t{_value}) : value(ormUUID_t{}); }
		};

		template<typename _fieldName, size_t _length> struct bitset_t : public type_t<_fieldName, std::bitset<_length>>
		{
		private:
			using parentType_t = type_t<_fieldName, std::bitset<_length>>;
			static_assert(_length > 0, "Cannot have a 0-length bitset");
			static_assert(_length <= 64, "Due to SQL limitations, cannot exceed a 64-length bitset");

			template<size_t N> struct int_t
			{
				template<size_t length, typename = enableIf<N == 1>> static auto eval() -> bool;
				template<size_t length, typename = enableIf<N >= 2 && N <= 8>> static auto eval() -> std::uint8_t;
				template<size_t length, typename = enableIf<N >= 9 && N <= 16>> static auto eval() -> std::uint16_t;
				template<size_t length, typename = enableIf<N >= 17 && N <= 32>> static auto eval() -> std::uint32_t;
				template<size_t length, typename = enableIf<N >= 33 && N <= 64>> static auto eval() -> std::uint64_t;
				using type = decltype(eval<N>());
			};

		public:
			using type = typename parentType_t::type;
			using intType = typename int_t<_length>::type;
			using parentType_t::operator =;
			using parentType_t::value;
			using parentType_t::operator type;
			using parentType_t::operator ==;
			using parentType_t::operator !=;

			constexpr bitset_t() noexcept : parentType_t{} { }
			bitset_t(const intType _value) noexcept : parentType_t{} { value(_value); }
			void operator =(const intType _value) noexcept { value(_value); }
			void value(const intType _value) noexcept { value(type{_value}); }
			constexpr size_t length() const noexcept { return _length; }
		};

		// Convinience just in case you don't like using the stdint.h like types above.
		template<typename fieldName> using bigInt_t = int64_t<fieldName>;
		template<typename fieldName> using long_t = int64_t<fieldName>;
		template<typename fieldName> using int_t = int32_t<fieldName>;
		template<typename fieldName> using short_t = int16_t<fieldName>;
		template<typename fieldName> using tinyInt_t = int8_t<fieldName>;
		template<typename fieldName> using bit_t = bool_t<fieldName>;
	} // namespace types

	// TODO: Fixme. This is half garbage atm.
	namespace condition
	{
		using tmplORM::types::type_t;

		template<typename... conditions> struct where_t { };

		template<typename fieldName, typename value_t> struct equals { };
		template<typename fieldName, typename value_t> struct notEquals { };
		template<typename fieldName, typename value_t> struct less { };
		template<typename fieldName, typename value_t> struct lessOrEquals { };
		template<typename fieldName, typename value_t> struct more { };
		template<typename fieldName, typename value_t> struct moreOrEquals { };
		template<typename fieldName, typename value_t> struct between { };
		template<typename... conditions> struct and_ { };
		template<typename... conditions> struct or_ { };

		// Compile-time operators for defining WHERE conditions
		namespace operators
		{
			template<typename fieldName, typename T, typename value_t> constexpr equals<fieldName, value_t>
				operator ==(const type_t<fieldName, T> &field, const value_t &value) noexcept;
			template<typename fieldName, typename T, typename value_t> constexpr notEquals<fieldName, value_t>
				operator !=(const type_t<fieldName, T> &field, const value_t &value) noexcept;
		}

		// TODO: Perform the condition stack unwind to count the actual number of things that need binding
		template<typename T> constexpr size_t countCond() noexcept { return 0; }
	} // namespace condition
	template<typename... conditions> using where = condition::where_t<conditions...>;

	namespace common
	{
		using tmplORM::types::type_t;
		using tmplORM::types::autoInc_t;
		using tmplORM::types::primary_t;
		using tmplORM::types::alias_t;
		using tmplORM::condition::where_t;
		using tmplORM::condition::countCond;

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
		} // namespace intConversion
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

		template<bool isNull> struct nullableHelper_t { using value = ts(" NOT NULL"); };
		template<> struct nullableHelper_t<true> { using value = ts(" NULL"); };
		template<bool isNull> using nullable = typename nullableHelper_t<isNull>::value;

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

		template<typename condition, typename... conditions> constexpr size_t countCond_() noexcept { return countCond<condition>() + countCond_<conditions...>(); }
		/*! @brief Counts how many values require binding for a SELECT query WHERE clause */
		template<typename> struct countCond_t;
		template<typename... conditions> struct countCond_t<where_t<conditions...>>
			{ constexpr static const size_t count = countCond_<conditions...>(); };

		template<typename fieldName, typename T> constexpr size_t countInsert_(const type_t<fieldName, T> &) noexcept { return 1; }
		template<typename T> constexpr size_t countInsert_(const autoInc_t<T> &) noexcept { return 0; }
		/*! @brief Counts how many fields are insertable for an INSERT query */
		template<typename...> struct countInsert_t;
		template<typename field, typename... fields> struct countInsert_t<field, fields...>
			{ constexpr static const size_t count = countInsert_(field{}) + countInsert_t<fields...>::count; };
		template<> struct countInsert_t<> { constexpr static const size_t count = 0; };

		template<typename fieldName, typename T> constexpr size_t countUpdate_(const type_t<fieldName, T> &) noexcept { return 1; }
		template<typename T> constexpr size_t countUpdate_(const primary_t<T> &) noexcept { return 0; }
		/*! @brief Counts how many fields are updateable for an UPDATE query */
		template<typename...> struct countUpdate_t;
		template<typename field, typename... fields> struct countUpdate_t<field, fields...>
			{ constexpr static const size_t count = countUpdate_(field{}) + countUpdate_t<fields...>::count; };
		template<> struct countUpdate_t<> { constexpr static const size_t count = 0; };

		template<typename fieldName, typename T> auto toType_(const type_t<fieldName, T> &) -> type_t<fieldName, T>;
		template<typename field> using toType = decltype(toType_(field{}));

		template<typename A, typename B> constexpr bool typestrcmp() noexcept { return std::is_same<A, B>::value; }

		template<bool, typename fieldName, typename... fields> struct fieldIndexHelper_t;
		template<typename name, typename fieldName, typename T>
			constexpr bool isFieldsName(const alias_t<fieldName, T> &) noexcept
				{ return typestrcmp<name, fieldName>(); }
		template<typename name, typename fieldName, typename T>
			constexpr bool isFieldsName(const type_t<fieldName, T> &) noexcept
				{ return typestrcmp<name, fieldName>(); }
		template<typename fieldName, typename field, typename... fields>
			using fieldIndex_ = fieldIndexHelper_t<isFieldsName<fieldName>(field{}), fieldName, fields...>;
		template<typename fieldName, typename field, typename... fields>
			struct fieldIndexHelper_t<false, fieldName, field, fields...>
				{ constexpr static const size_t index = fieldIndex_<fieldName, field, fields...>::index + 1; };
		template<typename fieldName, typename... fields> struct fieldIndexHelper_t<true, fieldName, fields...>
			{ constexpr static const size_t index = 0; };
		template<typename fieldName, typename field, typename... fields> struct fieldIndex_t
			{ constexpr static const size_t index = fieldIndex_<fieldName, field, fields...>::index; };

		template<size_t N, typename, typename... fields> struct fieldType_t
			{ using type = typename fieldType_t<N - 1, fields...>::type; };
		template<typename field, typename... fields> struct fieldType_t<0, field, fields...> { using type = field; };
		// .first is the valueLength, and .second is the declLength.
		using fieldLength_t = std::pair<const size_t, const size_t>;

		template<typename api_t> struct session_t final
		{
		private:
			api_t session;

		public:
			session_t() noexcept = default;
			~session_t() noexcept = default;
			session_t(session_t &&) noexcept = default;
			session_t &operator =(session_t &&) noexcept = default;

			template<typename... models> bool createTable() { return collect(session.template createTable(models())...); }
			template<typename model> fixedVector_t<model> select() { return session.template select<model>(model()); }
			template<typename model, typename where> fixedVector_t<model> select(const where &cond) { return session.template select<model, where>(model(), cond); }
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
			const api_t &inner() const noexcept { return session; }

			session_t(const session_t &) = delete;
			session_t &operator =(const session_t &) = delete;
		};
	} // namespace common
	using common::session_t;
} // namespace tmplORM

#endif /*tmplORM_HXX*/
