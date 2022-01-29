#ifndef tmplORM_STRING_HXX
#define tmplORM_STRING_HXX

#include <cstdarg>
#include <memory>
#include <new>
#include <string>
#include <tmplORM.extern.hxx>
#define tmplORM_STRING_FORMAT_ARGS(n, m) __attribute__((format(printf, n, m)))

/*!
 * @file
 * @author Rachel Mant
 * @date 2016-2020
 * @brief Defines the interface to the string helpers we implement
 */

tmplORM_FNAPI std::unique_ptr<const char []> formatString(const char *format, ...) noexcept tmplORM_STRING_FORMAT_ARGS(1, 2);
tmplORM_FNAPI std::unique_ptr<const char []> vaFormatString(const char *format, va_list args) noexcept;
tmplORM_FNAPI std::unique_ptr<char []> stringDup(const char *const str) noexcept;
inline std::unique_ptr<char []> stringDup(const std::unique_ptr<char []> &str) noexcept { return stringDup(str.get()); }
inline std::unique_ptr<char []> stringDup(const std::unique_ptr<const char []> &str) noexcept { return stringDup(str.get()); }
inline std::unique_ptr<const char []> strNewDup(const char *const str) noexcept { return stringDup(str); }

inline std::string operator ""_s(const char *str, const size_t len) noexcept { return {str, len}; }

struct utf16_t final
{
private:
	std::unique_ptr<char16_t []> str;
	utf16_t() noexcept = default;

public:
	utf16_t(const std::nullptr_t) noexcept : str() { }
	utf16_t(std::unique_ptr<char16_t []> &_str) noexcept : str(std::move(_str)) { }
	utf16_t(std::unique_ptr<char16_t []> &&_str) noexcept : str(std::move(_str)) { }
	utf16_t(utf16_t &&_str) noexcept : str(std::move(_str.str)) { }
	~utf16_t() noexcept = default;
	utf16_t &operator =(utf16_t &&_str) noexcept { str = std::move(_str.str); return *this; }
	operator const std::unique_ptr<char16_t []> &() const noexcept { return str; }
	operator std::unique_ptr<char16_t []>() noexcept { return std::move(str); }
	operator const char16_t *() const noexcept { return str.get(); }
	operator char16_t *() const noexcept { return str.get(); }
	operator const uint16_t *() const noexcept { return reinterpret_cast<uint16_t *>(str.get()); } // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast) lgtm [cpp/reinterpret-cast]
	operator uint16_t *() const noexcept { return reinterpret_cast<uint16_t *>(str.get()); } // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast) lgtm [cpp/reinterpret-cast]
	operator void *() const noexcept { return static_cast<void *>(str.get()); }
	explicit operator bool() const noexcept { return bool(str); }

	utf16_t(const utf16_t &) = delete;
	utf16_t &operator =(const utf16_t &) = delete;
};

struct utf8_t final
{
private:
	std::unique_ptr<char []> str;
	utf8_t() noexcept = default;

public:
	utf8_t(const std::nullptr_t) noexcept : str() { }
	utf8_t(std::unique_ptr<char []> &_str) noexcept : str(std::move(_str)) { }
	utf8_t(std::unique_ptr<char []> &&_str) noexcept : str(std::move(_str)) { }
	utf8_t(utf8_t &&_str) noexcept : str(std::move(_str.str)) { }
	~utf8_t() noexcept = default;
	utf8_t &operator =(utf8_t &&_str) noexcept { str = std::move(_str.str); return *this; }
	operator const std::unique_ptr<char []> &() const noexcept { return str; }
	operator const char *() const noexcept { return str.get(); }
	operator char *() const noexcept { return str.get(); }
	operator const uint8_t *() const noexcept { return reinterpret_cast<uint8_t *>(str.get()); } // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast) lgtm [cpp/reinterpret-cast]
	operator uint8_t *() const noexcept { return reinterpret_cast<uint8_t *>(str.get()); } // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast) lgtm [cpp/reinterpret-cast]
	explicit operator bool() const noexcept { return bool(str); }
	const char *release() noexcept { return str.release(); }

	utf8_t(const utf8_t &) = delete;
	utf8_t &operator =(const utf8_t &) = delete;
};

struct utf16 final
{
private:
	constexpr utf16() noexcept = default;

public:
	static utf16_t convert(const char *const str) noexcept;
	static utf8_t convert(const char16_t *const str) noexcept;

	// We get to use the new std::char_traits<> from C++11 here - this works for char, char16_t, char32_t and wchar_t!
	template<typename T> static size_t length(const T *const str) noexcept
		{ return std::char_traits<T>::length(str) + 1; }
	static size_t length(const utf16_t &str) noexcept { return str ? length<char16_t>(str) : 0; }
	static size_t length(const utf8_t &str) noexcept { return str ? length<char>(str) : 0; }
};

#endif /*tmplORM_STRING_HXX*/
