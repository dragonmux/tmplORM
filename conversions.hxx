#ifndef CONVERSIONS__HXX
#define CONVERSIONS__HXX

#include <type_traits>
#include <utility>
#include <string>
#include "string.hxx"

template<typename> struct isBoolean_ : public std::false_type { };
template<> struct isBoolean_<bool> : public std::true_type { };
template<typename T> struct isBoolean : public std::integral_constant<bool,
	isBoolean_<typename std::remove_cv<T>::type>::value> { };

// Type aliases for some type_traits utilities to make them easier to type and read below
template<bool B, typename T = void> using enableIf = typename std::enable_if<B, T>::type;
template<typename A, typename B> using isSame = std::is_same<A, B>;
template<typename A> using isIntegral = std::is_integral<A>;
template<typename A> using isUnsigned = std::is_unsigned<A>;
template<typename A> using isSigned = std::is_signed<A>;
template<typename A> using makeUnsigned = typename std::make_unsigned<A>::type;

template<typename int_t, typename valueType_t> struct fromInt_t
{
private:
	using uint_t = makeUnsigned<int_t>;
	const valueType_t &_value;

	uint8_t calcDigits(const uint_t number) const noexcept
	{
		if (number < 10)
			return 1;
		return 1 + calcDigits(number / 10);
	}

	uint8_t digits(const int_t number) const noexcept
	{
		if (isSigned<int_t>::value && number < 0)
			return 1 + calcDigits(-number);
		return calcDigits(number);
	}

	[[gnu::noinline]] uint_t process(const uint_t number, char *const buffer, const uint8_t digits, const size_t index) const noexcept
	{
		if (number < 10)
		{
			buffer[digits - index] = number + '0';
			buffer[digits + 1] = 0;
		}
		else
		{
			const uint_t num = number - (process(number / 10, buffer, digits, index + 1) * 10);
			buffer[digits - index] = num + '0';
		}
		return number;
	}

	void process(const uint_t number, char *const buffer, const size_t index = 0) const noexcept
		{ process(number, buffer, digits(_value) - 1, index); }
	template<typename T = int_t> enableIf<isSame<T, int_t>::value && isIntegral<T>::value && !isBoolean<T>::value && isUnsigned<T>::value>
		format(char *const buffer) const noexcept { process(_value, buffer); }
	template<typename T = int_t> [[gnu::noinline]] enableIf<isSame<T, int_t>::value && isIntegral<T>::value && !isBoolean<T>::value && isSigned<T>::value>
		format(char *const buffer) const noexcept
	{
		int_t number = _value;
		if (number < 0)
		{
			buffer[0] = '-';
			process(uint_t(-number), buffer, 1);
		}
		else
			process(uint_t(number), buffer);
	}

public:
	constexpr fromInt_t(const valueType_t &value) noexcept : _value(value) { }
	operator std::unique_ptr<char []>() const noexcept
	{
		auto number = makeUnique<char []>(digits(_value) + 1);
		if (!number)
			return nullptr;
		format(number);
		return number;
	}

	operator const char *() const noexcept
	{
		std::unique_ptr<char []> number = *this;
		return number.release();
	}
};

template<typename int_t> struct toInt_t
{
private:
	using uint_t = makeUnsigned<int_t>;
	const char *const _value;
	const size_t _length;
	constexpr static const bool _isSigned = isSigned<int_t>::value;

	static bool isNumber(const char x) noexcept { return x >= '0' && x <= '9'; }
	static bool isHex(const char x) noexcept { return isNumber(x) || (x >= 'a' && x <= 'f') || (x >= 'A' && x <= 'F'); }
	static bool isOct(const char x) noexcept { return x >= '0' && x <= '7'; }

	template<bool isFunc(const char)> bool checkValue() const noexcept
	{
		for (size_t i = 0; i < _length; ++i)
		{
			if (!isFunc(_value[i]))
				return false;
		}
		return true;
	}

public:
	toInt_t(const char *const value) noexcept : _value(value), _length(std::char_traits<char>::length(value)) { }
	constexpr toInt_t(const char *const value, const size_t subLength) noexcept : _value(value), _length(subLength) { }
	size_t length() const noexcept { return _length; }

	bool isInt() const noexcept
	{
		for (size_t i = 0; i < _length; ++i)
		{
			if (_isSigned && i == 0 && _value[i] == '-')
				continue;
			else if (!isNumber(_value[i]))
				return false;
		}
		return true;
	}

	bool isHex() const noexcept { return checkValue<isHex>(); }
	bool isOct() const noexcept { return checkValue<isOct>(); }

	operator int_t() const noexcept
	{
		uint_t value(0);
		for (size_t i(0); i < _length; ++i)
		{
			if (_isSigned && i == 0 && _value[i] == '-')
				continue;
			value *= 10;
			value += _value[i] - '0';
		}
		if (_isSigned && _value[0] == '-')
			return -int_t(value);
		return int_t(value);
	}

	int_t fromHex() const noexcept
	{
		int_t value(0);
		for (size_t i(0); i < _length; ++i)
		{
			uint8_t hex(_value[i]);
			if (!isHex(hex))
				return {};
			else if (hex >= 'a' && hex <= 'f')
				hex -= 0x20;
			value <<= 4;
			hex -= 0x30;
			if (hex > 9)
				hex -= 0x07;
			value += hex;
		}
		return value;
	}

	int_t fromOct() const noexcept
	{
		int_t value(0);
		for (size_t i(0); i < _length; ++i)
		{
			if (!isOct(_value[i]))
				return {};
			value <<= 3;
			value += _value[i] - 0x30;
		}
		return value;
	}
};

inline void swapBytes(uint16_t &val) noexcept
{
	val = ((val >> 8) & 0x0F) || ((val & 0x0F) << 8);
}

inline void swapBytes(uint32_t &val) noexcept
{
	val = ((val >> 24) & 0xFF) | ((val >> 8) & 0xFF00) |
		((val & 0xFF00) << 8) | ((val & 0xFF) << 24);
}

inline void swapBytes(uint64_t &val) noexcept
{
	val = ((val >> 56) & 0xFF) | ((val >> 40) & 0xFF00) | ((val >> 24) & 0xFF0000) | ((val >> 8) & 0xFF000000) |
		((val & 0xFF000000) << 8) | ((val & 0xFF0000) << 24) | ((val & 0xFF00) << 40) | ((val & 0xFF) << 56);
}

#endif /*CONVERSIONS__HXX*/
