#ifndef STRING__HXX
#define STRING__HXX

#include <cstdarg>
#include <memory>
#include <new>
#include <string>

std::unique_ptr<const char []> formatString(const char *format, ...) noexcept;
std::unique_ptr<const char []> vaFormatString(const char *format, va_list args) noexcept;
std::unique_ptr<char []> stringDup(const char *const str) noexcept
std::unique_ptr<const char []> strNewDup(const char *const str) noexcept;

struct utf16_t final
{
private:
	std::unique_ptr<char16_t []> str;
	utf16_t() noexcept : str() { }

public:
	utf16_t(const std::nullptr_t) noexcept : str() { }
	utf16_t(std::unique_ptr<char16_t []> &_str) noexcept : str(std::move(_str)) { }
	utf16_t(std::unique_ptr<char16_t []> &&_str) noexcept : str(std::move(_str)) { }
	utf16_t(utf16_t &&_str) noexcept : str(std::move(_str.str)) { }
	utf16_t &operator =(utf16_t &&_str) noexcept { str = std::move(_str.str); return *this; }
	operator const std::unique_ptr<char16_t []> &() const noexcept { return str; }
	operator std::unique_ptr<char16_t []>() noexcept { return std::move(str); }
	operator const char16_t *() const noexcept { return str.get(); }
	operator char16_t *() const noexcept { return str.get(); }
	operator const uint16_t *() const noexcept { return reinterpret_cast<uint16_t *const>(str.get()); }
	operator uint16_t *() const noexcept { return reinterpret_cast<uint16_t *const>(str.get()); }
	explicit operator bool() const noexcept { return bool(str); }
};

struct utf8_t final
{
private:
	std::unique_ptr<char []> str;
	utf8_t() noexcept : str() { }
	utf8_t(const utf8_t &) = delete;
	utf8_t &operator =(const utf8_t &) = delete;

public:
	utf8_t(const std::nullptr_t) noexcept : str() { }
	utf8_t(std::unique_ptr<char []> &_str) noexcept : str(std::move(_str)) { }
	utf8_t(std::unique_ptr<char []> &&_str) noexcept : str(std::move(_str)) { }
	utf8_t(utf8_t &&_str) noexcept : str(std::move(_str.str)) { }
	utf8_t &operator =(utf8_t &&_str) noexcept { str = std::move(_str.str); return *this; }
	operator const std::unique_ptr<char []> &() const noexcept { return str; }
	operator std::unique_ptr<char []>() noexcept { return std::move(str); }
	operator const char *() const noexcept { return str.get(); }
	operator char *() const noexcept { return str.get(); }
	operator const uint8_t *() const noexcept { return reinterpret_cast<uint8_t *const>(str.get()); }
	operator uint8_t *() const noexcept { return reinterpret_cast<uint8_t *const>(str.get()); }
	explicit operator bool() const noexcept { return bool(str); }
};

struct utf16 final
{
private:
	constexpr utf16() noexcept { }

public:
	static utf16_t convert(const char *const str) noexcept;
	static utf8_t convert(const char16_t *const str) noexcept;

	template<typename T> static size_t length(const T *const str) noexcept
		{ return std::char_traits<T>::length(str); }
	static size_t length(const utf16_t &str) noexcept { return str ? length<char16_t>(str) + 1 : 0; }
	static size_t length(const utf8_t &str) noexcept { return str ? length<char>(str) + 1 : 0; }
};

template<typename T> struct makeUnique_ { using uniqueType = std::unique_ptr<T>; };
template<typename T> struct makeUnique_<T []> { using arrayType = std::unique_ptr<T []>; };
template<typename T, size_t N> struct makeUnique_<T [N]> { struct invalidType { }; };

template<typename T, typename... Args> inline typename makeUnique_<T>::uniqueType makeUnique(Args &&...args) noexcept
{
	using consT = typename std::remove_const<T>::type;
	return std::unique_ptr<T>(new (std::nothrow) consT(std::forward<Args>(args)...));
}

template<typename T> inline typename makeUnique_<T>::arrayType makeUnique(const size_t num) noexcept
{
	using consT = typename std::remove_const<typename std::remove_extent<T>::type>::type;
	return std::unique_ptr<T>(new (std::nothrow) consT[num]());
}

template<typename T, typename... Args> inline typename makeUnique_<T>::invalidType makeUnique(Args &&...) noexcept = delete;

#endif /*STRING__HXX*/
