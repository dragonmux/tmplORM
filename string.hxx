#ifndef STRING__HXX
#define STRING__HXX

#include <memory>
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

#if __cplusplus <= 201103L
template<typename T> struct makeUnique_ { using singleType = std::unique_ptr<T>; };
template<typename T> struct makeUnique_<T []> { using arrayType = std::unique_ptr<T []>; };
template<typename T, size_t N> struct makeUnique_<T [N]> { struct invalidType { }; };

template<typename T, typename... Args> inline typename makeUnique_<T>::singleType make_unique(Args &&...args)
	{ return std::unique_ptr<T>(new T(std::forward<Args>(args)...)); }

template<typename T> inline typename makeUnique_<T>::arrayType make_unique(size_t num)
	{ return std::unique_ptr<T>(new typename std::remove_extent<T>::type[num]()); }

template<typename T, typename... Args> inline typename makeUnique_<T>::invalidType make_unique(Args &&...) = delete;
#else
using std::make_unique;
#endif

#endif /*STRING__HXX*/
