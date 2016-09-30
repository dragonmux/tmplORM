#ifndef STRING__HXX
#define STRING__HXX

#include <memory>
#include <new>
#include <cstdarg>

extern std::unique_ptr<const char []> formatString(const char *format, ...) noexcept;
extern std::unique_ptr<const char []> vaFormatString(const char *format, va_list args) noexcept;

struct toUTF16_t
{
private:
	constexpr toUTF16_t() noexcept { }

public:
	std::unique_ptr<char16_t []> convert(const char *const str) noexcept;
	std::unique_ptr<char []> convert(const char16_t *const str) noexcept;
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
